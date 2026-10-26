#include "pages.h"
#include "meter.h"
#include "painter.h"
#include <algorithm>

static std::unique_ptr<Button> MakeButton(const std::string& id, const std::string& title,
                                          const std::string& value = {}, float frac = 0) {
    auto b = std::make_unique<Button>();
    b->id = id;
    b->title = title;
    b->value = value;
    if (frac > 0) b->fixedFrac = frac;
    return b;
}

// Matches the Lua Button1Line used throughout the Home page.
static std::unique_ptr<Button> MakeValueButton(const std::string& id, const std::string& title,
                                               const std::string& value) {
    auto b = MakeButton(id, title, value);
    b->showValue = true;
    b->button1Line = true;
    return b;
}

static std::unique_ptr<Panel> MakeRow(float frac = 0) {
    auto row = std::make_unique<Panel>(LayoutDir::Row);
    row->style = "none";
    row->margin = 0;
    if (frac > 0) row->fixedFrac = frac;
    return row;
}

// TLOC / Elapsed share one button, as in the original 'Drive' button.
class DriveButton : public Button {
public:
    std::string remaining = "000:00:00";
    std::string elapsed = "000:00:00";

    void onPaintContents(Painter& p) override {
        Rect r = bounds.shrink(bounds.w * 0.2f, 0);
        const float rowH = r.h * 0.5f;
        auto line = [&](float y, const std::string& label, const std::string& val, Color valColor) {
            Rect lr{ r.x, y, r.w * 0.4f, rowH };
            Rect vr{ r.x + r.w * 0.4f, y, r.w * 0.6f, rowH };
            p.setFont(std::max(10, (int)(rowH * 0.8f)), true);
            p.textIn(label, lr, "center", true, Color{ 255, 255, 255 });
            p.setFont(std::max(10, (int)(rowH * 0.62f)));
            p.textIn(val, vr, "left", true, valColor);
        };
        line(r.y, "TLOC", remaining, Color{ 255, 255, 255 });
        line(r.y + rowH, "Elapsed", elapsed, NamedColor("darkgray"));
    }
};

static std::unique_ptr<Button> MakeTab(const std::string& id, const std::string& label, float frac = 0) {
    auto b = std::make_unique<Button>();
    b->id = id;
    b->style = "tab";
    b->background = Color::rgb(0x555555);
    b->color = NamedColor("lightgray");
    auto bar = label.find('|');
    if (label.rfind("icon:", 0) == 0) {
        b->graphic = label.substr(5);
        b->color = Color{ 255, 255, 255 };
    } else if (bar == std::string::npos) {
        b->title = label;
    } else {
        b->title = label.substr(0, bar);
        b->value = label.substr(bar + 1);
        b->showValue = true;
    }
    if (frac > 0) b->fixedFrac = frac;
    return b;
}

std::unique_ptr<Panel> BuildHeader() {
    auto header = std::make_unique<Panel>(LayoutDir::Row);
    header->id = "Header";
    header->style = "bar";
    header->background = Color::rgb(0x555555);
    header->margin = 0;
    header->add(MakeTab("TabHome", "icon:home", 0.12f));
    header->add(MakeTab("TabBack", "icon:reply", 0.12f));
    header->add(MakeTab("TabOutputMix", "Output|Mix"));
    header->add(MakeTab("TabTrackMix", "Track|Mix"));
    header->add(MakeTab("TabRecord", "Record|Enables"));
    header->add(MakeTab("TabInput", "Input|Config"));
    header->add(MakeTab("TabHeadphone", "Headphone|Mix"));
    header->add(MakeTab("TabMainMenu", "Main|Menu"));
    return header;
}

std::unique_ptr<Label> BuildStatusLine() {
    auto status = std::make_unique<Label>();
    status->id = "StatusLine";
    status->text = "Offline";
    status->align = "left";
    status->style = "fill";
    status->background = Color::rgb(0x3c3c3c);
    status->color = Color{ 210, 210, 210 };
    status->maxPx = 13;
    return status;
}

std::unique_ptr<Page> BuildPlaceholderPage(const std::string& title) {
    auto page = std::make_unique<Page>(title);
    page->dir = LayoutDir::Column;
    auto label = std::make_unique<Label>();
    label->text = title;
    label->align = "center";
    label->style = "none";
    label->fixedFrac = 0.12f;
    label->color = NamedColor("lightgray");
    page->add(std::move(label));
    return page;
}

std::unique_ptr<Page> BuildHomePage() {
    // Proportions follow remote/pages/Home.lua: fixed rows take their h,
    // the rest share what is left.
    auto page = std::make_unique<Page>("Home");
    page->dir = LayoutDir::Column;
    page->style = "fill";
    page->background = Color::rgb(0x505050);
    page->margin = 1;

    auto body = std::make_unique<Panel>(LayoutDir::Row);
    body->style = "none";
    body->margin = 0;
    body->weight = 1;

    auto left = std::make_unique<Panel>(LayoutDir::Column);
    left->style = "none";
    left->margin = 1;

    auto top = MakeRow(0.15f);
    auto stop = MakeButton("StatusButton", "STOP");
    stop->fixedFrac = 0.50f;
    top->add(std::move(stop));
    auto hp = MakeValueButton("Headphone", "", "HP 1");
    hp->graphic = "headphones";
    top->add(std::move(hp));
    top->add(MakeValueButton("Battery", "Battery", "---"));
    left->add(std::move(top));

    auto tc = std::make_unique<TimecodeView>();
    tc->id = "timecode";
    left->add(std::move(tc));

    auto drive = std::make_unique<DriveButton>();
    drive->id = "Drive";
    drive->fixedFrac = 0.15f;
    left->add(std::move(drive));

    auto row3 = MakeRow(0.10f);
    row3->add(MakeValueButton("SR", "SR", "48000"));
    row3->add(MakeValueButton("FR", "FR", "23.98"));
    row3->add(MakeValueButton("PreRecord", "Pre Record", "OFF"));
    left->add(std::move(row3));

    auto row4 = MakeRow();
    row4->add(MakeValueButton("Cur", "Cur", "---"));
    row4->add(MakeValueButton("Segs", "Segs", "---"));
    row4->add(MakeValueButton("Folder", "Folder", "---"));
    left->add(std::move(row4));

    auto row5 = MakeRow();
    auto scene = MakeValueButton("Scene", "Scene", "SCENE 1");
    scene->fixedFrac = 0.6f;
    row5->add(std::move(scene));
    row5->add(MakeValueButton("Take", "Take", "1"));
    left->add(std::move(row5));

    left->add(MakeValueButton("Note", "Note", "Nova Touch"));

    auto right = std::make_unique<Panel>(LayoutDir::Column);
    right->style = "none";
    right->margin = 1;

    auto meters = std::make_unique<Panel>(LayoutDir::Column);
    meters->id = "MeterPanel";
    meters->style = "fill";
    meters->background = Color::rgb(0x181818);
    meters->margin = 1;
    auto scale = std::make_unique<MeterScale>();
    scale->fixedFrac = 1.f / (1.4f * 16);
    meters->add(std::move(scale));
    for (int i = 1; i <= 8; ++i) {
        auto m = std::make_unique<Meter>();
        m->id = "Meter" + std::to_string(i);
        m->source = "meters.bus";
        m->tag = i;
        m->label = "TRACK " + std::to_string(i);
        m->strikethru = true;
        meters->add(std::move(m));
    }
    right->add(std::move(meters));
    auto sel = MakeButton("MeterSelect", "TRACKS 1-8");
    sel->fixedFrac = 0.10f;
    sel->color = NamedColor("gold");
    right->add(std::move(sel));

    body->add(std::move(left));
    body->add(std::move(right));
    page->add(std::move(body));

    auto status = std::make_unique<Label>();
    status->id = "HomeStatus";
    status->style = "fill";
    status->background = Color{ 255, 0, 0 };
    status->color = Color{ 255, 255, 255 };
    status->mono = true;
    status->maxPx = 30;
    status->fixedFrac = 0.06f;
    status->text = "Offline";
    page->add(std::move(status));
    return page;
}

std::unique_ptr<Page> BuildMetersPage() {
    auto page = std::make_unique<Page>("Meters");
    page->dir = LayoutDir::Column;
    page->margin = 4;

    auto title = std::make_unique<Label>();
    title->text = "METERS";
    title->align = "center";
    title->fixedFrac = 0.07f;
    title->style = "none";
    page->add(std::move(title));

    auto bridge = std::make_unique<Panel>(LayoutDir::Row);
    bridge->id = "MeterBridge";
    bridge->style = "none";
    for (int i = 1; i <= 8; ++i) {
        auto col = std::make_unique<Panel>(LayoutDir::Column);
        col->style = "none";
        col->margin = 2;
        auto name = std::make_unique<Button>();
        name->id = "InputName" + std::to_string(i);
        name->title = "IN " + std::to_string(i);
        name->fixedFrac = 0.12f;
        col->add(std::move(name));
        auto m = std::make_unique<Meter>();
        m->source = "meters.bridge";
        m->tag = i;
        m->vertical = true;
        col->add(std::move(m));
        bridge->add(std::move(col));
    }
    page->add(std::move(bridge));
    return page;
}

static Button* AsButton(Control* c) { return dynamic_cast<Button*>(c); }

void SyncHomePage(Page& page, const AppModel& m) {
    if (auto* b = AsButton(page.find("StatusButton"))) {
        if (m.transport == "record") {
            b->title = "REC";
            b->color = NamedColor("orangered");
            b->background = Color::rgb(0x300000);
        } else if (m.transport == "play") {
            b->title = "PLAY";
            b->color = Color::rgb(0x20F020);
            b->background = Color::rgb(0x003000);
        } else {
            b->title = "STOP";
            b->color = Color{ 255, 255, 255 };
            b->background = Color{ 0, 0, 0 };
        }
    }
    if (auto* tc = dynamic_cast<TimecodeView*>(page.find("timecode"))) {
        tc->text = m.timecode;
        if (m.transport == "record") {
            tc->background = Color::rgb(0x300000);
            tc->color = NamedColor("orangered");
        } else if (m.transport == "play") {
            tc->background = Color::rgb(0x003000);
            tc->color = Color::rgb(0x20F020);
        } else {
            tc->background = Color::rgb(0x000020);
            tc->color = Color::rgb(0xF0F0FF);
        }
    }
    if (auto* b = AsButton(page.find("Headphone"))) b->value = m.headphone;
    if (auto* b = AsButton(page.find("Battery"))) b->value = m.battery;
    if (auto* d = dynamic_cast<DriveButton*>(page.find("Drive"))) {
        d->remaining = m.remaining;
        d->elapsed = m.elapsed;
    }
    if (auto* s = dynamic_cast<Label*>(page.find("HomeStatus"))) {
        s->text = m.online ? m.status : "Offline";
        s->background = m.online ? Color::rgb(0x606060) : Color{ 255, 0, 0 };
    }
    if (auto* b = AsButton(page.find("SR"))) b->value = m.sampleRate;
    if (auto* b = AsButton(page.find("FR"))) b->value = m.frameRate;
    if (auto* b = AsButton(page.find("PreRecord"))) b->value = m.preroll;
    if (auto* b = AsButton(page.find("Cur"))) b->value = m.storeCurrent;
    if (auto* b = AsButton(page.find("Segs"))) b->value = m.storeSegment;
    if (auto* b = AsButton(page.find("Folder"))) b->value = m.storeFolder;
    if (auto* b = AsButton(page.find("Scene"))) b->value = m.scene;
    if (auto* b = AsButton(page.find("Take"))) b->value = m.take;
    if (auto* b = AsButton(page.find("Note"))) b->value = m.note;
    if (auto* b = AsButton(page.find("MeterSelect"))) {
        if (m.meterSource == "OUT")
            b->title = "OUT 1-8";
        else
            b->title = "TRACKS 1-" + std::to_string(m.meterCount);
    }

    const std::string src = (m.meterSource == "OUT") ? "meters.out" : "meters.bus";
    for (int i = 1; i <= 8; ++i) {
        if (auto* meter = dynamic_cast<Meter*>(page.find("Meter" + std::to_string(i)))) {
            meter->source = src;
            meter->visible = i <= m.meterCount;
            meter->strikethru = (src != "meters.out") && !m.trackArmed[i - 1];
            if (src == "meters.out")
                meter->label = "OUT " + std::to_string(i);
            else
                meter->label = m.trackName[i - 1].empty() ? "TRACK " + std::to_string(i) : m.trackName[i - 1];
        }
    }
}

void SyncMetersPage(Page& page, const AppModel&) {
    (void)page;
}
