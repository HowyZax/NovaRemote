#include "autodetect.h"
#include "protocol.h"
#include "serial.h"

#include <chrono>
#include <thread>

namespace {

bool IsNovaCommand(uint8_t cmd) {
    switch (cmd) {
    case proto::cInPeaksMix8: case proto::cBusPeaks: case proto::cOutPeaks:
    case proto::cTimecode: case proto::cInPeaksPC: case proto::cInPeaksMix8_COMP:
    case proto::cBusPeaks_COMP: case proto::cOutPeaks_COMP: case proto::cInPeaksPC_COMP:
    case proto::cTimecodeReader: case proto::cVersion: case proto::cGotoPage:
    case proto::cPopup: case proto::cRequest: case proto::cSetting: case proto::cDebug:
    case proto::cEvent: case proto::cCommand:
        return true;
    default:
        return false;
    }
}

} // namespace

void AutoDetect::start(const std::vector<std::string>& ports, const std::vector<uint8_t>& probe,
                       float timeoutSeconds) {
    cancel();
    jobs_.clear();
    for (const std::string& port : ports) {
        auto job = std::make_shared<Job>();
        job->port = port;
        job->result.port = port;
        jobs_.push_back(job);

        // The thread owns the shared_ptr, so it may finish after this object is
        // gone (for example a port that blocks for seconds while opening).
        std::thread([job, probe, timeoutSeconds] {
            SerialPort sp;
            Result r;
            r.port = job->port;
            if (!sp.open(job->port, 38400)) {
                r.note = "could not open";
            } else {
                sp.write(probe);
                std::string buf;
                std::string version;
                bool found = false;
                auto deadline = std::chrono::steady_clock::now()
                              + std::chrono::milliseconds((int)(timeoutSeconds * 1000));
                while (!found && !job->stop && std::chrono::steady_clock::now() < deadline) {
                    std::string rx = sp.readAvailable();
                    if (rx.empty()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(20));
                        continue;
                    }
                    buf.append(rx);
                    if (buf.size() > 4096) buf.erase(0, buf.size() - 4096);
                    proto::parse(buf, [&](uint8_t cmd, const std::string& data) {
                        if (!IsNovaCommand(cmd)) return;
                        found = true;
                        if (cmd == proto::cVersion && !data.empty()) version = data;
                    });
                    if (found && version.empty()) {
                        // Give the Nova a moment to send its name; a bare meter
                        // packet is enough to count as found either way.
                        auto grace = std::chrono::steady_clock::now() + std::chrono::milliseconds(300);
                        while (version.empty() && std::chrono::steady_clock::now() < grace) {
                            std::string more = sp.readAvailable();
                            if (more.empty()) { std::this_thread::sleep_for(std::chrono::milliseconds(20)); continue; }
                            buf.append(more);
                            proto::parse(buf, [&](uint8_t cmd, const std::string& data) {
                                if (cmd == proto::cVersion && !data.empty()) version = data;
                            });
                        }
                    }
                }
                sp.close();
                r.found = found;
                r.note = found ? (version.empty() ? "Nova packets" : version)
                               : (job->stop ? "cancelled" : "no answer");
            }
            std::lock_guard<std::mutex> g(job->lock);
            job->result = r;
            job->done = true;
        }).detach();
    }
}

bool AutoDetect::running() const {
    for (const auto& job : jobs_)
        if (!job->done) return true;
    return false;
}

void AutoDetect::cancel() {
    for (const auto& job : jobs_) job->stop = true;
}

bool AutoDetect::poll(std::vector<Result>* results) {
    bool all = true;
    results->clear();
    for (const auto& job : jobs_) {
        if (!job->done) {
            all = false;
            Result pending;
            pending.port = job->port;
            pending.note = "searching";
            results->push_back(pending);
            continue;
        }
        std::lock_guard<std::mutex> g(job->lock);
        results->push_back(job->result);
    }
    return all;
}
