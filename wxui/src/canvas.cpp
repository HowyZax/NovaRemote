#include "canvas.h"
#include "painter.h"
#include <wx/dcmemory.h>
#include <algorithm>

Canvas::Canvas(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxBLACK);
    Bind(wxEVT_PAINT, &Canvas::onPaint, this);
    Bind(wxEVT_ERASE_BACKGROUND, &Canvas::onErase, this);
    Bind(wxEVT_SIZE, &Canvas::onSize, this);
    Bind(wxEVT_LEFT_DOWN, &Canvas::onLeftDown, this);
    Bind(wxEVT_LEFT_UP, &Canvas::onLeftUp, this);
}

void Canvas::ensureBackBuffer(const wxSize& client) {
    if (client.x <= 0 || client.y <= 0) return;
    if (backBufferSize_ == client && backBuffer_.IsOk()) return;
    backBufferSize_ = client;
    backBuffer_ = wxBitmap(client.x, client.y, 24);
}

void Canvas::layoutChrome() {
    wxSize sz = GetClientSize();
    float w = (float)sz.x;
    float h = (float)sz.y;
    if (w < 1 || h < 1) return;
    float headerH = header_ ? std::max(40.f, h * 0.1f) : 0.f;
    float statusH = status_ ? 18.f : 0.f;
    if (header_) header_->layout(Rect{ 0, 0, w, headerH });
    if (page_) page_->layout(Rect{ 0, headerH, w, h - headerH - statusH });
    if (status_) status_->layout(Rect{ 0, h - statusH, w, statusH });
}

void Canvas::render(wxDC& dc) {
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    Painter p(dc);
    if (header_) header_->paint(p);
    if (page_) page_->paint(p);
    if (status_) status_->paint(p);
}

void Canvas::setChrome(Control* header, Control* status) {
    header_ = header;
    status_ = status;
    layoutChrome();
    Refresh(false);
}

void Canvas::setPage(Page* page) {
    page_ = page;
    layoutChrome();
    Refresh(false);
}

void Canvas::requestRedraw() { Refresh(false); }

void Canvas::onErase(wxEraseEvent&) {}

void Canvas::onSize(wxSizeEvent&) {
    layoutChrome();
    Refresh(false);
}

void Canvas::onPaint(wxPaintEvent&) {
    wxPaintDC paintDC(this);
    const wxSize client = GetClientSize();
    ensureBackBuffer(client);
    if (!backBuffer_.IsOk()) {
        render(paintDC);
        return;
    }
    wxMemoryDC memDC;
    memDC.SelectObject(backBuffer_);
    render(memDC);
    memDC.SelectObject(wxNullBitmap);
    paintDC.DrawBitmap(backBuffer_, 0, 0);
}

Control* Canvas::pick(float x, float y) {
    if (header_) {
        if (Control* c = header_->hitTest(x, y)) return c;
    }
    if (page_) {
        if (Control* c = page_->hitTest(x, y)) return c;
    }
    if (status_) {
        if (Control* c = status_->hitTest(x, y)) return c;
    }
    return nullptr;
}

void Canvas::onLeftDown(wxMouseEvent& e) {
    Control* c = pick((float)e.GetX(), (float)e.GetY());
    if (auto* b = dynamic_cast<Button*>(c)) {
        pressed_ = b;
        b->pressed = true;
        Refresh(false);
    }
}

void Canvas::onLeftUp(wxMouseEvent& e) {
    Control* c = pick((float)e.GetX(), (float)e.GetY());
    Button* was = pressed_;
    pressed_ = nullptr;
    if (was) was->pressed = false;
    if (was && c == was) was->onClick();
    Refresh(false);
}
