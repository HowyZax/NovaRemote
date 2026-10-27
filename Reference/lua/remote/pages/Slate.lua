local message = require 'message'
local config = require 'config'

local SlateTitle = Control:subclass('SlateTitle')
SlateTitle.margin = 0
SlateTitle.background = 'white'
SlateTitle.color = 'black'
SlateTitle.page = 'Home'

function SlateTitle:__init(items)
    if not items.elements then
        items.elements = LayoutElement {
            rows = 1,
            margin = 0,
--          TextElement{ text="Zaxcom", align='left', margin = 0.01 }
            GraphicElement{ graphic="smptebars", margin=0 },
            GraphicElement{ graphic="logo", w=0.25, margin = 0.01 },
        }
    end
    super(SlateTitle).__init(self, items)
end

function SlateTitle:onTouchRelease()
    self:onClick()
end

function SlateTitle:onClick()
    if self.page then
        Page.Enter(self.page, self:findParentByClass(Page))
    end
end

local SlateLabel = Control:subclass('SlateLabel')
SlateLabel:attributes{ 'align' }
SlateLabel.margin = 0
SlateLabel.background = 'white'
SlateLabel.color = 'black'
--SlateLabel.style = 'button'
function SlateLabel:onTouchRelease()
    self:onClick()
end

function SlateLabel:onClick()
    if self.page then
        Page.Enter(self.page, self:findParentByClass(Page))
    end
end

function SlateLabel:__init(items)
    if not items.elements then
        items.elements = LayoutElement {
            columns = 1,
            TextElement { h=0.2, margin=0.03, text=items[1] or '', align='left', background='white', color='darkgray' },
            TextElement {        margin=0.03, text=items[2] or '', align='center', background='white', color='black', id='value' },
        }
    end
    super(SlateLabel).__init(self, items)
end

function SlateLabel:onPaint(painter)
    painter:box()
    super(SlateLabel).onPaint(self, painter)
end

function SlateLabel:onUpdate()
    local text = self:valueAsText()
    if text then self:setText(text) end
end

function SlateLabel:setText(s)
    self.ui.value:setText(s)
end

local _M = Page {
    name = 'Slate',
    title = 'Timecode Slate',
    columns = 1,
    delegate = true,
    SlateTitle {
        h = 0.1,
--      "Zaxcom",
        -- TODO: logo and color bars.
    },
    Panel {
        h = 0.3,
        columns = 1,
--      background = 'white',
--      style = 'button.flat',
        Timecode { background=0x300000, color='orangered',
                        setting='timecode.generator.tc', id='Timecode',
                        onClick=function() end },
    },
    Panel {
        rows = 1,
        SlateLabel { "SCENE",   setting='scene.current',        page='SceneTake' },
        SlateLabel { "TAKE",    setting='take.current', w=0.4,  page='SceneTake' },
    },
    Panel {
        rows = 1,
        SlateLabel { "CAMERA",  setting='camera.name', w=0.3, page='SetupMenu' },
        --SlateLabel { "FILE",    "000002.WAV" },
        SlateLabel { "DATE",    "03/20/15", id='Date' },
    },

    onEnterPage = function(self)
        message.subscribe('timecode.generator', function(msg, tc)
            self.ui.timecode:setText( tc:text() )
            self.ui.timecode:update()
        end )
        message.subscribe('timer.second', function()
            self.ui.date:update()
        end )
    end,
    onUpdateDate = function(self, label)
        label:setText( os.date('%x') )
    end,

}

_M.fullscreen = true


