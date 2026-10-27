#include "pages.h"
#include "painter.h"
#include "settings.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <vector>

namespace {

const Color kOutputMix = Color::rgb(0x32CD32);  // limegreen
const Color kTrackMix = Color::rgb(0x4169E1);   // royalblue
const Color kPlum = Color::rgb(0xDDA0DD);
const Color kPanelBg = Color::rgb(0x3a3a3a);

struct MixFlags {
    bool on = false;
    bool post = false;
    bool invert = false;
};

class MixPage;
class MixCell;
class AuxCell;

std::unique_ptr<Panel> Column(float margin = 4) {
    auto p = std::make_unique<Panel>(LayoutDir::Column);
    p->style = "none";
    p->margin = margin;
    return p;
}

std::unique_ptr<Panel> Row(float margin = 4) {
    auto p = std::make_unique<Panel>(LayoutDir::Row);
    p->style = "none";
    p->margin = margin;
    return p;
}

std::unique_ptr<Panel> Spacer(float frac = 0) {
    auto s = std::make_unique<Panel>();
    s->style = "none";
    if (frac > 0) s->fixedFrac = frac;
    return s;
}

std::unique_ptr<Label> Caption(const std::string& text, int maxPx, bool bold = true,
                               const std::string& align = "center") {
    auto l = std::make_unique<Label>();
    l->text = text;
    l->align = align;
    l->style = "none";
    l->bold = bold;
    l->maxPx = maxPx;
    return l;
}

void DrawSine(Painter& p, const Rect& g, Color c) {
    std::vector<std::pair<float, float>> pts;
    for (int i = 0; i <= 16; ++i) {
        float t = i / 16.f;
        pts.push_back({ g.x + t * g.w, g.vcenter() - std::sin(t * 6.2831853f) * g.h * 0.32f });
    }
    p.polyline(pts, c, 2);
}

void DrawPulse(Painter& p, const Rect& g, Color c) {
    float lo = g.y + g.h * 0.8f;
    float hi = g.y + g.h * 0.2f;
    p.polyline({
        { g.x, lo }, { g.x + g.w * 0.25f, lo }, { g.x + g.w * 0.25f, hi },
        { g.x + g.w * 0.75f, hi }, { g.x + g.w * 0.75f, lo }, { g.right(), lo },
    }, c, 2);
}

void DrawFader(Painter& p, const Rect& g, Color c) {
    float cx = g.hcenter();
    p.fillRect(Rect{ cx - 1, g.y + g.h * 0.1f, 2, g.h * 0.8f }, c);
    p.fillRect(Rect{ g.x + g.w * 0.2f, g.vcenter() - g.h * 0.09f, g.w * 0.6f, g.h * 0.18f }, c);
}

class MixPage : public Page {
public:
    MixPage(const std::string& name, const std::string& bus, int busCount, Color mixColor)
        : Page(name), bus(bus), busCount(busCount), mixColor(mixColor),
          assign(busCount), aux(busCount) {}

    std::string bus;
    std::string prefix;             // "outputmix" or "trackmix"
    int busCount;
    Color mixColor;
    bool post = false;
    bool invert = false;
    int which = 0;
    int preset = 1;
    SettingSink sink;
    std::vector<std::map<std::string, MixFlags>> assign;
    std::vector<std::map<std::string, int>> aux;
    Label* title = nullptr;
    Button* faderButton = nullptr;
    Button* phaseButton = nullptr;
    Control* panelAB = nullptr;
    Control* panelC = nullptr;
    std::vector<Button*> selectCells;
    std::vector<MixCell*> cells;
    std::vector<AuxCell*> auxes;

    void sync(const nova::SettingStore& s);

    void select(int idx) {
        which = idx;
        if (title) title->text = bus + " " + std::to_string(idx + 1);
        for (size_t i = 0; i < selectCells.size(); ++i)
            selectCells[i]->style = ((int)i == idx) ? "button.selected" : "button";
        if (panelC) {
            const bool alt = idx >= 4;
            panelAB->visible = !alt;
            panelC->visible = alt;
            if (bounds.w > 0) layout(bounds);
        }
    }
};

// AI: 1-based source index. The wire value is this minus 1, matching
// Reference/NovaSide_v5.23/PC_GUI_OFFSETS.h: analog 0, return 4, AES 6, RX 14,
// output 22, track 30. SetCrosspointFromPC reads those same ranges.
// Leave out the old 4 phantom digitals. The firmware skip is a commented j += 4.
static int SourceIndex(char kind, int num) {
    switch (kind) {
    case 'A': return num;       // 1..4
    case 'R': return 4 + num;   // 5..6
    case 'D': return 6 + num;   // 7..14
    case 'X': return 14 + num;  // 15..22
    case 'O': return 22 + num;  // 23..30
    case 'T': return 30 + num;  // 31..46
    }
    return 0;
}

static MixFlags FlagsFrom(long long v) {
    MixFlags f;
    f.on = (v & 0x01) != 0;
    f.post = (v & 0x02) != 0;     // fader bit: post-fade
    f.invert = (v & 0x04) != 0;   // phase bit
    return f;
}

class MixCell : public Button {
public:
    MixCell(MixPage* page, char kind, int num)
        : page_(page), kind_(kind), num_(num), source_(SourceIndex(kind, num)) {
        style = "button.cell";
        background = page->mixColor;
    }

    int source() const { return source_; }
    std::string key() const { return std::string(1, kind_) + std::to_string(num_); }

    void onClick() override {
        MixFlags& f = flags();
        bool post = page_->post;
        bool inv = page_->invert;
        if (isOutput()) { post = false; inv = false; }
        if (f.on && !isOutput() && (f.post != post || f.invert != inv)) {
            f.post = post;
            f.invert = inv;
        } else if (!f.on) {
            f.on = true;
            f.post = post;
            f.invert = inv;
        } else {
            f = MixFlags{};
        }
        if (page_->sink)
            page_->sink(page_->prefix + ".matrix",
                        { page_->preset, page_->which + 1, source_ },
                        (f.on ? 1 : 0) | (f.post ? 2 : 0) | (f.invert ? 4 : 0));
    }

    void onPaintContents(Painter& p) override {
        const MixFlags& f = flags();
        const bool small = !f.on;
        Rect r = bounds.shrink(5, 3);
        Rect body = small ? Rect{ r.x, r.y, r.w, r.h * 0.4f } : r;
        const Color ink = small ? Color{ 255, 255, 255 } : Color{ 0, 0, 0 };

        float x = body.x;
        if (kind_ == 'A' || kind_ == 'D') {
            float gs = std::min(body.h * (small ? 1.f : 0.6f), body.w * 0.3f);
            Rect g{ x + 2, body.y + (body.h - gs) * 0.5f, gs, gs };
            if (kind_ == 'A') DrawSine(p, g, ink);
            else DrawPulse(p, g, ink);
            x += gs + 4;
        }
        Rect tr{ x, body.y, body.right() - x, body.h };
        if (f.on && !isOutput() && !f.post) {
            float fw = body.w * 0.18f;
            DrawFader(p, Rect{ body.right() - fw, body.y + body.h * 0.15f, fw, body.h * 0.7f }, ink);
            tr.w -= fw;
        }
        const std::string text = label();
        if (small)
            p.setFont(std::min(16, std::max(10, (int)(body.h * 0.8f))), true);
        else
            p.setFont(std::min(30, std::max(12, (int)(body.h * 0.5f))), true);
        const bool left = small || kind_ == 'A' || kind_ == 'D';
        p.textIn(text, tr, left ? "left" : "center", true, ink);
        if (f.on && f.invert) {
            wxSize sz = p.textBounds(text);
            float tx = left ? tr.x + 4 : tr.x + (tr.w - sz.x) * 0.5f;
            float ty = tr.y + (tr.h - sz.y) * 0.5f;
            p.fillRect(Rect{ tx, ty, (float)sz.x, 2 }, ink);
        }
    }

private:
    MixPage* page_;
    char kind_;
    int num_;
    int source_;

    bool isOutput() const { return kind_ == 'O'; }
    MixFlags& flags() const {
        return page_->assign[page_->which][std::string(1, kind_) + std::to_string(num_)];
    }
    std::string label() const {
        const std::string n = std::to_string(num_);
        switch (kind_) {
        case 'A': case 'D': return n;
        case 'R': return "RET" + n;
        case 'X': return "RX" + n;
        case 'O': return "OUT" + n;
        case 'T': return "TRK" + n;
        }
        return n;
    }
};

class AuxCell : public Button {
public:
    AuxCell(MixPage* page, std::string key, std::string setting, std::vector<std::string> values)
        : page_(page), key_(std::move(key)), setting_(std::move(setting)), values_(std::move(values)) {
        style = "button.cell";
        background = Color{ 0, 0, 0 };
    }
    const std::string& key() const { return key_; }
    const std::string& setting() const { return setting_; }
    void onClick() override {
        int& v = page_->aux[page_->which][key_];
        v = (v + 1) % (int)values_.size();
        if (page_->sink)
            page_->sink(page_->prefix + "." + setting_, { page_->preset, page_->which + 1 }, v);
    }
    void onPaintContents(Painter& p) override {
        auto it = page_->aux[page_->which].find(key_);
        int v = it == page_->aux[page_->which].end() ? 0 : it->second;
        if (v < 0 || v >= (int)values_.size()) v = 0;
        const std::string& s = values_[v];
        if (s.empty()) return;
        Rect r = bounds.shrink(4, 2);
        p.setFont(std::min(24, std::max(11, (int)(r.h * 0.6f))), true);
        p.textIn(s, r, "center", true, NamedColor("gold"));
    }

private:
    MixPage* page_;
    std::string key_;
    std::string setting_;
    std::vector<std::string> values_;
};

std::unique_ptr<Panel> ArtMatrix(MixPage* page, const char* art) {
    auto grid = Column(2);
    std::map<char, int> count;
    std::vector<std::string> rows(1);
    for (const char* c = art; *c; ++c) {
        if (*c == '|') rows.emplace_back();
        else rows.back().push_back(*c);
    }
    size_t cols = 0;
    for (auto& r : rows) cols = std::max(cols, r.size());
    for (auto& r : rows) {
        auto line = Row(2);
        for (size_t i = 0; i < cols; ++i) {
            char k = i < r.size() ? r[i] : '-';
            if (k == '-') {
                line->add(Spacer());
                continue;
            }
            auto cell = std::make_unique<MixCell>(page, k, ++count[k]);
            page->cells.push_back(cell.get());
            line->add(std::move(cell));
        }
        grid->add(std::move(line));
    }
    return grid;
}

struct AuxSpec {
    const char* title;
    const char* setting;
    std::vector<std::string> values;
};

std::unique_ptr<Panel> AuxGroup(MixPage* page, const std::vector<AuxSpec>& specs) {
    auto outer = Column(0);
    outer->add(Spacer(0.1f));
    auto row = Row(1);
    for (const AuxSpec& a : specs) {
        auto cell = Column(1);
        cell->style = "cell.panel";
        cell->background = Color::rgb(0x101010);
        auto label = Caption(a.title, 14);
        label->fixedFrac = 0.4f;
        cell->add(std::move(label));
        auto aux = std::make_unique<AuxCell>(page, a.title, a.setting, a.values);
        page->auxes.push_back(aux.get());
        cell->add(std::move(aux));
        row->add(std::move(cell));
    }
    outer->add(std::move(row));
    outer->add(Spacer(0.1f));
    return outer;
}

std::unique_ptr<Button> SideButton(const std::string& title, std::vector<std::string> values,
                                   std::function<void(int)> onChange) {
    auto b = std::make_unique<Button>();
    b->title = title;
    b->value = values[0];
    b->showValue = true;
    b->centerTitle = true;
    Button* raw = b.get();
    b->click = [raw, values, onChange] {
        int i = 0;
        for (int k = 0; k < (int)values.size(); ++k)
            if (values[k] == raw->value) i = k;
        i = (i + 1) % (int)values.size();
        raw->value = values[i];
        if (onChange) onChange(i);
    };
    return b;
}

struct MixSpec {
    const char* name;
    const char* prefix;
    const char* bus;
    const char* selectWord;
    int busCount;
    int selectRows;
    Color color;
    std::vector<AuxSpec> aux;
    const char* artA;
    const char* artB;
    const char* artC;
};

std::unique_ptr<Page> BuildMix(const MixSpec& spec) {
    auto page = std::make_unique<MixPage>(spec.name, spec.bus, spec.busCount, spec.color);
    MixPage* mp = page.get();
    mp->prefix = spec.prefix;
    page->dir = LayoutDir::Column;
    page->margin = 6;

    auto main = Row(6);

    auto side = Column(6);
    side->fixedFrac = 0.2f;
    side->add(Spacer());
    auto fader = SideButton("FADER", { "Pre", "Post" }, [mp](int i) {
        mp->post = i != 0;
        if (mp->sink) mp->sink(mp->prefix + ".fader", {}, i);
    });
    mp->faderButton = fader.get();
    side->add(std::move(fader));
    auto phase = SideButton("PHASE", { "Normal", "Invert" }, [mp](int i) {
        mp->invert = i != 0;
        if (mp->sink) mp->sink(mp->prefix + ".phase", {}, i);
    });
    mp->phaseButton = phase.get();
    side->add(std::move(phase));
    main->add(std::move(side));

    auto frame = Column(8);
    frame->style = "panel.bg";
    frame->background = kPanelBg;

    auto mix = Column(8);
    mix->style = "panel.gradient";
    mix->background = spec.color;

    auto head = Row(4);
    head->fixedFrac = 0.27f;
    auto title = Caption(std::string(spec.bus) + " 1", 34, true, "left");
    title->fixedFrac = 0.3f;
    mp->title = title.get();
    head->add(std::move(title));
    head->add(Spacer(0.12f));
    head->add(AuxGroup(mp, spec.aux));
    mix->add(std::move(head));

    auto ab = Row(4);
    auto a = ArtMatrix(mp, spec.artA);
    a->fixedFrac = 0.33f;
    ab->add(std::move(a));
    ab->add(ArtMatrix(mp, spec.artB));
    mp->panelAB = ab.get();
    mix->add(std::move(ab));

    if (spec.artC) {
        auto c = ArtMatrix(mp, spec.artC);
        c->visible = false;
        mp->panelC = c.get();
        mix->add(std::move(c));
    }

    frame->add(std::move(mix));
    main->add(std::move(frame));
    page->add(std::move(main));

    auto pick = Row(6);
    pick->style = "panel.bg";
    pick->background = kPanelBg;
    pick->fixedFrac = 0.25f;
    auto cap = Column(4);
    cap->fixedFrac = 0.2f;
    cap->add(Caption(spec.selectWord, 26));
    cap->add(Caption("Select", 26));
    pick->add(std::move(cap));
    auto cells = Column(3);
    const int perRow = spec.busCount / spec.selectRows;
    for (int r = 0; r < spec.selectRows; ++r) {
        auto line = Row(3);
        for (int i = 0; i < perRow; ++i) {
            int idx = r * perRow + i;
            auto b = std::make_unique<Button>();
            b->title = std::to_string(idx + 1);
            b->maxFont = 22;
            b->click = [mp, idx] {
                mp->select(idx);
                if (mp->sink) mp->sink(mp->prefix + ".which", {}, idx);
            };
            mp->selectCells.push_back(b.get());
            line->add(std::move(b));
        }
        cells->add(std::move(line));
    }
    pick->add(std::move(cells));
    page->add(std::move(pick));

    mp->select(0);
    return page;
}

void MixPage::sync(const nova::SettingStore& s) {
    if (s.has(prefix + ".preset")) preset = (int)s.num(prefix + ".preset", preset);
    if (s.has(prefix + ".fader") && faderButton) {
        post = s.num(prefix + ".fader", 0) != 0;
        faderButton->value = post ? "Post" : "Pre";
    }
    if (s.has(prefix + ".phase") && phaseButton) {
        invert = s.num(prefix + ".phase", 0) != 0;
        phaseButton->value = invert ? "Invert" : "Normal";
    }
    if (s.has(prefix + ".which")) {
        int w = (int)s.num(prefix + ".which", which);
        if (w < 0) w = 0;
        if (w >= busCount) w = busCount - 1;
        if (w != which) select(w);
    }
    if (s.has(prefix + ".matrix")) {
        for (MixCell* cell : cells) {
            for (int bus = 0; bus < busCount; ++bus)
                assign[bus][cell->key()] = FlagsFrom(
                    s.num(prefix + ".matrix", 0, { preset, bus + 1, cell->source() }));
        }
    }
    for (AuxCell* aux : auxes) {
        const std::string name = prefix + "." + aux->setting();
        if (!s.has(name)) continue;
        for (int bus = 0; bus < busCount; ++bus)
            this->aux[bus][aux->key()] = (int)s.num(name, 0, { preset, bus + 1 });
    }
}

// hpmix.tracks / outputs / returns menu (remote/settings.lua).
static const char* kHpMenu[] = { "", "L", "R", "LR", "/L", "/R", "/LR", "L/R", "/L/R" };
static const int kHpMenuCount = 9;
static bool HasPhase(const char* s) { return std::string(s).find('/') != std::string::npos; }

class HpPage;

class HpCell : public Button {
public:
    HpCell(Color bg, HpPage* page, std::string setting, int chan)
        : page_(page), setting_(std::move(setting)), chan_(chan) {
        style = "button.cell";
        background = bg;
    }
    int chan() const { return chan_; }
    const std::string& setting() const { return setting_; }
    void setMenu(int v) {
        if (v < 0 || v >= kHpMenuCount) v = 0;
        menu_ = v;
        value = kHpMenu[v];
    }
    void onClick() override;
    void onPaintContents(Painter& p) override {
        if (value.empty()) return;
        Rect r = bounds.shrink(4, 2);
        p.setFont(std::min(26, std::max(12, (int)(r.h * 0.6f))), true);
        p.textIn(value, r, "center", true, Color{ 255, 255, 255 });
    }

private:
    HpPage* page_;
    std::string setting_;
    int chan_;
    int menu_ = 0;
};

class HpPage : public Page {
public:
    HpPage() : Page("Headphone Mix") {}
    bool phase = false;
    int preset = 1;
    SettingSink sink;
    Button* presetButton = nullptr;
    Button* enableButton = nullptr;
    Button* phaseButton = nullptr;
    std::vector<HpCell*> cells;
    void sync(const nova::SettingStore& s);
};

void HpCell::onClick() {
    const bool phase = page_->phase;
    int v = menu_;
    const int orig = v;
    const char* text = kHpMenu[v];
    if (text[0] && HasPhase(text) != phase) {
        std::string next;
        for (char c : std::string(text)) {
            if (c == '/') continue;
            if (phase) next += '/';
            next += c;
        }
        v = 0;
        for (int i = 0; i < kHpMenuCount; ++i)
            if (next == kHpMenu[i]) v = i;
    } else {
        for (int step = 0; step < kHpMenuCount; ++step) {
            v = (v + 1) % kHpMenuCount;
            text = kHpMenu[v];
            if (v == 0 || !text[0] || HasPhase(text) == phase) break;
        }
        if (v == orig) v = (orig + 1) % kHpMenuCount;
    }
    setMenu(v);
    if (page_->sink) page_->sink(setting_, { page_->preset, chan_ }, menu_);
}

void HpPage::sync(const nova::SettingStore& s) {
    if (s.has("hpmix.preset")) {
        preset = (int)s.num("hpmix.preset", preset);
        if (presetButton) presetButton->value = std::to_string(preset);
    }
    if (s.has("hpmix.phase")) {
        phase = s.num("hpmix.phase", 0) != 0;
        if (phaseButton) phaseButton->value = phase ? "Invert" : "Normal";
    }
    if (s.has("hpmix.enable") && enableButton)
        enableButton->value = s.num("hpmix.enable", 0, { preset }) ? "ON" : "OFF";
    for (HpCell* cell : cells) {
        if (s.has(cell->setting()))
            cell->setMenu((int)s.num(cell->setting(), 0, { preset, cell->chan() }));
    }
}

} // namespace

std::unique_ptr<Page> BuildOutputMixPage() {
    return BuildMix({
        "Output Mix", "outputmix", "OUT", "Output", 8, 1, kOutputMix,
        { { "COMP", "comp", { "OUT", "IN" } }, { "TONE", "tone", { "", "X" } },
          { "SLATE", "slate", { "", "X" } }, { "TALK1", "com1", { "", "X" } } },
        "AA|AA|RR", "DDDD|DDDD|XXXX|XXXX", "TTTTTTTT|TTTTTTTT|------RR|OOOO",
    });
}

std::unique_ptr<Page> BuildTrackMixPage() {
    return BuildMix({
        "Track Mix", "trackmix", "TRACK", "Track", 16, 2, kTrackMix,
        { { "ISO ATT", "neverclip", { "", "X" } }, { "COMP", "comp", { "OUT", "IN" } },
          { "TONE", "tone", { "", "X" } }, { "SLATE", "slate", { "", "X" } } },
        "AA|AA|RR", "DDDD|DDDD|XXXX|XXXX|OOOO", nullptr,
    });
}

std::unique_ptr<Page> BuildHeadphoneMixPage() {
    auto page = std::make_unique<HpPage>();
    HpPage* hp = page.get();
    page->dir = LayoutDir::Row;
    page->margin = 6;

    auto side = Column(6);
    side->fixedFrac = 0.2f;
    auto preset = SideButton("PRESET",
                             { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" },
                             [hp](int i) {
                                 hp->preset = i + 1;
                                 if (hp->sink) hp->sink("hpmix.preset", {}, hp->preset);
                             });
    hp->presetButton = preset.get();
    side->add(std::move(preset));
    auto enable = SideButton("ENABLE", { "ON", "OFF" }, [hp](int i) {
        if (hp->sink) hp->sink("hpmix.enable", { hp->preset }, i == 0 ? 1 : 0);
    });
    hp->enableButton = enable.get();
    side->add(std::move(enable));
    side->add(Spacer());
    auto phase = SideButton("PHASE", { "Normal", "Invert" }, [hp](int i) {
        hp->phase = i != 0;
        if (hp->sink) hp->sink("hpmix.phase", {}, i);
    });
    hp->phaseButton = phase.get();
    side->add(std::move(phase));
    page->add(std::move(side));

    auto frame = Column(6);
    frame->style = "panel.bg";
    frame->background = kPanelBg;
    const char* art = "TTTTTTTT|TTTTTTTT|OOOO--RR";
    std::map<char, int> count;
    std::vector<std::string> rows(1);
    for (const char* c = art; *c; ++c) {
        if (*c == '|') rows.emplace_back();
        else rows.back().push_back(*c);
    }
    for (auto& r : rows) {
        auto line = Row(3);
        for (char k : r) {
            if (k == '-') {
                line->add(Spacer());
                continue;
            }
            int n = ++count[k];
            const char* word = k == 'T' ? "TRACK " : k == 'O' ? "OUT " : "RET ";
            Color bg = k == 'T' ? kPlum : k == 'O' ? kOutputMix : Color{ 0, 0, 0 };
            auto cell = Column(2);
            cell->style = "cell.panel";
            cell->background = Color::rgb(0x707070);
            auto label = Caption(word + std::to_string(n), 14);
            label->fixedFrac = 0.4f;
            cell->add(std::move(label));
            const char* setting = k == 'T' ? "hpmix.tracks" : k == 'O' ? "hpmix.outputs" : "hpmix.returns";
            auto button = std::make_unique<HpCell>(bg, hp, setting, n);
            hp->cells.push_back(button.get());
            cell->add(std::move(button));
            line->add(std::move(cell));
        }
        frame->add(std::move(line));
    }
    page->add(std::move(frame));
    return page;
}

void SetMixSink(Page& outputMix, Page& trackMix, Page& headphoneMix, SettingSink sink) {
    if (auto* m = dynamic_cast<MixPage*>(&outputMix)) m->sink = sink;
    if (auto* m = dynamic_cast<MixPage*>(&trackMix)) m->sink = sink;
    if (auto* m = dynamic_cast<HpPage*>(&headphoneMix)) m->sink = sink;
}

void SyncMixPages(Page& outputMix, Page& trackMix, Page& headphoneMix, const nova::SettingStore& s) {
    if (auto* m = dynamic_cast<MixPage*>(&outputMix)) m->sync(s);
    if (auto* m = dynamic_cast<MixPage*>(&trackMix)) m->sync(s);
    if (auto* m = dynamic_cast<HpPage*>(&headphoneMix)) m->sync(s);
}
