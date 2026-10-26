#include "protocol.h"

namespace proto {

std::vector<uint8_t> pack(uint8_t cmd, const std::string& data, uint8_t src, uint8_t dst) {
    if (data.size() > 0xFF) return {};
    std::vector<uint8_t> pkt;
    pkt.reserve(kHeader + data.size());
    pkt.push_back(kMagic);
    pkt.push_back(0); // checksum placeholder
    pkt.push_back((uint8_t)data.size());
    pkt.push_back(src);
    pkt.push_back(dst);
    pkt.push_back(cmd);
    pkt.insert(pkt.end(), data.begin(), data.end());
    unsigned sum = 0;
    for (size_t i = 2; i < pkt.size(); ++i)
        sum += pkt[i];
    pkt[1] = (uint8_t)(sum & 0xFF);
    return pkt;
}

int parse(std::string& buf, const std::function<void(uint8_t, const std::string&)>& fn) {
    int errors = 0;
    while (!buf.empty()) {
        size_t magic = buf.find((char)kMagic);
        if (magic == std::string::npos) {
            buf.clear();
            break;
        }
        if (magic > 0) buf.erase(0, magic);
        if (buf.size() < (size_t)kHeader) break;
        const uint8_t len = (uint8_t)buf[2];
        const size_t total = (size_t)kHeader + len;
        if (buf.size() < total) break;
        unsigned sum = 0;
        for (size_t i = 2; i < total; ++i)
            sum += (uint8_t)buf[i];
        if ((uint8_t)(sum & 0xFF) != (uint8_t)buf[1]) {
            ++errors;
            buf.erase(0, 1);
            continue;
        }
        uint8_t cmd = (uint8_t)buf[5];
        std::string data = buf.substr(kHeader, len);
        fn(cmd, data);
        buf.erase(0, total);
    }
    return errors;
}

std::string meterSourceFor(uint8_t cmd, bool* compress) {
    if (compress) *compress = false;
    switch (cmd) {
    case cInPeaksPC: return "meters.in";
    case cBusPeaks: return "meters.bus";
    case cOutPeaks: return "meters.out";
    case cInPeaksMix8: return "meters.bridge";
    case cInPeaksPC_COMP:
        if (compress) *compress = true;
        return "meters.in";
    case cBusPeaks_COMP:
        if (compress) *compress = true;
        return "meters.bus";
    case cOutPeaks_COMP:
        if (compress) *compress = true;
        return "meters.out";
    case cInPeaksMix8_COMP:
        if (compress) *compress = true;
        return "meters.bridge";
    default: return {};
    }
}

std::vector<float> decodeMeters(const std::string& data) {
    constexpr float kRange = 40.f;
    std::vector<float> out;
    out.reserve(data.size());
    for (unsigned char ch : data) {
        if (ch < 255) ch = (unsigned char)(ch + 1);
        float val = (ch - (255 - kRange)) / kRange;
        if (val < 0) val = 0;
        if (val > 1) val = 1;
        out.push_back(val);
    }
    return out;
}

} // namespace proto
