local message = require 'message'

local _M = Page {
    name = 'PopupMsgBox',
    title = 'Message',
    floating = true,
    delegate = true,
    constrain = { 5/3, 8/3 },
    Panel {
--      background = true,
--      background = 'crimson', --'darksalmon', --0x20ffff,
        background = 'slategray', --'darksalmon', --0x20ffff,
        --style = 'msgbox.panel',
        columns = 1,
        margin = 0.05, --15,
        Spacer { h = 0.1 },
        Control {
            constrain=1,
            h = 0.2,
            elements = LayoutElement {
                GraphicElement { graphic='info-circle' }
            }
        },
        Panel {
            rows = 1,
            Spacer { w=0.1 },
            --Label { "Text message goes here!", fit=30, align='center', color='gold', id='Text' },
            Label { "Text message goes here!", fit={20,30}, align='center', id='Text' },
            Spacer { w=0.1 },
        },
        Panel {
            h = 0.2,
            Panel {
                rows = 1,
                Spacer { w=0.2 },
                Button { "OK", style='button.highlight', background='darkgreen',id='Ok' }, --0x217F21 },
--              Button { "OK", style='button.highlight',id='Ok' }, --0x217F21 },
                Spacer { w=0.2 },
            },
        },
        Spacer { h = 0.1 },
    },
}

function _M:onCreatedPage()
    assert(not self.timer)
    self.timeout = self.timeout or 0
    self.timer = ElapsedTimer()
    self.timer:cancel()
    message.subscribe('timer.tick', function()
            if self.timeout > 0 and self.timer:elapsed() > self.timeout then
                self:done()
            end
        end)
    message.subscribe('popup.msg', function(_, opts)
            self:show(type(opts) == 'table' and opts or { opts })
        end)
    self.wantAnimation = true
end

function _M:onEnterPage()
    self.wantAnimation = false
end

function _M:onExitPage()
    self.wantAnimation = true
end

function _M:onClickOk()
    self:done()
end

function _M:onClosePage()
    self:done()
end

function _M:show(opts)
    local timeout = opts.timeout or 0
    local text = opts[1] or ''
    self.timeout = timeout
    self.timer:restart();
    self.ui.text:setText(text)

    print('PopupMsgBox:show',  self.visible, self:isVisible())

    self.savedGeometry = Rect(Desktop.availableGeometry()):shrink(0.3)
    --self.savedGeometry = Rect(Desktop.availableGeometry()):shrink{0.3, 0.6,0.3,0}
    if self.wantAnimation then
        self:setGeometry(self.savedGeometry)
--        self:enter{
----          animateFrom = Rect(0,0,0,0),
--            animateFrom = self.savedGeometry, --Rect(0,0,0,0),
--            animateTo = self.savedGeometry
--            }
        self:enter()
    else
        self:update()
    end
end

