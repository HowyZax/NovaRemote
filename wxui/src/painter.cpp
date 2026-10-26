#include "painter.h"
#include <algorithm>

Painter::Painter(wxDC& dc)
    : dc_(dc)
    , font_(wxFontInfo(12).Family(wxFONTFAMILY_SWISS))
{
    wxSize sz = dc.GetSize();
    clip_ = Rect{ 0, 0, (float)sz.x, (float)sz.y };
    dc_.SetFont(font_);
}

wxGraphicsContext* Painter::makeGc() {
    wxGraphicsContext* gc = wxGraphicsContext::CreateFromUnknownDC(dc_);
    if (!gc) return nullptr;
    if (clip_.w > 0 && clip_.h > 0)
        gc->Clip(clip_.x, clip_.y, clip_.w, clip_.h);
    return gc;
}

void Painter::setClip(const Rect& r) {
    clip_ = r;
    dc_.DestroyClippingRegion();
    if (r.w > 0 && r.h > 0)
        dc_.SetClippingRegion((int)r.x, (int)r.y, (int)r.w, (int)r.h);
}

void Painter::setPen(Color c) { pen_ = c; }
void Painter::setPen(const std::string& name) { pen_ = NamedColor(name, pen_); }
void Painter::setBrush(Color c) { (void)c; }

void Painter::setFont(int pixelSize, bool bold, bool mono) {
    if (pixelSize < 8) pixelSize = 8;
    mono_ = mono;
    font_ = wxFontInfo(wxSize(0, pixelSize))
                .Family(mono ? wxFONTFAMILY_TELETYPE : wxFONTFAMILY_SWISS)
                .Weight(bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
    dc_.SetFont(font_);
}

void Painter::fill(Color c) { fillRect(clip_, c); }
void Painter::fill(const std::string& name) { fill(NamedColor(name)); }

void Painter::fillRect(const Rect& r, Color c) {
    if (r.w <= 0 || r.h <= 0) return;
    dc_.SetPen(*wxTRANSPARENT_PEN);
    dc_.SetBrush(wxBrush(wx(c)));
    dc_.DrawRectangle((int)r.x, (int)r.y, (int)r.w, (int)r.h);
}

void Painter::fillRect(const Rect& r, const std::string& name) {
    fillRect(r, NamedColor(name));
}

void Painter::drawRounded(const Rect& r, Color fill, Color border, int radius, int borderSize) {
    if (r.w <= 0 || r.h <= 0) return;
    if (wxGraphicsContext* gc = makeGc()) {
        gc->SetBrush(wxBrush(wx(fill)));
        gc->SetPen(wxPen(wx(border), std::max(1, borderSize)));
        gc->DrawRoundedRectangle(r.x, r.y, r.w - 1, r.h - 1, radius);
        delete gc;
        return;
    }
    dc_.SetBrush(wxBrush(wx(fill)));
    dc_.SetPen(wxPen(wx(border), std::max(1, borderSize)));
    dc_.DrawRoundedRectangle((int)r.x, (int)r.y, (int)r.w, (int)r.h, radius);
}

void Painter::gradientRounded(const Rect& r, const std::vector<std::pair<float, Color>>& stops,
                              Color border, int radius, int borderSize) {
    if (r.w <= 0 || r.h <= 0 || stops.size() < 2) return;
    wxGraphicsContext* gc = makeGc();
    if (!gc) {
        drawRounded(r, stops[stops.size() / 2].second, border, radius, borderSize);
        return;
    }
    wxGraphicsGradientStops gs(wx(stops.front().second), wx(stops.back().second));
    for (size_t i = 1; i + 1 < stops.size(); ++i)
        gs.Add(wx(stops[i].second), stops[i].first);
    gc->SetBrush(gc->CreateLinearGradientBrush(r.x, r.y, r.x, r.y + r.h, gs));
    if (borderSize > 0) gc->SetPen(wxPen(wx(border), borderSize));
    else gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRoundedRectangle(r.x, r.y, r.w - 1, r.h - 1, radius);
    delete gc;
}

void Painter::polyline(const std::vector<std::pair<float, float>>& pts, Color c, int width) {
    if (pts.size() < 2) return;
    std::vector<wxPoint> w;
    w.reserve(pts.size());
    for (auto& pt : pts) w.emplace_back((int)pt.first, (int)pt.second);
    dc_.SetPen(wxPen(wx(c), width));
    dc_.DrawLines((int)w.size(), w.data());
}

void Painter::icon(const std::string& name, const Rect& r, Color c) {
    const float s = std::min(r.w, r.h);
    if (s <= 2) return;
    const float x0 = r.x + (r.w - s) * 0.5f;
    const float y0 = r.y + (r.h - s) * 0.5f;
    auto X = [&](float f) { return (double)(x0 + f * s); };
    auto Y = [&](float f) { return (double)(y0 + f * s); };
    wxGraphicsContext* gc = makeGc();
    if (!gc) return;
    gc->SetBrush(wxBrush(wx(c)));
    gc->SetPen(*wxTRANSPARENT_PEN);
    wxGraphicsPath path = gc->CreatePath();
    if (name == "home") {
        static const float pts[][2] = {
            { 0.50f, 0.06f }, { 0.97f, 0.50f }, { 0.83f, 0.50f }, { 0.83f, 0.94f },
            { 0.60f, 0.94f }, { 0.60f, 0.66f }, { 0.40f, 0.66f }, { 0.40f, 0.94f },
            { 0.17f, 0.94f }, { 0.17f, 0.50f }, { 0.03f, 0.50f },
        };
        path.MoveToPoint(X(pts[0][0]), Y(pts[0][1]));
        for (size_t i = 1; i < sizeof(pts) / sizeof(pts[0]); ++i)
            path.AddLineToPoint(X(pts[i][0]), Y(pts[i][1]));
        path.CloseSubpath();
        gc->FillPath(path);
    } else if (name == "reply") {
        path.MoveToPoint(X(0.04f), Y(0.40f));
        path.AddLineToPoint(X(0.44f), Y(0.08f));
        path.AddLineToPoint(X(0.44f), Y(0.72f));
        path.CloseSubpath();
        gc->FillPath(path);
        wxGraphicsPath tail = gc->CreatePath();
        tail.MoveToPoint(X(0.40f), Y(0.40f));
        tail.AddCurveToPoint(X(0.78f), Y(0.40f), X(0.96f), Y(0.56f), X(0.92f), Y(0.94f));
        wxPen pen(wx(c), std::max(1, (int)(s * 0.16f)));
        pen.SetCap(wxCAP_BUTT);
        gc->SetPen(pen);
        gc->StrokePath(tail);
    } else if (name == "headphones") {
        wxGraphicsPath band = gc->CreatePath();
        const double pi = 3.14159265358979;
        band.AddArc(X(0.5f), Y(0.56f), s * 0.38, pi, 2 * pi, true);
        gc->SetPen(wxPen(wx(c), std::max(1, (int)(s * 0.10f))));
        gc->StrokePath(band);
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(X(0.06f), Y(0.50f), s * 0.22, s * 0.44, s * 0.06);
        gc->DrawRoundedRectangle(X(0.72f), Y(0.50f), s * 0.22, s * 0.44, s * 0.06);
    }
    delete gc;
}

void Painter::styledRect(const Rect& r, const std::string& style, Color color,
                         const std::string& state) {
    // Same as QColor::lighter(factor): scale brightness, spill the excess into the other channels.
    auto qtLighter = [](Color c, int factor) {
        int mx = std::max({ (int)c.r, (int)c.g, (int)c.b });
        if (mx == 0) return c;
        int v = mx * factor / 100;
        if (v <= 255) {
            auto s = [&](unsigned char ch) { return (unsigned char)std::min(255, ch * factor / 100); };
            return Color{ s(c.r), s(c.g), s(c.b), c.a };
        }
        int overflow = v - 255;
        auto s = [&](unsigned char ch) {
            int x = ch * 255 / mx;
            x += (255 - x) * overflow / std::max(1, v);
            return (unsigned char)std::min(255, x);
        };
        return Color{ s(c.r), s(c.g), s(c.b), c.a };
    };
    Color c0 = color;
    Color c1 = c0;
    Color c2 = c0;
    if (c0.lightness() == 0) {
        c0 = Color::rgb(0x262626);
        c1 = Color::rgb(0x353535);
        c2 = Color::rgb(0x6f6f6f);
    } else if (c0.lightness() > 200) {
        c1 = qtLighter(c0, 105);
        c2 = qtLighter(c0, 120);
    } else if (c0.lightness() > 100) {
        c1 = qtLighter(c0, 110);
        c2 = qtLighter(c0, 130);
    } else {
        c1 = qtLighter(c0, 115);
        c2 = qtLighter(c0, 140);
    }

    int radius = 3;
    int border = 1;
    Color borderColor = Color::rgb(0x333333);

    if (style == "panel.gradient") {
        gradientRounded(r, { { 0.f, c1 }, { 0.05f, c0 }, { 1.f, c2 } }, Color{ 255, 255, 255 }, 8, 1);
        return;
    }
    if (style == "panel.bg") {
        drawRounded(r, color, Color::rgb(0x555555), 4, 1);
        return;
    }
    if (style == "cell.panel") {
        drawRounded(r, color, color, 4, 1);
        return;
    }
    if (style == "button.cell") {
        if (state == "down")
            drawRounded(r, c1, borderColor, radius, border);
        else
            gradientRounded(r, { { 0.f, c2 }, { 0.5f, c0 }, { 1.f, c1 } }, borderColor, radius, border);
        return;
    }
    if (style == "button.selected" && state != "down") {
        gradientRounded(r, { { 0.f, Color{ 255, 255, 255 } }, { 0.05f, c2 }, { 0.49f, c1 }, { 0.50f, c0 }, { 1.f, c0 } },
                        Color::rgb(0x00C000), 3, 2);
        return;
    }
    if (style == "fill" || style == "bar") {
        fillRect(r, color);
        return;
    }
    if (style == "panel" || style == "panel.flat") {
        radius = (style == "panel.flat") ? 0 : 8;
        borderColor = Color{ 255, 255, 255 };
        drawRounded(r, color, borderColor, radius, border);
        return;
    }
    if (style == "timecode") {
        drawRounded(r, color, Color::rgb(0x60626F), 3, 1);
        return;
    }

    if (state == "down") {
        drawRounded(r, c1, borderColor, radius, border);
        return;
    }

    if (style.rfind("button", 0) == 0 || style == "tab") {
        if (wxGraphicsContext* gc = makeGc()) {
            wxGraphicsGradientStops stops(*wxWHITE, wx(c0));
            stops.Add(wx(c2), 0.05f);
            stops.Add(wx(c1), 0.49f);
            stops.Add(wx(c0), 0.50f);
            gc->SetBrush(gc->CreateLinearGradientBrush(
                r.x, r.y, r.x, r.y + r.h, stops));
            gc->SetPen(wxPen(wx(borderColor), border));
            gc->DrawRoundedRectangle(r.x, r.y, r.w - 1, r.h - 1, radius);
            delete gc;
            return;
        }
    }

    drawRounded(r, color, borderColor, radius, border);
}

void Painter::box(const Rect& r, Color c) {
    if (r.w <= 0 || r.h <= 0) return;
    dc_.SetBrush(*wxTRANSPARENT_BRUSH);
    dc_.SetPen(wxPen(wx(c)));
    if ((int)r.w == 1)
        dc_.DrawLine((int)r.x, (int)r.y, (int)r.x, (int)r.bottom() - 1);
    else if ((int)r.h == 1)
        dc_.DrawLine((int)r.x, (int)r.y, (int)r.right() - 1, (int)r.y);
    else
        dc_.DrawRectangle((int)r.x, (int)r.y, (int)r.w, (int)r.h);
}

void Painter::box(const Rect& r, const std::string& name) {
    box(r, NamedColor(name));
}

void Painter::text(const std::string& s, float x, float y) {
    dc_.SetTextForeground(wx(pen_));
    dc_.SetFont(font_);
    dc_.DrawText(wxString::FromUTF8(s.c_str()), (int)x, (int)y);
}

void Painter::textIn(const std::string& s, const Rect& r,
                     const std::string& align, bool fit, Color color) {
    if (s.empty() || r.w <= 1 || r.h <= 1) return;
    dc_.SetFont(font_);
    const bool bold = font_.GetWeight() == wxFONTWEIGHT_BOLD;
    wxString ws = wxString::FromUTF8(s.c_str());
    wxSize sz = dc_.GetTextExtent(ws);
    if (fit) {
        int px = font_.GetPixelSize().GetHeight();
        if (px < 8) px = std::max(8, font_.GetPointSize());
        while (px > 8 && (sz.x > r.w || sz.y > r.h)) {
            --px;
            setFont(px, bold, mono_);
            sz = dc_.GetTextExtent(ws);
        }
    }
    float x = r.x;
    float y = r.y + (r.h - sz.y) * 0.5f;
    if (align == "center") x = r.x + (r.w - sz.x) * 0.5f;
    else if (align == "right") x = r.right() - sz.x;
    else if (align == "left") x = r.x + 4;
    if (y < r.y) y = r.y;
    dc_.SetTextForeground(wx(color));
    wxDCClipper clip(dc_, (int)r.x, (int)r.y, (int)r.w, (int)r.h);
    dc_.DrawText(ws, (int)x, (int)y);
}

wxSize Painter::textBounds(const std::string& s) const {
    return dc_.GetTextExtent(wxString::FromUTF8(s.c_str()));
}
