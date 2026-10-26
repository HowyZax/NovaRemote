#pragma once

#include <map>
#include <string>
#include <vector>

// AI: cSetting (0x55) values. Indexes in this store are 1-based. The wire index is
// 0-based. Kind and bounds come from the last packet the Nova sent. The bounds in
// NovaSide_v5.23/protocol.h are the 2016 generator, so trust a live dump over that
// file. These five are stored one higher than the wire value: trackmix.preset,
// outputmix.preset, hpmix.preset, input.channel, input.configure.
// Input arrays are 22 wide and packed: analog 1-4, return 5-6, AES 7-14, RX 15-22.
// Send that packed index. Do not add 10 to skip the dummy gChannelConfig slots.
// Track-mix sources 23-26 are outputs. The dump advertises a source bound of 22;
// pass a wider fallback to encodeNumber for those cells.
// Sample rate, routing, names, and input setup on a connected Nova are live.
// Do not send one of those just to see if encoding works.
namespace nova {

extern const char* const kProtocolVersion;

struct Value {
    bool isText = false;
    long long num = 0;
    std::string text;
};

using Index = std::vector<int>;  // 1-based; empty for scalar settings

const char* SettingName(int id);              // nullptr if unknown
int SettingId(const std::string& name);       // 0 if unknown

class SettingStore {
public:
    // Decode one cSetting payload. On success *name is the setting and *summary a
    // human-readable description of what changed.
    bool apply(const std::string& payload, std::string* name, std::string* summary);

    // Build a cSetting payload for one scalar (idx empty) or one array element, and
    // update the local copy. Value size and array bounds come from what the Nova sent;
    // fallbacks are used before anything has been received.
    bool encodeNumber(const std::string& name, const Index& idx, long long value,
                      std::string* payload, int fallbackKind = 1, const Index& fallbackBounds = {});
    bool encodeText(const std::string& name, const Index& idx, const std::string& value,
                    std::string* payload, const Index& fallbackBounds = {});

    bool has(const std::string& name) const;
    const Value* get(const std::string& name, const Index& idx = {}) const;
    long long num(const std::string& name, long long def, const Index& idx = {}) const;
    std::string text(const std::string& name, const std::string& def, const Index& idx = {}) const;

private:
    struct Meta {
        int kind = -1;
        Index bounds;
    };
    std::map<std::string, std::map<Index, Value>> values_;
    std::map<std::string, Meta> meta_;

    bool encodeHeader(const std::string& name, const Index& idx, int kind,
                      const Index& fallbackBounds, std::string* payload, int* usedKind);
};

} // namespace nova
