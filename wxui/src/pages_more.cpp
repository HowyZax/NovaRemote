// AI: Input Config sends the packed PC channel, 1-22, which is the index in the
// Nova dump. Analog and return match gChannelConfig. AES and RX do not: the
// firmware stores those columns without ConvertPCchanToNova. Keep sending the
// packed index. Attack, decay, threshold, ratio, and gain are the one input
// compressor at index {1}, shared by every channel.
#include "pages.h"
#include "meter.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

static std::unique_ptr<Panel> MakeColumn() {
    auto p = std::make_unique<Panel>(LayoutDir::Column);
    p->style = "none";
    p->margin = 4;
    return p;
}

static std::unique_ptr<Panel> MakeRow() {
    auto p = std::make_unique<Panel>(LayoutDir::Row);
    p->style = "none";
    p->margin = 4;
    return p;
}

static void AddSpacer(Panel& parent) {
    auto s = std::make_unique<Panel>();
    s->style = "none";
    parent.add(std::move(s));
}

static std::unique_ptr<Button> MenuButton(const std::string& id, const std::string& label) {
    auto b = std::make_unique<Button>();
    b->id = id;
    b->style = "tab";
    b->background = Color::rgb(0x3a3a3a);
    b->color = Color{ 230, 230, 230 };
    auto bar = label.find('|');
    if (bar == std::string::npos) {
        b->title = label;
    } else {
        b->title = label.substr(0, bar);
        b->value = label.substr(bar + 1);
        b->showValue = true;
    }
    return b;
}

static void CycleValue(Button* b, std::vector<std::string> values) {
    b->showValue = true;
    if (b->value.empty() && !values.empty()) b->value = values[0];
    b->click = [b, values] {
        int i = 0;
        for (int k = 0; k < (int)values.size(); ++k) {
            if (values[k] == b->value) { i = k; break; }
        }
        b->value = values[(i + 1) % (int)values.size()];
    };
}

static std::unique_ptr<Button> ValueButton(const std::string& id, const std::string& title,
                                           const std::string& value) {
    auto b = std::make_unique<Button>();
    b->id = id;
    b->title = title;
    b->value = value;
    b->showValue = true;
    return b;
}

static std::unique_ptr<Page> GridPage(const std::string& title,
                                      std::vector<std::unique_ptr<Button>> cells,
                                      int columns) {
    auto page = std::make_unique<Page>(title);
    page->dir = LayoutDir::Column;
    page->margin = 8;
    std::unique_ptr<Panel> row;
    int col = 0;
    for (size_t i = 0; i < cells.size(); ++i) {
        if (col == 0) row = MakeRow();
        if (cells[i]) row->add(std::move(cells[i]));
        else AddSpacer(*row);
        ++col;
        if (col == columns) {
            page->add(std::move(row));
            col = 0;
        }
    }
    if (row) page->add(std::move(row));
    return page;
}

std::unique_ptr<Page> BuildMainMenuPage() {
    const char* labels[] = {
        "nav:OutputMix|Output|Mix",
        "nav:TrackMix|Track|Mix",
        "nav:HeadphoneMix|Headphone|Mix",
        "",
        "",
        "nav:RecordChannels|Record|Enables",
        "nav:TrackNames|Track|Names",
        "",
        "nav:InputConfigure|Input|Config",
        "",
        "",
        "",
        "",
        "",
        "nav:SetupMenu|Setup",
        "Tone|Tone",
        "",
        "",
        "nav:Fullscreen|Full|Screen",
        "nav:Exit|Exit",
    };
    std::vector<std::unique_ptr<Button>> cells;
    for (const char* spec : labels) {
        if (!spec[0]) {
            cells.push_back(nullptr);
            continue;
        }
        std::string s = spec;
        auto bar = s.find('|');
        std::string id = s.substr(0, bar);
        std::string label = s.substr(bar + 1);
        auto b = MenuButton(id, label);
        if (id == "Tone") CycleValue(b.get(), { "OFF", "ON" });
        if (id == "nav:Exit") {
            b->background = Color{ 120, 0, 0 };
            b->color = Color{ 255, 255, 255 };
        }
        cells.push_back(std::move(b));
    }
    return GridPage("Main Menu", std::move(cells), 5);
}

std::unique_ptr<Page> BuildSetupPage() {
    const char* labels[] = {
        "PreRecordSetup|Pre|Record",
        "",
        "Mp3Rec|MP3|Rec",
        "HomeMeters|Home|Meters",
        "",
        "SerialSetup|Serial",
        "SearchPorts|Search Serial Ports",
        "IsoAtt|ISO|Attenuation",
        "",
        "",
        "",
        "",
        "",
        "",
        "SaveLayout|Save|Layout",
        "",
        "",
        "nav:About|About|Touch",
        "",
        "",
    };
    std::vector<std::unique_ptr<Button>> cells;
    for (const char* spec : labels) {
        if (!spec[0]) {
            cells.push_back(nullptr);
            continue;
        }
        std::string s = spec;
        auto bar = s.find('|');
        std::string id = s.substr(0, bar);
        auto b = MenuButton(id, s.substr(bar + 1));
        if (id == "PreRecordSetup") CycleValue(b.get(), { "OFF", "3 sec", "6 sec", "10 sec" });
        if (id == "Mp3Rec") { b->value = "OFF"; b->showValue = true; }
        if (id == "HomeMeters") { b->value = "8"; b->showValue = true; }
        if (id == "SerialSetup") { b->value = "none"; b->showValue = true; } // app.cpp sets the port
        // SearchPorts is an action button; app.cpp shows "searching..." on it only while probing.
        if (id == "IsoAtt") CycleValue(b.get(), { "OFF", "3 dB", "6 dB", "10 dB" });
        cells.push_back(std::move(b));
    }
    return GridPage("Setup Menu", std::move(cells), 5);
}

std::unique_ptr<Page> BuildRecordPage() {
    auto page = std::make_unique<Page>("Record Enables");
    page->dir = LayoutDir::Column;
    page->margin = 8;
    const int columns = 8;
    std::unique_ptr<Panel> row;
    for (int i = 1; i <= 16; ++i) {
        if ((i - 1) % columns == 0) row = MakeRow();
        auto cell = MakeColumn();
        cell->margin = 3;
        auto name = std::make_unique<Label>();
        name->id = "ArmName" + std::to_string(i);
        name->text = "TRACK " + std::to_string(i);
        name->align = "center";
        name->style = "none";
        name->fixedFrac = 0.28f;
        cell->add(std::move(name));
        auto b = std::make_unique<Button>();
        b->id = "Arm" + std::to_string(i);
        b->title = "DISARMED";
        b->color = Color{ 160, 160, 160 };
        cell->add(std::move(b));
        row->add(std::move(cell));
        if (i % columns == 0) page->add(std::move(row));
    }
    return page;
}

void SyncRecordPage(Page& page, const AppModel& m) {
    for (int i = 1; i <= 16; ++i) {
        if (auto* l = dynamic_cast<Label*>(page.find("ArmName" + std::to_string(i))))
            l->text = m.trackName[i - 1].empty() ? "TRACK " + std::to_string(i) : m.trackName[i - 1];
        auto* b = dynamic_cast<Button*>(page.find("Arm" + std::to_string(i)));
        if (!b) continue;
        if (m.trackArmed[i - 1]) {
            b->title = "ARMED";
            b->color = NamedColor("orangered");
            b->background = Color::rgb(0x300000);
        } else {
            b->title = "DISARMED";
            b->color = Color{ 170, 170, 170 };
            b->background = Color{ 0, 0, 0 };
        }
    }
}

static std::unique_ptr<Panel> TitledGroup(const std::string& id, const std::string& title) {
    auto box = MakeColumn();
    box->id = id;
    box->style = "panel";
    box->margin = 4;
    auto label = std::make_unique<Label>();
    label->text = title;
    label->align = "center";
    label->style = "none";
    label->fixedFrac = 0.16f;
    box->add(std::move(label));
    return box;
}

// Packed PC input list, 1-based. Same order as the mix sources and as
// ConvertNovaChanToPC: analog, return, AES, RX. Not gChannelConfig order.
static const int kInputChannels = 22;

static std::string InputName(int ch) {
    if (ch < 1) ch = 1;
    if (ch > kInputChannels) ch = kInputChannels;
    if (ch <= 4) return "ANALOG " + std::to_string(ch);
    if (ch <= 6) return "RET " + std::to_string(ch - 4);
    if (ch <= 14) return "DIGITAL " + std::to_string(ch - 6);
    return "RX " + std::to_string(ch - 14);
}

static std::string DbText(long long v) {
    if (v > 0) return "+" + std::to_string(v) + " dB";
    return std::to_string(v) + " dB";
}

static std::string TenthsMs(long long tenths) {
    char b[32];
    std::snprintf(b, sizeof(b), "%.1f ms", tenths / 10.0);
    return b;
}

static std::string RatioText(long long tenths) {
    char b[32];
    std::snprintf(b, sizeof(b), "%.1f:1", tenths / 10.0);
    return b;
}

static const char* LinkText(long long v) {
    static const char* const names[] = { "NONE", "GROUP A", "GROUP B", "GROUP C" };
    if (v < 0 || v > 3) return names[0];
    return names[v];
}

std::unique_ptr<Page> BuildInputConfigPage() {
    auto page = std::make_unique<Page>("Input Config");
    page->dir = LayoutDir::Row;
    page->margin = 6;

    auto input = MakeColumn();
    input->add(ValueButton("InChannel", "INPUT", "ANALOG 1"));
    input->add(ValueButton("InLevel", "LEVEL", "MIC"));
    input->add(ValueButton("InTrim", "TRIM LEVEL", "0 dB"));
    input->add(ValueButton("In48", "48V", "OFF"));
    input->add(ValueButton("InPhase", "PHASE", "Normal"));
    page->add(std::move(input));

    auto mid = MakeColumn();
    mid->id = "InMid";
    auto notch1 = TitledGroup("Notch1Box", "NOTCH 1");
    notch1->add(ValueButton("Notch1", "ENABLE", "OFF"));
    notch1->add(ValueButton("Notch1Freq", "FREQ", "0 Hz"));
    mid->add(std::move(notch1));
    auto delay = TitledGroup("DelayBox", "DELAY");
    delay->add(ValueButton("DelayEn", "ENABLE", "OFF"));
    delay->add(ValueButton("DelayTime", "DELAY TIME", "0.0 ms"));
    mid->add(std::move(delay));
    page->add(std::move(mid));

    auto mid2 = MakeColumn();
    mid2->id = "InMid2";
    auto notch2 = TitledGroup("Notch2Box", "NOTCH 2");
    notch2->add(ValueButton("Notch2", "ENABLE", "OFF"));
    notch2->add(ValueButton("Notch2Freq", "FREQ", "0 Hz"));
    mid2->add(std::move(notch2));
    auto hpf = TitledGroup("HpfBox", "HPF");
    hpf->add(ValueButton("HpfEn", "ENABLE", "OFF"));
    hpf->add(ValueButton("HpfFreq", "HPF FREQ", "40 Hz"));
    mid2->add(std::move(hpf));
    page->add(std::move(mid2));

    auto meterCol = MakeColumn();
    meterCol->fixedFrac = 0.12f;
    auto ml = std::make_unique<Label>();
    ml->text = "LEVEL";
    ml->align = "center";
    ml->style = "none";
    ml->fixedFrac = 0.08f;
    meterCol->add(std::move(ml));
    auto meter = std::make_unique<Meter>();
    meter->id = "InputMeter";
    // The 17th peak of the Mix8 meter packet is the Nova's current input.
    meter->source = "meters.input";
    meter->tag = 1;
    meter->vertical = true;
    meterCol->add(std::move(meter));
    page->add(std::move(meterCol));

    auto comp = MakeColumn();
    comp->id = "InComp";
    comp->add(ValueButton("CompEn", "COMPRESS", "OFF"));
    comp->add(ValueButton("CompLink", "LINK", "NONE"));
    comp->add(ValueButton("CompAtk", "ATTACK", "5 ms"));
    comp->add(ValueButton("CompDec", "DECAY", "100 ms"));
    comp->add(ValueButton("CompTh", "THRESH", "-6 dB"));
    comp->add(ValueButton("CompRatio", "RATIO", "4.0:1"));
    page->add(std::move(comp));
    return page;
}

void SyncInputConfigPage(Page& page, const nova::SettingStore& s) {
    int ch = (int)s.num("input.configure", 1);
    if (ch < 1 || ch > kInputChannels) ch = 1;
    const bool analog = ch <= 4;
    const bool digital = ch >= 7 && ch <= 14;
    const bool rest = !digital;
    bool relayout = false;
    auto set = [&](const char* id, const std::string& value, bool show) {
        auto* b = dynamic_cast<Button*>(page.find(id));
        if (!b) return;
        b->value = value;
        b->showValue = true;
        if (b->visible != show) {
            b->visible = show;
            relayout = true;
        }
    };
    set("InChannel", InputName(ch), true);
    set("InLevel", s.num("input.level", 0, { ch }) ? "LINE" : "MIC", analog);
    set("InTrim", DbText(s.num("input.trim.level", 0, { ch })), digital);
    set("In48", s.num("input.power48V", 0, { ch }) ? "ON" : "OFF", analog);
    set("InPhase", s.num("input.phase", 0, { ch }) ? "Invert" : "Normal", !digital);
    set("Notch1", s.num("input.notch.enable", 0, { ch, 1 }) ? "ON" : "OFF", rest);
    set("Notch1Freq", std::to_string(s.num("input.notch.freq", 0, { ch, 1 })) + " Hz", rest);
    set("DelayEn", s.num("input.delay.enable", 0, { ch }) ? "ON" : "OFF", rest);
    set("DelayTime", TenthsMs(s.num("input.delay.time", 0, { ch })), rest);
    set("Notch2", s.num("input.notch.enable", 0, { ch, 2 }) ? "ON" : "OFF", rest);
    set("Notch2Freq", std::to_string(s.num("input.notch.freq", 0, { ch, 2 })) + " Hz", rest);
    set("HpfEn", s.num("input.hpf.enable", 0, { ch }) ? "ON" : "OFF", rest);
    set("HpfFreq", std::to_string(s.num("input.hpf.freq", 40, { ch })) + " Hz", rest);
    set("CompEn", s.num("input.compress.enable", 0, { ch }) ? "ON" : "OFF", rest);
    set("CompLink", LinkText(s.num("input.compress.link", 0, { ch })), rest);
    // Attack, decay, threshold and ratio are the one input compressor, index 1,
    // shared by every channel. The Nova sends them as a 3-wide array.
    set("CompAtk", std::to_string(s.num("input.compress.attack", 5, { 1 })) + " ms", rest);
    set("CompDec", std::to_string(s.num("input.compress.decay", 100, { 1 })) + " ms", rest);
    set("CompTh", DbText(s.num("input.compress.thresh", -6, { 1 })), rest);
    set("CompRatio", RatioText(s.num("input.compress.ratio", 40, { 1 })), rest);
    for (const char* id : { "InMid", "InMid2", "InComp" }) {
        if (auto* p = page.find(id)) {
            if (p->visible != rest) {
                p->visible = rest;
                relayout = true;
            }
        }
    }
    if (relayout && page.bounds.w > 1) page.layout(page.bounds);
}

void SetInputConfigSink(Page& page, SettingSink sink, SettingGet get) {
    auto channel = [get] {
        long long c = get("input.configure", {});
        if (c < 1 || c > kInputChannels) return 1;
        return (int)c;
    };
    auto at = [](int c) { return nova::Index{ c }; };
    auto notch = [](int n) {
        return [n](int c) { return nova::Index{ c, n }; };
    };
    auto global = [](int) { return nova::Index{ 1 }; };

    if (auto* b = dynamic_cast<Button*>(page.find("InChannel"))) {
        b->click = [=] {
            int n = channel() % kInputChannels + 1;
            sink("input.configure", {}, n);
        };
    }
    auto toggle = [&](const char* id, const char* setting, auto indexFn) {
        if (auto* b = dynamic_cast<Button*>(page.find(id))) {
            b->click = [=] {
                nova::Index idx = indexFn(channel());
                sink(setting, idx, get(setting, idx) ? 0 : 1);
            };
        }
    };
    // Click steps the stored number and wraps inside the range the Nova accepts.
    // Delay and notch frequency travel as one byte, so they stop at 250.
    auto step = [&](const char* id, const char* setting, auto indexFn,
                    long long lo, long long hi, long long delta) {
        if (auto* b = dynamic_cast<Button*>(page.find(id))) {
            b->click = [=] {
                nova::Index idx = indexFn(channel());
                long long v = get(setting, idx);
                if (v < lo) v = lo;
                else {
                    v += delta;
                    if (v > hi) v = lo;
                    if (v < lo) v = hi;
                }
                sink(setting, idx, v);
            };
        }
    };

    toggle("InLevel", "input.level", at);
    toggle("In48", "input.power48V", at);
    toggle("InPhase", "input.phase", at);
    toggle("Notch1", "input.notch.enable", notch(1));
    toggle("Notch2", "input.notch.enable", notch(2));
    toggle("DelayEn", "input.delay.enable", at);
    toggle("HpfEn", "input.hpf.enable", at);
    toggle("CompEn", "input.compress.enable", at);
    step("InTrim", "input.trim.level", at, -20, 30, 1);
    step("Notch1Freq", "input.notch.freq", notch(1), 20, 250, 10);
    step("Notch2Freq", "input.notch.freq", notch(2), 20, 250, 10);
    step("DelayTime", "input.delay.time", at, 0, 250, 10);
    step("HpfFreq", "input.hpf.freq", at, 40, 230, 10);
    step("CompAtk", "input.compress.attack", global, 1, 100, 1);
    step("CompDec", "input.compress.decay", global, 5, 100, 5);
    step("CompTh", "input.compress.thresh", global, -20, 0, -1);
    step("CompRatio", "input.compress.ratio", global, 11, 200, 1);

    if (auto* b = dynamic_cast<Button*>(page.find("CompLink"))) {
        b->click = [=] {
            int c = channel();
            long long v = get("input.compress.link", { c });
            if (v < 0 || v > 3) v = 0;
            sink("input.compress.link", { c }, (v + 1) % 4);
        };
    }
}

std::unique_ptr<Page> BuildTrackNamesPage() {
    auto page = std::make_unique<Page>("Track Names");
    page->dir = LayoutDir::Column;
    page->margin = 8;
    for (int row = 0; row < 4; ++row) {
        auto line = MakeRow();
        for (int col = 0; col < 4; ++col) {
            int n = row * 4 + col + 1;
            auto b = ValueButton("Name" + std::to_string(n), "TRACK " + std::to_string(n),
                                 "Track " + std::to_string(n));
            line->add(std::move(b));
        }
        page->add(std::move(line));
    }
    return page;
}

void SyncTrackNamesPage(Page& page, const AppModel& m) {
    for (int i = 1; i <= 16; ++i) {
        auto* b = dynamic_cast<Button*>(page.find("Name" + std::to_string(i)));
        if (!b) continue;
        b->title = "TRACK " + std::to_string(i);
        b->showValue = true;
        b->value = m.trackName[i - 1].empty() ? "Track " + std::to_string(i) : m.trackName[i - 1];
    }
}
