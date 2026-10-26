#include "ui.h"
#include "painter.h"
#include <algorithm>
#include <cctype>

void Control::add(std::unique_ptr<Control> child) {
    children.push_back(std::move(child));
}

Control* Control::find(const std::string& want) {
    std::string key = want;
    for (char& c : key) c = (char)std::tolower((unsigned char)c);
    std::string mine = id;
    for (char& c : mine) c = (char)std::tolower((unsigned char)c);
    if (mine == key) return this;
    for (auto& ch : children) {
        if (Control* f = ch->find(want)) return f;
    }
    return nullptr;
}

Control* Control::hitTest(float x, float y) {
    if (!visible || !bounds.contains(x, y)) return nullptr;
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        if (Control* f = (*it)->hitTest(x, y)) return f;
    }
    return this;
}

void LayoutChildren(Control& parent, const Rect& r) {
    parent.bounds = r;
    if (parent.children.empty()) return;

    const bool row = parent.dir == LayoutDir::Row;
    const float innerW = r.w - parent.margin * 2;
    const float innerH = r.h - parent.margin * 2;
    const float axis = row ? innerW : innerH;
    const float cross = row ? innerH : innerW;

    float used = 0;
    float weightSum = 0;
    int visibleN = 0;
    for (auto& ch : parent.children) {
        if (!ch->visible) continue;
        ++visibleN;
        if (ch->fixedFrac > 0)
            used += axis * ch->fixedFrac;
        else
            weightSum += ch->weight;
    }
    if (visibleN == 0) return;
    used += parent.margin * (visibleN - 1);
    float leftover = axis - used;
    if (leftover < 0) leftover = 0;
    if (weightSum <= 0) weightSum = 1;

    float pos = row ? r.x + parent.margin : r.y + parent.margin;
    for (auto& ch : parent.children) {
        if (!ch->visible) continue;
        float span = (ch->fixedFrac > 0)
            ? axis * ch->fixedFrac
            : leftover * (ch->weight / weightSum);
        Rect cr;
        if (row)
            cr = Rect{ pos, r.y + parent.margin, span, cross };
        else
            cr = Rect{ r.x + parent.margin, pos, cross, span };
        ch->layout(cr);
        pos += span + parent.margin;
    }
}

void Control::layout(const Rect& r) {
    LayoutChildren(*this, r);
}

void Control::paint(Painter& p) {
    if (!visible) return;
    p.setClip(bounds);
    if (style != "none") {
        const char* state = "up";
        if (auto* b = dynamic_cast<Button*>(this))
            if (b->pressed) state = "down";
        p.styledRect(bounds, style, background, state);
    }
    onPaintContents(p);
    for (auto& ch : children)
        ch->paint(p);
}

void Control::onPaintContents(Painter&) {}

void Label::onPaintContents(Painter& p) {
    int px = (int)(bounds.h * 0.72f);
    if (px > maxPx) px = maxPx;
    if (px < 11) px = 11;
    p.setFont(px, bold, mono);
    p.textIn(text, bounds.shrink(8, 1), align, true, color);
}

Button::Button() {
    style = "button";
    background = Color{ 0, 0, 0 };
}

void Button::onPaintContents(Painter& p) {
    Rect r = bounds.shrink(8, 4);
    if (!graphic.empty() && title.empty() && !button1Line) {
        float s = std::min(bounds.w, bounds.h) * 0.55f;
        p.icon(graphic, Rect{ bounds.hcenter() - s * 0.5f, bounds.vcenter() - s * 0.5f, s, s }, color);
        return;
    }
    if (button1Line) {
        Rect head{ bounds.x + 2, bounds.y + 2, bounds.w - 8, bounds.h * 0.25f };
        if (!graphic.empty()) {
            p.icon(graphic, Rect{ head.x + 4, head.y, head.h, head.h }, Color{ 255, 255, 255 });
        } else if (!title.empty()) {
            p.setFont(std::max(9, (int)(head.h * 0.95f)), true);
            p.textIn(title, head, "left", true, Color{ 255, 255, 255 });
        }
        Rect vr{ bounds.x, bounds.y + bounds.h * 0.25f, bounds.w, bounds.h * 0.75f };
        vr = vr.shrink(vr.w * 0.1f, vr.h * 0.05f);
        if (!value.empty()) {
            p.setFont(std::max(10, (int)(vr.h * 0.95f)), true);
            p.textIn(value, vr, "center", true, NamedColor("gold"));
        }
        return;
    }
    if (style == "tab") {
        Rect inner = bounds.shrink(4, 3);
        if (showValue && !value.empty()) {
            Rect top{ inner.x, inner.y, inner.w, inner.h * 0.48f };
            Rect bot{ inner.x, inner.y + inner.h * 0.48f, inner.w, inner.h * 0.52f };
            p.setFont(std::min(20, std::max(11, (int)(top.h * 0.75f))), true);
            p.textIn(title, top, "center", true, color);
            p.textIn(value, bot, "center", true, color);
        } else {
            p.setFont(std::min(18, std::max(12, (int)(inner.h * 0.42f))), true);
            p.textIn(title, inner, "center", true, color);
        }
        return;
    }
    const bool twoLine = showValue || !value.empty();
    if (twoLine && centerTitle) {
        Rect top{ r.x, r.y + r.h * 0.12f, r.w, r.h * 0.40f };
        Rect bot{ r.x, r.y + r.h * 0.55f, r.w, r.h * 0.35f };
        p.setFont(std::min(24, std::max(13, (int)(top.h * 0.75f))), true);
        p.textIn(title, top, "center", true, Color{ 255, 255, 255 });
        if (!value.empty()) {
            p.setFont(std::min(17, std::max(11, (int)(bot.h * 0.7f))), true);
            p.textIn(value, bot, "center", true, NamedColor("gold"));
        }
        return;
    }
    if (twoLine) {
        const float topH = std::min(18.f, std::max(11.f, r.h * 0.30f));
        Rect top{ r.x, r.y, r.w, topH };
        Rect bot{ r.x, r.y + topH, r.w, r.h - topH };
        if (!title.empty()) {
            p.setFont(11);
            p.textIn(title, top, "left", true, Color{ 230, 230, 230 });
        }
        if (!value.empty()) {
            p.setFont(std::min(36, std::max(14, (int)(bot.h * 0.70f))), true);
            p.textIn(value, bot, "center", true, NamedColor("gold"));
        }
    } else {
        int cap = maxFont > 0 ? maxFont : 42;
        p.setFont(std::min(cap, std::max(12, (int)(r.h * 0.48f))), true);
        p.textIn(title, r, "center", true, color);
    }
}

void Button::onClick() {
    if (click) click();
}

TimecodeView::TimecodeView() {
    style = "timecode";
    background = Color::rgb(0x000020);
    color = Color::rgb(0xF0F0FF);
}

void TimecodeView::onPaintContents(Painter& p) {
    p.setFont((int)(bounds.h * 0.7f), true);
    p.textIn(text, bounds, "center", true, color);
}

Page::Page(std::string n) : name(std::move(n)) {
    id = name;
    style = "none";
    background = Color{ 0, 0, 0 };
}
