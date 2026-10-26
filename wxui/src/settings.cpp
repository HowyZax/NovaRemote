#include "settings.h"

#include <algorithm>
#include <cstring>
#include <iterator>

namespace nova {

const char* const kProtocolVersion = "01.01";

// Index + 1 is the protocol id (settings/id.lua).
static const char* const kNames[] = {
    "sample.rate", "record.preroll", "record.tracks", "tone.level", "tone.enable",
    "home.meters.count", "track.name", "track.color", "trackmix.preset", "trackmix.names",
    "trackmix.phase", "trackmix.fader", "trackmix.matrix", "trackmix.isomatrix", "trackmix.comp",
    "trackmix.tone", "trackmix.slate", "trackmix.neverclip", "trackmix.which", "outputmix.preset",
    "outputmix.names", "outputmix.phase", "outputmix.fader", "outputmix.matrix", "outputmix.comp",
    "outputmix.tone", "outputmix.slate", "outputmix.com1", "outputmix.com2", "outputmix.trk1",
    "outputmix.trk2", "outputmix.neverclip", "outputmix.which", "outputmix.pan.follow", "sourceroute.phase",
    "sourceroute.fader", "mixer.bank", "mixer.enable", "mixer.source", "output.source",
    "hpmix.preset", "hpmix.names", "hpmix.phase", "hpmix.enable", "hpmix.tracks",
    "hpmix.outputs", "hpmix.returns", "hpmix.znet.select", "tapemix.phase", "tapemix.tracks",
    "tapemix.outputs", "tapemix.returns", "monomix.phase", "monomix.tracks", "monomix.outputs",
    "monomix.returns", "metadata.continuity", "metadata.history", "scene.current", "scene.next",
    "scene.continuity", "take.current", "take.next", "take.continuity", "note.current",
    "note.next", "note.preset", "input.channel", "input.warning", "input.level",
    "input.power48V", "input.delay.enable", "input.delay.time", "input.hpf.enable", "input.hpf.freq",
    "input.trim.level", "input.trim.mode", "input.zaxnet.id", "input.compress.enable", "input.compress.attack",
    "input.compress.decay", "input.compress.thresh", "input.compress.ratio", "input.compress.gain", "input.source",
    "input.eq", "input.cl", "input.hpf", "input.ntc1", "input.ntc2",
    "input.phase", "input.dly", "input.name", "input.color", "input.notch.enable",
    "input.notch.freq", "input.notch.q", "names.character.name", "names.character.color", "names.preset.name",
    "names.preset.color", "timecode.out", "timecode.displayed", "timecode.framerate", "timecode.incuserbits",
    "timecode.runmode", "timecode.autojam", "timecode.reader.tc", "timecode.reader.ub", "timecode.generator.tc",
    "timecode.generator.ub", "mirror.drive", "mirror.firewire.power", "mirror.filetype", "mirror.resolution",
    "mirror.src", "mirror.mode", "mirror.startseg", "mirror.endseg", "mirror.timestamp",
    "mirror.tracks", "setup.transport", "setup.gpi1.remoteroll", "setup.serial.remoteroll", "setup.serial.mode",
    "setup.master.mode", "script.file", "script.show.actions", "script.edit.enable", "script.scroll.step",
    "script.font", "store.current", "store.segment", "store.folder", "serial.port",
    "camera.name", "home.meters.source", "mp3record.mode", "wirelessrx.centerfreq", "controller.backlight",
    "neverclip.attenuation", "battery.status", "drive.time.remaining", "drive.time.elapsed", "status.text",
    "meters.meter.hold", "meters.meter.decay", "meters.peak.hold", "meters.peak.decay", "player.clip",
    "player.clips", "ui.update.period", "input.configure", "input.compress.link", "hpmix2.phase",
    "hpmix2.tracks", "hpmix2.outputs", "hpmix2.returns", "meters.count", "meters.source",
    "mp3.record.file", "mp3.record.metadata", "faders.zaxnet.id", "faders.mode",
};
static const int kNameCount = (int)(sizeof(kNames) / sizeof(kNames[0]));

const char* SettingName(int id) {
    return (id >= 1 && id <= kNameCount) ? kNames[id - 1] : nullptr;
}

int SettingId(const std::string& name) {
    for (int i = 0; i < kNameCount; ++i)
        if (name == kNames[i]) return i + 1;
    return 0;
}

// Settings declared with protocol={offset=-1}: the wire value is one less than the setting.
static int ProtocolOffset(const std::string& name) {
    static const char* const offsetMinus1[] = {
        "trackmix.preset", "outputmix.preset", "hpmix.preset", "input.channel", "input.configure",
    };
    for (const char* n : offsetMinus1)
        if (name == n) return -1;
    return 0;
}

namespace {

struct Reader {
    const unsigned char* p;
    size_t n;
    size_t pos = 0;

    bool byte(int* out) {
        if (pos >= n) return false;
        *out = p[pos++];
        return true;
    }
    bool integer(int bytes, long long* out) {
        if (pos + bytes > n) return false;
        unsigned long long v = 0;
        for (int i = 0; i < bytes; ++i)
            v |= (unsigned long long)p[pos + i] << (8 * i);
        if (bytes < 8 && (v & (1ull << (8 * bytes - 1))))
            v |= ~0ull << (8 * bytes);
        *out = (long long)v;
        pos += bytes;
        return true;
    }
    bool cstring(std::string* out) {
        const void* end = std::memchr(p + pos, 0, n - pos);
        if (!end) return false;
        size_t len = (const unsigned char*)end - (p + pos);
        out->assign((const char*)p + pos, len);
        pos += len + 1;
        return true;
    }
};

std::string IndexText(const Index& idx) {
    std::string s;
    for (size_t i = 0; i < idx.size(); ++i) {
        s += i ? "," : "[";
        s += std::to_string(idx[i]);
    }
    if (!s.empty()) s += "]";
    return s;
}

std::string ValueText(const Value& v) {
    return v.isText ? "'" + v.text + "'" : std::to_string(v.num);
}

} // namespace

bool SettingStore::apply(const std::string& payload, std::string* outName, std::string* summary) {
    Reader r{ (const unsigned char*)payload.data(), payload.size() };
    long long id = 0;
    if (!r.integer(2, &id)) return false;
    int kind = 0;
    if (!r.byte(&kind)) return false;
    kind = (signed char)kind;
    const char* cname = SettingName((int)id);
    std::string name = cname ? cname : "setting#" + std::to_string(id);
    if (outName) *outName = name;
    if (kind < 0 || kind > 5) {
        if (summary) *summary = name + ": unknown kind " + std::to_string(kind);
        return false;
    }

    int nArr = 0;
    if (!r.byte(&nArr)) return false;
    Index bounds;
    for (int i = 0; i < nArr; ++i) {
        int b = 0;
        if (!r.byte(&b)) return false;
        bounds.push_back(b);
    }
    Index base;
    if (nArr > 0) {
        int nIdx = 0;
        if (!r.byte(&nIdx)) return false;
        for (int i = 0; i < nIdx; ++i) {
            int b = 0;
            if (!r.byte(&b)) return false;
            base.push_back(b + 1);
        }
    }

    auto readValue = [&](Value* v) -> bool {
        if (kind == 0) {
            v->isText = true;
            return r.cstring(&v->text);
        }
        if (kind == 5) {
            v->isText = true;
            for (;;) {
                std::string k, val;
                if (!r.cstring(&k)) return false;
                if (k.empty()) return true;
                if (!r.cstring(&val)) return false;
                if (!v->text.empty()) v->text += ";";
                v->text += k + "=" + val;
            }
        }
        if (!r.integer(kind, &v->num)) return false;
        v->num -= ProtocolOffset(name);
        return true;
    };

    meta_[name] = Meta{ kind, bounds };
    auto& slot = values_[name];
    std::string desc = name;
    if (nArr == 0) {
        Value v;
        if (!readValue(&v)) return false;
        slot[{}] = v;
        desc += " = " + ValueText(v);
    } else {
        for (auto it = slot.begin(); it != slot.end();) {
            const Index& k = it->first;
            bool under = k.size() >= base.size() && std::equal(base.begin(), base.end(), k.begin());
            it = under ? slot.erase(it) : std::next(it);
        }
        desc += IndexText(bounds) + " from " + (base.empty() ? "[all]" : IndexText(base)) + ":";
        Index cur;
        bool have = false;
        int count = 0;
        while (r.pos < r.n) {
            if (!have) {
                cur = base;
                for (int k = (int)base.size(); k < nArr; ++k) {
                    int b = 0;
                    if (!r.byte(&b)) return false;
                    cur.push_back(b + 1);
                }
                have = true;
            } else {
                int j = 0;
                if (!r.byte(&j)) return false;
                if (j == 0xFF) {
                    have = false;
                    continue;
                }
                if (cur.empty()) return false;
                cur.back() = j + 1;
            }
            Value v;
            if (!readValue(&v)) return false;
            slot[cur] = v;
            if (count < 24) desc += " " + IndexText(cur) + "=" + ValueText(v);
            ++count;
        }
        if (count >= 24) desc += " ... (" + std::to_string(count) + " values)";
        if (count == 0) desc += " (all zero)";
    }
    if (summary) *summary = desc;
    return r.pos == r.n;
}

bool SettingStore::encodeHeader(const std::string& name, const Index& idx, int kind,
                                const Index& fallbackBounds, std::string* payload, int* usedKind) {
    const int id = SettingId(name);
    if (!id) return false;
    Index bounds = fallbackBounds;
    auto it = meta_.find(name);
    if (it != meta_.end()) {
        if (it->second.kind >= 0) kind = it->second.kind;
        if (!it->second.bounds.empty()) bounds = it->second.bounds;
    }
    // The Nova sometimes advertises a shorter array than the indexes it
    // actually accepts (track mix says 22 sources, then uses output cells
    // at 23-26). A caller can pass a wider fallback for those indexes.
    for (size_t i = 0; i < bounds.size() && i < fallbackBounds.size(); ++i)
        if (fallbackBounds[i] > bounds[i]) bounds[i] = fallbackBounds[i];
    if (idx.size() != bounds.size()) return false;
    for (size_t i = 0; i < idx.size(); ++i)
        if (idx[i] < 1 || idx[i] > bounds[i]) return false;

    std::string& b = *payload;
    b.clear();
    b += (char)(id & 0xFF);
    b += (char)((id >> 8) & 0xFF);
    b += (char)kind;
    b += (char)bounds.size();
    for (int v : bounds) b += (char)v;
    if (!bounds.empty()) {
        b += (char)idx.size();
        for (int v : idx) b += (char)(v - 1);
    }
    *usedKind = kind;
    return true;
}

bool SettingStore::encodeNumber(const std::string& name, const Index& idx, long long value,
                                std::string* payload, int fallbackKind, const Index& fallbackBounds) {
    int kind = 0;
    if (!encodeHeader(name, idx, fallbackKind, fallbackBounds, payload, &kind)) return false;
    if (kind < 1 || kind > 4) return false;
    // Array elements that are zero are sent as no value at all (settings/protocol.lua).
    if (idx.empty() || value != 0) {
        long long wire = value + ProtocolOffset(name);
        for (int i = 0; i < kind; ++i)
            *payload += (char)((wire >> (8 * i)) & 0xFF);
    }
    Value v;
    v.num = value;
    values_[name][idx] = v;
    return true;
}

bool SettingStore::encodeText(const std::string& name, const Index& idx, const std::string& value,
                              std::string* payload, const Index& fallbackBounds) {
    int kind = 0;
    if (!encodeHeader(name, idx, 0, fallbackBounds, payload, &kind)) return false;
    if (kind != 0) return false;
    if (idx.empty() || !value.empty()) {
        *payload += value;
        *payload += '\0';
    }
    Value v;
    v.isText = true;
    v.text = value;
    values_[name][idx] = v;
    return true;
}

bool SettingStore::has(const std::string& name) const {
    return values_.count(name) != 0;
}

const Value* SettingStore::get(const std::string& name, const Index& idx) const {
    auto it = values_.find(name);
    if (it == values_.end()) return nullptr;
    auto jt = it->second.find(idx);
    return jt == it->second.end() ? nullptr : &jt->second;
}

long long SettingStore::num(const std::string& name, long long def, const Index& idx) const {
    const Value* v = get(name, idx);
    return (v && !v->isText) ? v->num : def;
}

std::string SettingStore::text(const std::string& name, const std::string& def, const Index& idx) const {
    const Value* v = get(name, idx);
    if (!v) return def;
    return v->isText ? v->text : std::to_string(v->num);
}

} // namespace nova
