local config = require 'config'

Page.DefaultWidth = 480
Page.DefaultHeight = 272

local pageHome
--local pageCurrent

function Page:setHome()
    pageHome = self
end
function Page.Current()
    error 'Multiple pages can be active; current page is not a supported concept'
--  return pageCurrent
end

function Page:MSG_PAINT()
    Canvas:update()
end

function Page:enter(...)
    print('GOTO PAGE', self.name, self.widget, self.ExitPage)

    if not self.widget then
        self:create()
    end

    assert(self.widget)
    if self.modal then
--      local x,y,w,h = mainWindow:geometry() --  root.widget:geometry()
--      local r = Rect(Desktop.availableGeometry()):shrink(0.1)
--      self.widget:setGeometry(r:unpack())
        local r = Rect(Desktop.availableGeometry()):shrink(0.2)
--      mainWindow:animate(self.widget, r:unpack())
        self.widget:setGeometry(r:unpack())
        self:perform('onEnter', ...)
        self:update()
        self.widget:exec()
        self:perform('onExit')
    elseif self.floating then
        local options = ...
--      local x,y,w,h = mainWindow:geometry() --  root.widget:geometry()

        if options and options.animateFrom then
            local screen = Rect(Desktop.availableGeometry())
            local rdflt = config.nomad and screen:shrink{ 0.05, 0.2, 0.05, 0 } or
                                           screen:shrink( 0.3 )
            local r2 = options.animateTo or rdflt
            local r1 = r2:copy()
            if true then
                local edges = {
                    { screen:bottom() - r2:bottom(), function() r1.y = screen:bottom() end },
                    { screen:right() - r2:right(),   function() r1.x = screen:right() end },
                    { r2:left() - screen:left(),     function() r1.x = screen:left() - r2:width() end },
                    { r2:top() - screen:top(),       function() r1.y = screen:top() - r2:height() end },
                }
                local imin = 1
                for i = 2,#edges do
                    if edges[i][1] < edges[imin][1] then
                        imin = i
                    end
                end
                edges[imin][2]()
            else
                r1:setCenter( options.animateFrom:center() )
                --r1.x = options.animateFrom.x
                --r1.y = options.animateFrom.y
            end
            do
                self.widget:setGeometry(r2:unpack()) -- workaround: set final geometry while hidden, for setFrame
                self.widget:setFrame(Page.wantFrames)
            end


            self.widget:animate(r1.x, r1.y, r1.w, r1.h, r2:unpack())
        end

        self.widget:show()
        self:perform('onEnter', ...)
        self.widget:setFocus()

        --self.widget:exec()
        --self:perform('onExit')
    elseif self.popup then
        local parent = ...
        assert(isinstance(parent, Page), 'Parent page required for popup')
        assert(isinstance(parent.parent, PageStack))
        parent.parent:enter(self, ...)
    else
        RootSetPage(self)
    end
end

function Page:done(result)
    if self.modal then
        self.widget:done(result)
    elseif self.floating then
        self.widget:hide()
        self:perform('onExit')
    elseif self.popup then
        self.parent:exit(self)
    end
end
Page.exit = Page.done

--function Page.Exit()
--    print('Page.Exit', pageCurrent,  pageCurrent.ExitPage)
--    if pageCurrent then
--        if pageCurrent.modal then
--            pageCurrent.widget:done(1)
--        end
--        if pageCurrent.ExitPage then
--            pageCurrent.ExitPage:enter()
--        end
--    end
--end

function Page:update()
    self:perform('onUpdate')
    for _,c in ipairs(self) do
        c:update()
    end
    self.widget:update()
end

function Page:setFocus()
    self.widget:setFocus()
end

function Page:msgbox(...)
    MsgBox.Show(self, ...)
end
