#include "meter.h"
#include "painter.h"
#include <algorithm>
#include <cmath>

static const float kMeterHold = 0.30f;
static const float kMeterDecay = 1.25f;
static const float kPeakHold = 1.50f;
static const float kPeakDecay = 4.00f;
static const float kTimeout = 1.50f;

void PeakHold::setLevel(float level, float holdTime) {
    level = std::clamp(level, 0.f, 1.f);
    if (level >= peak) {
        peak = level;
        hold = holdTime;
    }
}

void PeakHold::update(float elapsed, float decayTime) {
    if (hold > 0)
        hold -= elapsed;
    else {
        peak -= elapsed / decayTime;
        if (peak < 0) peak = 0;
    }
}

void MeterState::setLevel(float v) {
    level.setLevel(v, kMeterHold);
    peak.setLevel(v, kPeakHold);
}

void MeterState::setCompress(float v) {
    compress.setLevel(v, kMeterHold);
}

void MeterState::update(float elapsed) {
    level.update(elapsed, kMeterDecay);
    peak.update(elapsed, kPeakDecay);
    compress.update(elapsed, kMeterDecay);
}

MeterBank& MeterBank::instance() {
    static MeterBank b;
    return b;
}

void MeterBank::ensure(const std::string& source, int n) {
    auto& g = groups_[source];
    if ((int)g.meters.size() < n)
        g.meters.resize(n);
}

void MeterBank::setLevels(const std::string& source, const std::vector<float>& levels, bool compress) {
    ensure(source, (int)levels.size());
    auto& g = groups_[source];
    for (size_t i = 0; i < g.meters.size(); ++i) {
        float v = i < levels.size() ? levels[i] : 0.f;
        if (compress) g.meters[i].setCompress(v);
        else g.meters[i].setLevel(v);
    }
    g.timeout = kTimeout;
}

MeterState* MeterBank::state(const std::string& source, int index1) {
    auto it = groups_.find(source);
    if (it == groups_.end()) return nullptr;
    int i = index1 - 1;
    if (i < 0 || i >= (int)it->second.meters.size()) return nullptr;
    return &it->second.meters[i];
}

void MeterBank::tick(float elapsed) {
    for (auto& kv : groups_)
        for (auto& m : kv.second.meters)
            m.update(elapsed);
}

void MeterBank::timeout(float elapsed) {
    for (auto& kv : groups_) {
        if (kv.second.timeout <= 0) continue;
        kv.second.timeout -= elapsed;
        if (kv.second.timeout <= 0) {
            for (auto& m : kv.second.meters) {
                m.setLevel(0);
                m.setCompress(0);
            }
        }
    }
}

struct Segment {
    Rect rect;
    Color color;
    const char* text = nullptr;
    bool marker = false;
};

static std::vector<Segment> MeterSegments(const Rect& r, bool vertical) {
    std::vector<Segment> tbl;
    const int n = 4;
    const int dbrange = 40;
    const float axis = vertical ? r.h : r.w;
    float pos = 0;
    for (int i = 1; i <= n; ++i) {
        Segment s;
        float end = i * axis / n;
        int db = (i - n - 1) * dbrange / n;
        if (db >= -10) s.color = NamedColor("red");
        else if (db >= -20) s.color = NamedColor("yellow");
        else s.color = NamedColor("green");
        s.marker = true;
        if (vertical)
            s.rect = Rect{ r.x, r.bottom() - end, r.w, end - pos };
        else
            s.rect = Rect{ r.x + pos, r.y, end - pos, r.h };
        tbl.push_back(s);
        pos = end;
    }
    return tbl;
}

Meter::Meter() {
    style = "none";
    background = Color{ 0, 0, 0 };
}

// Light gray dotted markers at -40, -30, -20 and -10, drawn over the meter color.
static void DrawMeterTicks(Painter& p, const Rect& r, bool vertical) {
    const Color c = NamedColor("lightgray");
    const int n = 4;
    if (vertical) {
        for (int i = 0; i < n; ++i) {
            float y = r.bottom() - i * r.h / n - 1;
            for (float x = r.x; x < r.right(); x += 4)
                p.fillRect(Rect{ x, y, 2, 1 }, c);
        }
    } else {
        for (int i = 0; i < n; ++i) {
            float x = r.x + i * r.w / n;
            for (float y = r.y; y < r.bottom(); y += 4)
                p.fillRect(Rect{ x, y, 1, 2 }, c);
        }
    }
}

void Meter::onPaintContents(Painter& p) {
    MeterState* st = MeterBank::instance().state(source, tag);
    MeterState local;
    if (!st) st = &local;

    const Rect r = bounds;
    const auto segs = MeterSegments(r, vertical);
    const float level = st->level.peak;
    const float peak = st->peak.peak;
    const float compress = st->compress.peak;
    const float len = vertical ? r.h : r.w;
    const float lit = len * level;
    const float peakPos = len * peak;
    const float peakW = std::max(len * 0.015f, 1.f);

    // Background (unlit)
    p.fillRect(r, Color{ 0, 0, 0 });

    auto blitLit = [&](float from, float to) {
        if (to <= from) return;
        if (vertical) {
            float y0 = r.bottom() - to;
            float y1 = r.bottom() - from;
            Rect clip{ r.x, y0, r.w, y1 - y0 };
            for (auto& s : segs) {
                Rect ir{
                    std::max(s.rect.x, clip.x),
                    std::max(s.rect.y, clip.y),
                    0, 0
                };
                ir.w = std::min(s.rect.right(), clip.right()) - ir.x;
                ir.h = std::min(s.rect.bottom(), clip.bottom()) - ir.y;
                if (ir.w > 0 && ir.h > 0) p.fillRect(ir, s.color);
            }
        } else {
            Rect clip{ r.x + from, r.y, to - from, r.h };
            for (auto& s : segs) {
                Rect ir{
                    std::max(s.rect.x, clip.x),
                    std::max(s.rect.y, clip.y),
                    0, 0
                };
                ir.w = std::min(s.rect.right(), clip.right()) - ir.x;
                ir.h = std::min(s.rect.bottom(), clip.bottom()) - ir.y;
                if (ir.w > 0 && ir.h > 0) p.fillRect(ir, s.color);
            }
        }
    };

    blitLit(0, lit);
    if (peakPos > 0) {
        float a = peakPos - peakW;
        if (a < 0) a = 0;
        blitLit(a, peakPos);
        if (vertical)
            p.fillRect(Rect{ r.x, r.bottom() - peakPos - 1, r.w, 2 }, Color{ 255, 255, 255 });
        else
            p.fillRect(Rect{ r.x + peakPos - 1, r.y, 2, r.h }, Color{ 255, 255, 255 });
    }
    if (compress > 0.01f) {
        float c = len * compress;
        if (vertical)
            p.fillRect(Rect{ r.x + r.w * 0.375f, r.bottom() - c, r.w * 0.25f, c }, Color{ 255, 255, 255 });
        else
            p.fillRect(Rect{ r.right() - c, r.y + r.h * 0.375f, c, r.h * 0.25f }, Color{ 255, 255, 255 });
    }

    if (strikethru && !vertical) {
        const Color red = NamedColor("orangered");
        const float x0 = r.x + r.w * 0.02f;
        const float w = r.w * 0.96f;
        for (int i = 0; i < 3; ++i)
            p.fillRect(Rect{ x0, r.y + r.h * (0.3f + 0.2f * i), w, 1 }, red);
    }
    if (!label.empty()) {
        Rect lr{ r.x + r.w * 0.04f, r.y, r.w * 0.5f, r.h };
        p.setFont(std::min(26, std::max(10, (int)(r.h * 0.45f))), true);
        p.textIn(label, lr, "left", true, Color::rgb(0xD0D0D0));
    }
    DrawMeterTicks(p, r, vertical);
}

MeterScale::MeterScale() {
    style = "none";
}

void MeterScale::onPaintContents(Painter& p) {
    p.fillRect(bounds, Color{ 0, 0, 0 });
    const Rect r = bounds;
    DrawMeterTicks(p, r, vertical);
    p.setFont(std::min(20, std::max(10, (int)(r.h * 0.95f))), true);
    struct Mark { const char* text; float frac; const char* align; };
    const Mark marks[] = {
        { "-40.0", 0.f, "left" },
        { "-30.0", 0.25f, "center" },
        { "-20.0", 0.50f, "center" },
        { "-10.0", 0.75f, "center" },
        { "0", 1.f, "right" },
    };
    for (const Mark& m : marks) {
        wxSize sz = p.textBounds(m.text);
        float tw = (float)sz.x;
        float th = (float)sz.y;
        float x = r.x + 2;
        if (m.align[0] == 'c') x = r.x + m.frac * r.w - tw * 0.5f;
        else if (m.align[0] == 'r') x = r.right() - tw - 2;
        float y = r.y + (r.h - th) * 0.5f;
        p.fillRect(Rect{ x - 3, y - 1, tw + 6, th + 2 }, Color{ 0, 0, 0 });
        p.setPen("lightgray");
        p.text(m.text, x, y);
    }
}
