#pragma once

#include <cstdint>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

struct Color {
    unsigned char r = 0, g = 0, b = 0, a = 255;

    static Color rgb(unsigned rgb) {
        return Color{
            (unsigned char)((rgb >> 16) & 0xFF),
            (unsigned char)((rgb >> 8) & 0xFF),
            (unsigned char)(rgb & 0xFF),
            255
        };
    }

    Color lighter(int percent) const {
        auto lift = [&](unsigned char c) -> unsigned char {
            int v = c + (255 - c) * percent / 100;
            if (v > 255) v = 255;
            return (unsigned char)v;
        };
        return Color{ lift(r), lift(g), lift(b), a };
    }

    int lightness() const {
        return (int)(0.299 * r + 0.587 * g + 0.114 * b);
    }
};

inline Color NamedColor(const std::string& name, Color fallback = Color{ 0, 0, 0 }) {
    if (name.empty()) return fallback;
    if (name[0] == '#') {
        unsigned v = 0;
        sscanf(name.c_str() + 1, "%x", &v);
        if (name.size() <= 7) return Color::rgb(v);
        return Color{
            (unsigned char)((v >> 16) & 0xFF),
            (unsigned char)((v >> 8) & 0xFF),
            (unsigned char)(v & 0xFF),
            (unsigned char)((v >> 24) & 0xFF)
        };
    }
    if (name == "black") return Color{ 0, 0, 0 };
    if (name == "white") return Color{ 255, 255, 255 };
    if (name == "gold") return Color{ 255, 215, 0 };
    if (name == "orangered") return Color{ 255, 69, 0 };
    if (name == "red") return Color{ 220, 0, 0 };
    if (name == "yellow") return Color{ 220, 200, 0 };
    if (name == "green") return Color{ 0, 180, 40 };
    if (name == "darkgray" || name == "darkgrey") return Color{ 80, 80, 80 };
    if (name == "lightgray" || name == "lightgrey") return Color{ 180, 180, 180 };
    if (name == "gray" || name == "grey") return Color{ 96, 96, 96 };
    if (name == "on") return Color{ 240, 240, 255 };
    return fallback;
}

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;

    float right() const { return x + w; }
    float bottom() const { return y + h; }
    float hcenter() const { return x + w * 0.5f; }
    float vcenter() const { return y + h * 0.5f; }

    bool contains(float px, float py) const {
        return px >= x && py >= y && px < right() && py < bottom();
    }

    Rect shrink(float dx, float dy) const {
        Rect r{ x + dx, y + dy, w - 2 * dx, h - 2 * dy };
        if (r.w < 0) r.w = 0;
        if (r.h < 0) r.h = 0;
        return r;
    }

    Rect inset(float l, float t, float rgt, float b) const {
        return Rect{ x + l, y + t, w - l - rgt, h - t - b };
    }
};
