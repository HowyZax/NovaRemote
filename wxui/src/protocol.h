#pragma once

// AI: PC side of the Nova serial link. Read README.md and
// Reference/NovaSide_v5.23/README.md before changing an opcode, the checksum, or a meter.
// A packet is 0xAA, checksum, length, source, destination, command. The checksum
// is the low 8 bits of the sum of every byte from the length onward. PC commands
// are below 0x6f, so the length is one byte. The port is 38400 8O1. The version
// string is PCnovaT-0.92.11#01.01. NovaSide protocol.h still says PROTOCOL_VERSION
// "00.01"; that string is stale. Add a command only when Reference/NovaSide_v5.23/ZaxLan.h
// already defines it.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace proto {

enum Command : uint8_t {
    cInPeaksMix8 = 0x10,
    cBusPeaks = 0x11,
    cOutPeaks = 0x12,
    cTimecode = 0x16,
    cInPeaksPC = 0x18,
    cInPeaksMix8_COMP = 0x19,
    cBusPeaks_COMP = 0x1A,
    cOutPeaks_COMP = 0x1B,
    cInPeaksPC_COMP = 0x1C,
    cTimecodeReader = 0x1D,
    cVersion = 0x20,
    cGotoPage = 0x52,
    cPopup = 0x53,
    cRequest = 0x54,
    cSetting = 0x55,
    cDebug = 0x56,
    cEvent = 0x57,
    cCommand = 0x58,
};

constexpr uint8_t kMagic = 0xAA;
constexpr int kHeader = 6;

std::vector<uint8_t> pack(uint8_t cmd, const std::string& data = {},
                          uint8_t src = 0, uint8_t dst = 0);

// Append incoming bytes and invoke fn(cmd, payload) for each complete packet.
// Returns number of checksum/parse errors this call.
int parse(std::string& buf, const std::function<void(uint8_t, const std::string&)>& fn);

std::string meterSourceFor(uint8_t cmd, bool* compress);
std::vector<float> decodeMeters(const std::string& data);

} // namespace proto
