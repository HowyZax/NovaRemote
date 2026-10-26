#pragma once

#include "geom.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Painter;

enum class LayoutDir { Column, Row };

class Control {
public:
    virtual ~Control() = default;

    std::string id;
    Rect bounds;
    Color background = Color{ 0, 0, 0 };
    Color color = Color{ 255, 255, 255 };
    std::string style = "panel";
    bool visible = true;
    float weight = 1.f;     // share of leftover space
    float fixedFrac = 0.f;  // if >0, fraction of parent in layout axis

    void add(std::unique_ptr<Control> child);
    Control* find(const std::string& id);
    Control* hitTest(float x, float y);

    virtual void layout(const Rect& r);
    virtual void paint(Painter& p);
    virtual void onClick() {}
    virtual void onPaintContents(Painter& p);

    std::vector<std::unique_ptr<Control>> children;
    LayoutDir dir = LayoutDir::Column;
    float margin = 4.f;
};

class Panel : public Control {
public:
    explicit Panel(LayoutDir d = LayoutDir::Column) { dir = d; style = "panel"; }
};

class Label : public Control {
public:
    std::string text;
    std::string align = "left";
    bool bold = false;
    bool mono = false;
    int maxPx = 18;
    void onPaintContents(Painter& p) override;
};

class Button : public Control {
public:
    std::string title;
    std::string value;
    bool showValue = false;
    bool centerTitle = false;  // big centered title over a gold value (FADER / PHASE style)
    int maxFont = 0;           // cap for single-line titles, 0 = default
    bool button1Line = false;  // Home-page style: small label in the top quarter, large gold value
    std::string graphic;       // icon drawn instead of the title (see Painter::icon)
    bool pressed = false;
    std::function<void()> click;

    Button();
    void onPaintContents(Painter& p) override;
    void onClick() override;
};

class TimecodeView : public Control {
public:
    std::string text = "00:00:00:00";
    TimecodeView();
    void onPaintContents(Painter& p) override;
};

class Page : public Control {
public:
    std::string name;
    explicit Page(std::string n);
};

void LayoutChildren(Control& parent, const Rect& r);
