#include "canvas.h"
#include "meter.h"
#include "pages.h"
#include "protocol.h"
#include "serial.h"
#include "settings.h"

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/textdlg.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <memory>
#include <vector>

// AI: On connect, send cVersion "PCnovaT-<kAppVersion>#01.01", answer an empty
// cVersion, then send an empty cRequest so the Nova dumps every setting. Skip
// wxApp::OnInit(); its parser rejects --port, and MainFrame reads argv itself.
// Record arm, sample rate, frame rate, preroll, mix cells, input controls, tone,
// and track names send real settings. Change one only when the user asked to
// change the connected recorder.
static const char* const kAppVersion = "0.92.10";
static const float kCommsTimeout = 3.f;

// Generator timecode with local flywheel between packets, as remote/timecode.lua.
struct TimecodeTracker {
    int hh = 0, mm = 0, ss = 0, ff = 0;
    float bb = 0;  // 1/160ths of a frame
    int format = 4;
    bool run = false;
    bool unknown = true;
    float timeout = 0;

    void unpack(const unsigned char* b, size_t n) {
        if (n < 10) return;
        unknown = false;
        timeout = kCommsTimeout;
        if (b[0] & 0x40) return;
        hh = b[1]; mm = b[2]; ss = b[3]; ff = b[4]; bb = b[5];
        if ((b[0] & 0x3F) <= 6) format = b[0] & 0x3F;
        run = (b[0] & 0x80) == 0;
    }

    void advance(float secs) {
        if (timeout > 0) {
            timeout -= secs;
            if (timeout <= 0) {
                timeout = 0;
                run = false;
                unknown = true;
            }
        }
        if (!run) return;
        static const struct { double rate; int fps; bool df; } rates[] = {
            { 24000.0 / 1001, 24, false }, { 24, 24, false }, { 25, 25, false },
            { 30000.0 / 1001, 30, false }, { 30000.0 / 1001, 30, true },
            { 30, 30, false }, { 30, 30, true },
        };
        const auto& r = rates[format];
        bb += (float)(160.0 * secs * r.rate);
        while (bb >= 160) {
            bb -= 160;
            if (++ff >= r.fps) {
                ff = 0;
                if (++ss >= 60) {
                    ss = 0;
                    ++mm;
                    if (r.df && (mm % 10) != 0) ff = 2;
                    if (mm >= 60) {
                        mm = 0;
                        if (++hh >= 24) hh = 0;
                    }
                }
            }
        }
    }

    std::string text() const {
        if (unknown) return "--:--:--:--";
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d:%02d", hh, mm, ss, ff);
        return buf;
    }
};

class MainFrame : public wxFrame {
public:
    MainFrame();

private:
    Canvas* canvas_ = nullptr;
    wxTimer timer_;
    SerialPort serial_;
    std::string rxBuf_;
    AppModel model_;
    std::unique_ptr<Panel> header_;
    std::unique_ptr<Label> statusLine_;
    std::unique_ptr<Page> home_;
    std::unique_ptr<Page> meters_;
    std::unique_ptr<Page> outputMix_;
    std::unique_ptr<Page> trackMix_;
    std::unique_ptr<Page> recordEnables_;
    std::unique_ptr<Page> inputConfig_;
    std::unique_ptr<Page> headphoneMix_;
    std::unique_ptr<Page> mainMenu_;
    std::unique_ptr<Page> setup_;
    std::unique_ptr<Page> trackNames_;
    std::vector<Page*> history_;
    Page* current_ = nullptr;
    bool demoMeters_ = true;
    std::string portName_;
    uint64_t rxBytes_ = 0;
    uint64_t txBytes_ = 0;
    uint64_t rxErrors_ = 0;
    uint64_t rxPackets_ = 0;
    int fps_ = 0;
    int frameCount_ = 0;
    std::chrono::steady_clock::time_point lastTick_;
    std::chrono::steady_clock::time_point fpsStamp_;
    nova::SettingStore settings_;
    TimecodeTracker generator_;
    float commsTimeout_ = 0;
    FILE* log_ = nullptr;
    uint64_t meterPackets_ = 0;

    void logLine(const std::string& line);
    void send(uint8_t cmd, const std::string& data = {});
    void sendVersion();
    void handlePacket(uint8_t cmd, const std::string& data);
    void applySettings();
    void sendNumber(const std::string& name, const nova::Index& idx, long long value,
                    int fallbackKind = 1, const nova::Index& fallbackBounds = {});
    void sendText(const std::string& name, const nova::Index& idx, const std::string& value,
                  const nova::Index& fallbackBounds = {});
    void goOffline(const char* why);

    void buildMenus();
    void wireTab(const char* id, Page* page);
    void wireNav(Control* root);
    void openNamed(const std::string& name);
    void showPage(Page* page);
    void goBack();
    void highlightTabs();
    void openPort(const std::string& name);
    void onSerialBytes(const std::string& bytes);
    void onTimer(wxTimerEvent&);
    void refreshUi();
};

class App : public wxApp {
public:
    bool OnInit() override;
};

wxIMPLEMENT_APP(App);

bool App::OnInit() {
    // wxApp::OnInit would reject --port; MainFrame reads argv itself.
    SetAppName("NovaRemote");
    auto* frame = new MainFrame();
    frame->Show(true);
    return true;
}

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "NovaRemote", wxDefaultPosition, wxSize(1280, 800))
{
    SetBackgroundColour(*wxBLACK);
    canvas_ = new Canvas(this);
    SetMinClientSize(wxSize(960, 600));

    MeterBank::instance().ensure("meters.bus", 16);
    MeterBank::instance().ensure("meters.out", 8);
    MeterBank::instance().ensure("meters.bridge", 16);
    MeterBank::instance().ensure("meters.in", 16);

    home_ = BuildHomePage();
    meters_ = BuildMetersPage();
    if (auto* b = dynamic_cast<Button*>(home_->find("MeterSelect"))) {
        b->click = [this] {
            model_.meterSource = (model_.meterSource == "OUT") ? "TRACK" : "OUT";
            refreshUi();
        };
    }
    if (auto* b = dynamic_cast<Button*>(home_->find("StatusButton"))) {
        b->click = [this] { showPage(meters_.get()); };
    }

    outputMix_ = BuildOutputMixPage();
    trackMix_ = BuildTrackMixPage();
    recordEnables_ = BuildRecordPage();
    inputConfig_ = BuildInputConfigPage();
    headphoneMix_ = BuildHeadphoneMixPage();
    SetMixSink(*outputMix_, *trackMix_, *headphoneMix_,
               [this](const std::string& name, const nova::Index& idx, long long value) {
                   // Track mix is advertised as 22 sources, but outputs sit at
                   // 23-26, which is what SetCrosspointFromPC accepts.
                   nova::Index wider;
                   if (name == "trackmix.matrix") wider = {8, 16, 26};
                   sendNumber(name, idx, value, 1, wider);
               });
    SetInputConfigSink(*inputConfig_,
                       [this](const std::string& name, const nova::Index& idx, long long value) {
                           nova::Index wider;
                           if (idx.size() == 2) wider = { 22, 2 };
                           else if (name == "input.compress.attack" || name == "input.compress.decay" ||
                                    name == "input.compress.thresh" || name == "input.compress.ratio")
                               wider = { 3 };
                           else if (!idx.empty()) wider = { 22 };
                           sendNumber(name, idx, value, 1, wider);
                       },
                       [this](const std::string& name, const nova::Index& idx) {
                           return settings_.num(name, 0, idx);
                       });
    mainMenu_ = BuildMainMenuPage();
    setup_ = BuildSetupPage();
    trackNames_ = BuildTrackNamesPage();
    for (int i = 1; i <= 16; ++i) {
        if (auto* b = dynamic_cast<Button*>(recordEnables_->find("Arm" + std::to_string(i)))) {
            b->click = [this, i] {
                sendNumber("record.tracks", { i }, model_.trackArmed[i - 1] ? 0 : 1, 1, { 32 });
            };
        }
    }
    auto cycle = [this](Page* page, const char* id, const char* setting, int count, int first = 0) {
        if (auto* b = dynamic_cast<Button*>(page->find(id))) {
            b->click = [this, setting, count, first] {
                long long v = settings_.num(setting, first) + 1;
                if (v >= first + count) v = first;
                sendNumber(setting, {}, v);
            };
        }
    };
    cycle(home_.get(), "SR", "sample.rate", 4);
    cycle(home_.get(), "FR", "timecode.framerate", 7);
    cycle(home_.get(), "PreRecord", "record.preroll", 11);
    cycle(setup_.get(), "PreRecordSetup", "record.preroll", 11);
    cycle(setup_.get(), "IsoAtt", "neverclip.attenuation", 5);
    if (auto* b = dynamic_cast<Button*>(mainMenu_->find("Tone"))) {
        b->click = [this] {
            sendNumber("tone.enable", {}, settings_.num("tone.enable", 0) ? 0 : 1);
        };
    }
    if (auto* b = dynamic_cast<Button*>(setup_->find("Mp3Rec"))) {
        b->click = [this] {
            sendNumber("mp3record.mode", {}, settings_.num("mp3record.mode", 0) ? 0 : 1);
        };
    }
    if (auto* b = dynamic_cast<Button*>(setup_->find("HomeMeters"))) {
        b->click = [this] {
            long long v = settings_.num("home.meters.count", 8) + 1;
            if (v > 16) v = 8;
            sendNumber("home.meters.count", {}, v);
        };
    }
    for (int i = 1; i <= 16; ++i) {
        if (auto* b = dynamic_cast<Button*>(trackNames_->find("Name" + std::to_string(i)))) {
            b->click = [this, i] {
                std::string cur = settings_.text("track.name", "", { i });
                wxTextEntryDialog dlg(this, "Name for track " + std::to_string(i), "Track name", cur);
                if (dlg.ShowModal() == wxID_OK)
                    sendText("track.name", { i }, dlg.GetValue().ToStdString(), { 16 });
            };
        }
    }
    if (auto* b = dynamic_cast<Button*>(setup_->find("HomeMeters"))) {
        b->click = [this, b] {
            model_.meterCount = (model_.meterCount >= 16) ? 8 : model_.meterCount + 4;
            if (model_.meterCount > 16) model_.meterCount = 8;
            b->value = std::to_string(model_.meterCount);
            b->showValue = true;
            refreshUi();
        };
    }

    header_ = BuildHeader();
    statusLine_ = BuildStatusLine();
    canvas_->setChrome(header_.get(), statusLine_.get());
    wireTab("TabHome", home_.get());
    wireTab("TabOutputMix", outputMix_.get());
    wireTab("TabTrackMix", trackMix_.get());
    wireTab("TabRecord", recordEnables_.get());
    wireTab("TabInput", inputConfig_.get());
    wireTab("TabHeadphone", headphoneMix_.get());
    wireTab("TabMainMenu", mainMenu_.get());
    if (auto* b = dynamic_cast<Button*>(header_->find("TabBack")))
        b->click = [this] { goBack(); };
    wireNav(mainMenu_.get());
    wireNav(setup_.get());

    showPage(home_.get());
    buildMenus();

    wxFileName logPath(wxStandardPaths::Get().GetExecutablePath());
    logPath.SetFullName("serial.log");
    log_ = _wfopen(logPath.GetFullPath().wc_str(), L"w");

    lastTick_ = fpsStamp_ = std::chrono::steady_clock::now();
    timer_.Bind(wxEVT_TIMER, &MainFrame::onTimer, this);
    timer_.Start(50);

    // --port=COMx (or --port COMx) on the command line, otherwise the last port used.
    std::string port;
    for (int i = 1; i < wxTheApp->argc; ++i) {
        wxString a = wxTheApp->argv[i];
        if (a.StartsWith("--port=")) port = a.Mid(7).ToStdString();
        else if (a == "--port" && i + 1 < wxTheApp->argc) port = wxString(wxTheApp->argv[++i]).ToStdString();
    }
    if (port.empty()) {
        wxString last;
        if (wxConfig::Get()->Read("SerialPort", &last)) port = last.ToStdString();
    }
    if (!port.empty()) openPort(port);

    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& e) {
        timer_.Stop();
        serial_.close();
        if (log_) {
            std::fclose(log_);
            log_ = nullptr;
        }
        e.Skip();
    });
}

void MainFrame::logLine(const std::string& line) {
    if (!log_) return;
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    int ms = (int)(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000);
    char stamp[32];
    std::strftime(stamp, sizeof(stamp), "%H:%M:%S", std::localtime(&t));
    std::fprintf(log_, "%s.%03d %s\n", stamp, ms, line.c_str());
    std::fflush(log_);
}

static std::string Hex(const std::string& data, size_t max = 48) {
    std::string s;
    char b[4];
    for (size_t i = 0; i < data.size() && i < max; ++i) {
        std::snprintf(b, sizeof(b), "%02X ", (unsigned char)data[i]);
        s += b;
    }
    if (data.size() > max) s += "...";
    return s;
}

void MainFrame::send(uint8_t cmd, const std::string& data) {
    if (!serial_.isOpen()) return;
    auto pkt = proto::pack(cmd, data);
    txBytes_ += pkt.size();
    serial_.write(pkt);
    char head[48];
    std::snprintf(head, sizeof(head), "TX cmd 0x%02X len %u: ", cmd, (unsigned)data.size());
    logLine(head + Hex(data));
}

void MainFrame::sendVersion() {
    send(proto::cVersion, std::string("PCnovaT-") + kAppVersion + "#" + nova::kProtocolVersion);
}

void MainFrame::goOffline(const char* why) {
    if (model_.online) logLine(std::string("Offline: ") + why);
    model_.online = false;
    model_.status = "Offline";
    model_.transport = "stop";
    commsTimeout_ = 0;
}

void MainFrame::sendNumber(const std::string& name, const nova::Index& idx, long long value,
                           int fallbackKind, const nova::Index& fallbackBounds) {
    std::string payload;
    std::string where = name;
    for (int i : idx) where += "[" + std::to_string(i) + "]";
    if (!settings_.encodeNumber(name, idx, value, &payload, fallbackKind, fallbackBounds)) {
        logLine("Cannot encode " + where + " = " + std::to_string(value));
        return;
    }
    logLine("Send " + where + " = " + std::to_string(value));
    send(proto::cSetting, payload);
    applySettings();
    refreshUi();
}

void MainFrame::sendText(const std::string& name, const nova::Index& idx, const std::string& value,
                         const nova::Index& fallbackBounds) {
    std::string payload;
    std::string where = name;
    for (int i : idx) where += "[" + std::to_string(i) + "]";
    if (!settings_.encodeText(name, idx, value, &payload, fallbackBounds)) {
        logLine("Cannot encode " + where + " = " + value);
        return;
    }
    logLine("Send " + where + " = " + value);
    send(proto::cSetting, payload);
    applySettings();
    refreshUi();
}

static const char* MenuText(const char* const* items, int count, long long v) {
    return (v >= 0 && v < count) ? items[v] : "?";
}

void MainFrame::applySettings() {
    const nova::SettingStore& s = settings_;
    if (s.has("sample.rate")) {
        static const char* const rates[] = { "48000", "48048", "88200", "96000" };
        model_.sampleRate = MenuText(rates, 4, s.num("sample.rate", 0));
    }
    if (s.has("timecode.framerate")) {
        static const char* const rates[] = { "23.98", "24", "25", "29.97NDF", "29.97DF", "30NDF", "30DF" };
        model_.frameRate = MenuText(rates, 7, s.num("timecode.framerate", 0));
    }
    if (s.has("record.preroll")) {
        long long v = s.num("record.preroll", 0);
        model_.preroll = v == 0 ? "OFF" : std::to_string(v) + "sec";
    }
    model_.scene = s.text("scene.current", model_.scene);
    model_.take = s.text("take.current", model_.take);
    model_.note = s.text("note.current", model_.note);
    model_.battery = s.text("battery.status", model_.battery);
    model_.remaining = s.text("drive.time.remaining", model_.remaining);
    model_.elapsed = s.text("drive.time.elapsed", model_.elapsed);
    model_.storeCurrent = s.text("store.current", model_.storeCurrent);
    model_.storeSegment = s.text("store.segment", model_.storeSegment);
    model_.storeFolder = s.text("store.folder", model_.storeFolder);
    if (s.has("status.text")) model_.status = s.text("status.text", "");
    if (s.has("hpmix.preset")) {
        long long preset = s.num("hpmix.preset", 1);
        std::string name = s.text("hpmix.names", "", { (int)preset });
        model_.headphone = name.empty() ? "HP " + std::to_string(preset) : name;
    }
    if (s.has("home.meters.source"))
        model_.meterSource = s.num("home.meters.source", 0) == 0 ? "TRACK" : "OUT";
    if (s.has("home.meters.count")) {
        long long n = s.num("home.meters.count", 8);
        model_.meterCount = (int)std::max(1ll, std::min(16ll, n));
    }
    if (s.has("record.tracks")) {
        for (int i = 1; i <= 16; ++i)
            model_.trackArmed[i - 1] = s.num("record.tracks", 0, { i }) != 0;
    }
    if (s.has("track.name")) {
        for (int i = 1; i <= 16; ++i)
            model_.trackName[i - 1] = s.text("track.name", "", { i });
    }
    if (outputMix_ && trackMix_ && headphoneMix_)
        SyncMixPages(*outputMix_, *trackMix_, *headphoneMix_, s);
    if (inputConfig_) SyncInputConfigPage(*inputConfig_, s);
    if (trackNames_) SyncTrackNamesPage(*trackNames_, model_);
    if (mainMenu_) {
        if (auto* b = dynamic_cast<Button*>(mainMenu_->find("Tone"))) {
            b->showValue = true;
            b->value = s.num("tone.enable", 0) ? "ON" : "OFF";
        }
    }
    if (setup_) {
        if (auto* b = dynamic_cast<Button*>(setup_->find("Mp3Rec")))
            b->value = s.num("mp3record.mode", 0) ? "Manual" : "Follow Recorder";
        if (auto* b = dynamic_cast<Button*>(setup_->find("HomeMeters")))
            b->value = std::to_string(s.num("home.meters.count", (long long)model_.meterCount));
    }
    if (auto* b = dynamic_cast<Button*>(setup_->find("PreRecordSetup"))) b->value = model_.preroll;
    if (s.has("neverclip.attenuation")) {
        static const char* const att[] = { "OFF", "-6dB", "-12dB", "-18dB", "-24dB" };
        if (auto* b = dynamic_cast<Button*>(setup_->find("IsoAtt")))
            b->value = MenuText(att, 5, s.num("neverclip.attenuation", 0));
    }
}

void MainFrame::handlePacket(uint8_t cmd, const std::string& data) {
    ++rxPackets_;
    commsTimeout_ = kCommsTimeout;
    if (!model_.online) {
        model_.online = true;
        if (!settings_.has("status.text")) model_.status = "";
        logLine("Online");
        sendVersion();
        send(proto::cRequest);   // ask the Nova to resend its settings
    }
    bool compress = false;
    std::string src = proto::meterSourceFor(cmd, &compress);
    if (!src.empty()) {
        ++meterPackets_;
        auto levels = proto::decodeMeters(data);
        MeterBank::instance().setLevels(src, levels, compress);
        // Mix8 packet byte 17 is the meter for the input selected on the Nova.
        if ((cmd == proto::cInPeaksMix8 || cmd == proto::cInPeaksMix8_COMP) && levels.size() >= 17)
            MeterBank::instance().setLevels("meters.input", std::vector<float>{ levels[16] },
                                            cmd == proto::cInPeaksMix8_COMP);
        return;
    }
    char head[48];
    std::snprintf(head, sizeof(head), "RX cmd 0x%02X len %u: ", cmd, (unsigned)data.size());
    switch (cmd) {
    case proto::cTimecode: {
        const auto* b = (const unsigned char*)data.data();
        if (data.size() >= 11) {
            const char* t = b[10] == 3 ? "record" : b[10] == 2 ? "play" : "stop";
            if (model_.transport != t) logLine(std::string("Transport ") + t);
            model_.transport = t;
        }
        generator_.unpack(b, data.size());
        return;
    }
    case proto::cTimecodeReader:
        return;
    case proto::cSetting: {
        std::string name, summary;
        bool ok = settings_.apply(data, &name, &summary);
        logLine(std::string(ok ? "Setting " : "Setting FAILED ") + summary + (ok ? "" : "  [" + Hex(data) + "]"));
        applySettings();
        return;
    }
    case proto::cVersion:
        logLine(head + Hex(data) + (data.empty() ? " (version request)" : " '" + data + "'"));
        if (data.empty()) sendVersion();
        return;
    case proto::cRequest:
        logLine(head + Hex(data) + " (setting request - not answered yet)");
        return;
    case proto::cGotoPage:
        logLine(std::string(head) + "goto page '" + data + "'");
        return;
    case proto::cPopup:
    case proto::cDebug:
    case proto::cCommand: {
        std::string text;
        for (char c : data)
            if (c >= 32 && c < 127) text += c;
        logLine(std::string(head) + "'" + text + "'");
        return;
    }
    default:
        logLine(head + Hex(data));
        return;
    }
}

void MainFrame::buildMenus() {
    auto* menuBar = new wxMenuBar();

    auto* file = new wxMenu();
    file->Append(wxID_EXIT, "E&xit\tAlt-X");
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(true); }, wxID_EXIT);
    menuBar->Append(file, "&File");

    auto* serial = new wxMenu();
    const int kPortBase = 4000;
    auto ports = SerialPort::listPorts();
    if (ports.empty()) {
        auto* none = serial->Append(wxID_ANY, "(no COM ports)");
        none->Enable(false);
    } else {
        for (size_t i = 0; i < ports.size(); ++i) {
            serial->Append(kPortBase + (int)i, wxString::FromUTF8(ports[i].c_str()));
            Bind(wxEVT_MENU, [this, ports, i](wxCommandEvent&) {
                openPort(ports[i]);
            }, kPortBase + (int)i);
        }
    }
    serial->AppendSeparator();
    int closeId = wxNewId();
    serial->Append(closeId, "Close port");
    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        serial_.close();
        goOffline("port closed");
        logLine("Closed " + portName_);
        portName_.clear();
        wxConfig::Get()->DeleteEntry("SerialPort");
        refreshUi();
    }, closeId);
    menuBar->Append(serial, "&Serial");

    auto* pages = new wxMenu();
    int homeId = wxNewId();
    int metersId = wxNewId();
    pages->Append(homeId, "&Home");
    pages->Append(metersId, "&Meters");
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { showPage(home_.get()); }, homeId);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { showPage(meters_.get()); }, metersId);
    menuBar->Append(pages, "&Pages");

    SetMenuBar(menuBar);
}

void MainFrame::wireNav(Control* root) {
    if (!root) return;
    if (auto* b = dynamic_cast<Button*>(root)) {
        const std::string& id = b->id;
        if (id.rfind("nav:", 0) == 0)
            b->click = [this, dest = id.substr(4)] { openNamed(dest); };
    }
    for (auto& child : root->children)
        wireNav(child.get());
}

void MainFrame::openNamed(const std::string& name) {
    if (name == "Exit") { Close(true); return; }
    if (name == "Fullscreen") {
        ShowFullScreen(!IsFullScreen());
        return;
    }
    if (name == "About") {
        wxMessageBox("Nova Touch GUI version NT0.92.10", "About", wxOK | wxICON_INFORMATION, this);
        return;
    }
    Page* page = nullptr;
    if (name == "Home") page = home_.get();
    else if (name == "Meters") page = meters_.get();
    else if (name == "OutputMix") page = outputMix_.get();
    else if (name == "TrackMix") page = trackMix_.get();
    else if (name == "RecordChannels") page = recordEnables_.get();
    else if (name == "InputConfigure") page = inputConfig_.get();
    else if (name == "HeadphoneMix") page = headphoneMix_.get();
    else if (name == "MainMenu") page = mainMenu_.get();
    else if (name == "SetupMenu") page = setup_.get();
    else if (name == "TrackNames") page = trackNames_.get();
    if (page) showPage(page);
}

void MainFrame::wireTab(const char* id, Page* page) {
    if (auto* b = dynamic_cast<Button*>(header_->find(id)))
        b->click = [this, page] { showPage(page); };
}

void MainFrame::showPage(Page* page) {
    if (!page) return;
    if (history_.empty() || history_.back() != page)
        history_.push_back(page);
    current_ = page;
    canvas_->setPage(current_);
    refreshUi();
}

void MainFrame::goBack() {
    if (history_.size() < 2) return;
    history_.pop_back();
    current_ = history_.back();
    canvas_->setPage(current_);
    refreshUi();
}

void MainFrame::highlightTabs() {
    struct Tab {
        const char* id;
        Page* page;
    };
    const Tab tabs[] = {
        { "TabHome", home_.get() },
        { "TabOutputMix", outputMix_.get() },
        { "TabTrackMix", trackMix_.get() },
        { "TabRecord", recordEnables_.get() },
        { "TabInput", inputConfig_.get() },
        { "TabHeadphone", headphoneMix_.get() },
        { "TabMainMenu", mainMenu_.get() },
    };
    for (const Tab& tab : tabs) {
        if (auto* b = dynamic_cast<Button*>(header_->find(tab.id)))
            b->background = (current_ == tab.page) ? Color::rgb(0x707070) : Color::rgb(0x555555);
    }
}

void MainFrame::openPort(const std::string& name) {
    if (!serial_.open(name, 38400)) {
        wxMessageBox("Could not open " + wxString::FromUTF8(name.c_str()),
                     "Serial", wxOK | wxICON_ERROR, this);
        return;
    }
    rxBuf_.clear();
    goOffline("port opened");
    demoMeters_ = false;
    for (const char* src : { "meters.bus", "meters.out", "meters.bridge", "meters.in" })
        MeterBank::instance().setLevels(src, std::vector<float>(16, 0.f));
    portName_ = name;
    wxConfig::Get()->Write("SerialPort", wxString::FromUTF8(name.c_str()));
    wxConfig::Get()->Flush();
    logLine("Opened " + name + " 38400 8O1");
    refreshUi();
}

void MainFrame::onSerialBytes(const std::string& bytes) {
    rxBytes_ += bytes.size();
    rxBuf_.append(bytes);
    rxErrors_ += proto::parse(rxBuf_, [this](uint8_t cmd, const std::string& data) {
        handlePacket(cmd, data);
    });
}

void MainFrame::refreshUi() {
    model_.timecode = generator_.text();
    if (current_ == home_.get()) SyncHomePage(*home_, model_);
    if (current_ == meters_.get()) SyncMetersPage(*meters_, model_);
    if (current_ == recordEnables_.get()) SyncRecordPage(*recordEnables_, model_);
    highlightTabs();
    if (statusLine_) {
        char buf[384];
        std::snprintf(buf, sizeof(buf),
                      "V0.92.10   Update rate: %d fps, Serial Rx: %llu bytes, %llu errors, %llu packets, Serial Tx: %llu bytes%s%s",
                      fps_,
                      (unsigned long long)rxBytes_,
                      (unsigned long long)rxErrors_,
                      (unsigned long long)rxPackets_,
                      (unsigned long long)txBytes_,
                      portName_.empty() ? "" : ", Port: ",
                      portName_.c_str());
        statusLine_->text = buf;
    }
    canvas_->requestRedraw();
}

void MainFrame::onTimer(wxTimerEvent&) {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - lastTick_).count();
    lastTick_ = now;
    if (dt > 0.2f) dt = 0.2f;
    ++frameCount_;
    if (now - fpsStamp_ >= std::chrono::seconds(1)) {
        fps_ = frameCount_;
        frameCount_ = 0;
        fpsStamp_ = now;
    }

    if (serial_.isOpen()) {
        std::string rx = serial_.readAvailable();
        if (!rx.empty()) onSerialBytes(rx);
    }
    if (model_.online) {
        commsTimeout_ -= dt;
        if (commsTimeout_ <= 0) goOffline("no packets for 3 seconds");
    }
    generator_.advance(dt);
    static float meterLogTimer = 0;
    meterLogTimer += dt;
    if (meterLogTimer >= 10.f) {
        meterLogTimer = 0;
        if (meterPackets_) {
            logLine("(" + std::to_string(meterPackets_) + " meter packets in the last 10 s)");
            meterPackets_ = 0;
        }
    }

    if (demoMeters_ && !model_.online) {
        static float t = 0;
        t += dt;
        std::vector<float> bus(8), bridge(8);
        for (int i = 0; i < 8; ++i) {
            float v = 0.35f + 0.35f * std::sin(t * (1.2f + 0.15f * i) + i);
            if (v < 0) v = 0;
            if (v > 1) v = 1;
            bus[i] = v;
            bridge[i] = v * 0.85f;
        }
        MeterBank::instance().setLevels("meters.bus", bus);
        MeterBank::instance().setLevels("meters.out", bus);
        MeterBank::instance().setLevels("meters.bridge", bridge);
    }

    MeterBank::instance().tick(dt);
    MeterBank::instance().timeout(dt);
    refreshUi();
}
