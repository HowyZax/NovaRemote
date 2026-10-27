#pragma once

// AI: finds the Nova by opening every serial port at once, one thread per port.
// Each thread sends the PC cVersion string and waits for a packet with a good
// checksum and a known Nova opcode (cVersion with the Nova's name, timecode, or
// meters). The Nova ignores an empty cVersion request (RemoteOasis.cpp), so the
// probe must carry our version string. Threads touch only their own SerialPort;
// results come back through poll() on the UI thread.

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class AutoDetect {
public:
    struct Result {
        std::string port;
        std::string note;   // "NOVA 12T v5.23 sn001234", "no answer", "could not open"
        bool found = false;
    };

    // Start probing. probe = bytes written right after each port opens.
    void start(const std::vector<std::string>& ports, const std::vector<uint8_t>& probe,
               float timeoutSeconds);
    bool running() const;
    void cancel();

    // Call from the UI thread. Returns true once every port has reported.
    // results is filled in the order the ports were given.
    bool poll(std::vector<Result>* results);

private:
    struct Job {
        std::string port;
        std::atomic<bool> done{ false };
        std::atomic<bool> stop{ false };
        std::mutex lock;
        Result result;
    };
    std::vector<std::shared_ptr<Job>> jobs_;
};
