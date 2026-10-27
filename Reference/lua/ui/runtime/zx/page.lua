--local widgets = require 'widget'

Page.DefaultWidth = 480
Page.DefaultHeight = 272

function Page.Exit()
    widgets.PageExit()
end

function Page:setHome()
    assert(self.widget)
    self.widget:setHome()
end

function Page.Current()
    local w = widgets.PageCurrent()
    for _,page in pairs(pages) do
        if page.widget == w then
            return page
        end
    end
    assert(false)
    return nil
end

function Page:enter()
    assert(self.widget)
    self.widget:enter()
end

function Page:done()
    widgets.PageExit()
end

function Page:update()
    widgets.PageRefresh()
end

function Page:isVisible()
    return self.widget == widgets.PageCurrent()
end

