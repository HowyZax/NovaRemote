#pragma once

#include "geom.h"
#include <wx/wx.h>
#include <wx/graphics.h>
#include <string>
#include <utility>
#include <vector>

class Painter {
public:
    explicit Painter(wxDC& dc);

    void setClip(const Rect& r);
    Rect clip() const { return clip_; }

    void setPen(Color c);
    void setPen(const std::string& name);
    void setBrush(Color c);
    void setFont(int pixelSize, bool bold = false, bool mono = false);

    void fill(Color c);
    void fill(const std::string& name);
    void fillRect(const Rect& r, Color c);
    void fillRect(const Rect& r, const std::string& name);

    // Styled fill matching Qt lpainter button/panel/timecode looks.
    void styledRect(const Rect& r, const std::string& style, Color color,
                    const std::string& state = "up");

    void box(const Rect& r, Color c);
    void box(const Rect& r, const std::string& name);
    void polyline(const std::vector<std::pair<float, float>>& pts, Color c, int width = 1);
    // Vector glyphs standing in for the original sprites.svg: home, reply, headphones.
    void icon(const std::string& name, const Rect& r, Color c);

    void text(const std::string& s, float x, float y);
    void textIn(const std::string& s, const Rect& r,
                const std::string& align = "center",
                bool fit = false,
                Color color = Color{ 255, 255, 255 });

    wxSize textBounds(const std::string& s) const;

private:
    wxDC& dc_;
    Rect clip_;
    Color pen_{ 255, 255, 255 };
    wxFont font_;
    bool mono_ = false;

    wxColour wx(Color c) const { return wxColour(c.r, c.g, c.b, c.a); }
    void drawRounded(const Rect& r, Color fill, Color border, int radius, int borderSize);
    void gradientRounded(const Rect& r, const std::vector<std::pair<float, Color>>& stops,
                         Color border, int radius, int borderSize);
    wxGraphicsContext* makeGc();
};
