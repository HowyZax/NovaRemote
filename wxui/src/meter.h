#pragma once

#include "ui.h"
#include <string>
#include <unordered_map>
#include <vector>

struct PeakHold {
    float peak = 0;
    float hold = 0;
    void setLevel(float level, float holdTime);
    void update(float elapsed, float decayTime);
};

struct MeterState {
    PeakHold level;
    PeakHold peak;
    PeakHold compress;
    void setLevel(float v);
    void setCompress(float v);
    void update(float elapsed);
};

class MeterBank {
public:
    static MeterBank& instance();

    void ensure(const std::string& source, int n);
    void setLevels(const std::string& source, const std::vector<float>& levels, bool compress = false);
    MeterState* state(const std::string& source, int index1);
    void tick(float elapsed);
    void timeout(float elapsed);

private:
    struct Group {
        std::vector<MeterState> meters;
        float timeout = 0;
    };
    std::unordered_map<std::string, Group> groups_;
};

class Meter : public Control {
public:
    std::string source = "meters.bus";
    int tag = 1;
    std::string label;
    bool strikethru = false;
    bool vertical = false;
    bool scale = false;

    Meter();
    void onPaintContents(Painter& p) override;
};

class MeterScale : public Control {
public:
    bool vertical = false;
    MeterScale();
    void onPaintContents(Painter& p) override;
};
