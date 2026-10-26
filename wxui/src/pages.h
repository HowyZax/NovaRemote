#pragma once

#include "settings.h"
#include "ui.h"
#include <functional>
#include <memory>
#include <string>

// AI: scene, take, and note here are the text on the Home screen. Remote_PC.cpp
// does not apply the scene/take/note settings the PC sends. trackArmed, mix
// cells, and Input Config do send when a port is open.
struct AppModel {
    bool online = false;
    std::string transport = "stop"; // stop | play | record
    std::string timecode = "--:--:--:--";
    std::string sampleRate = "48000";
    std::string frameRate = "23.98";
    std::string preroll = "OFF";
    std::string scene = "SCENE 1";
    std::string take = "1";
    std::string note = "Nova Touch";
    std::string headphone = "HP 1";
    std::string battery = "---";
    std::string remaining = "000:00:00";
    std::string elapsed = "000:00:00";
    std::string storeCurrent = "---";
    std::string storeSegment = "---";
    std::string storeFolder = "---";
    std::string status = "Offline";
    std::string meterSource = "TRACK"; // TRACK | OUT
    int meterCount = 8;
    bool trackArmed[16] = {};
    std::string trackName[16];  // empty = "TRACK n"
};

std::unique_ptr<Panel> BuildHeader();
std::unique_ptr<Label> BuildStatusLine();
std::unique_ptr<Page> BuildHomePage();
std::unique_ptr<Page> BuildMetersPage();
std::unique_ptr<Page> BuildPlaceholderPage(const std::string& title);
std::unique_ptr<Page> BuildMainMenuPage();
std::unique_ptr<Page> BuildRecordPage();
std::unique_ptr<Page> BuildOutputMixPage();
std::unique_ptr<Page> BuildTrackMixPage();
std::unique_ptr<Page> BuildHeadphoneMixPage();
std::unique_ptr<Page> BuildInputConfigPage();
std::unique_ptr<Page> BuildSetupPage();
std::unique_ptr<Page> BuildTrackNamesPage();
// Deliver a changed setting to the Nova. idx is 1-based, matching the protocol.
using SettingSink = std::function<void(const std::string&, const nova::Index&, long long)>;
void SetMixSink(Page& outputMix, Page& trackMix, Page& headphoneMix, SettingSink sink);
void SyncMixPages(Page& outputMix, Page& trackMix, Page& headphoneMix, const nova::SettingStore& s);

// Read a stored number while a click decides the next value. Missing values are 0.
using SettingGet = std::function<long long(const std::string&, const nova::Index&)>;
void SetInputConfigSink(Page& page, SettingSink sink, SettingGet get);
void SyncInputConfigPage(Page& page, const nova::SettingStore& s);

void SyncHomePage(Page& page, const AppModel& m);
void SyncMetersPage(Page& page, const AppModel& m);
void SyncRecordPage(Page& page, const AppModel& m);
void SyncTrackNamesPage(Page& page, const AppModel& m);
