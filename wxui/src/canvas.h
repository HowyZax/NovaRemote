#pragma once

#include "ui.h"
#include <wx/wx.h>
#include <memory>

class Canvas : public wxPanel {
public:
    explicit Canvas(wxWindow* parent);

    void setChrome(Control* header, Control* status);
    void setPage(Page* page);
    void requestRedraw();

private:
    Control* header_ = nullptr;
    Control* status_ = nullptr;
    Page* page_ = nullptr;
    Button* pressed_ = nullptr;
    wxBitmap backBuffer_;
    wxSize backBufferSize_{ 0, 0 };

    void ensureBackBuffer(const wxSize& client);
    void layoutChrome();
    void render(wxDC& dc);
    Control* pick(float x, float y);
    void onPaint(wxPaintEvent&);
    void onErase(wxEraseEvent&);
    void onSize(wxSizeEvent&);
    void onLeftDown(wxMouseEvent&);
    void onLeftUp(wxMouseEvent&);
};
