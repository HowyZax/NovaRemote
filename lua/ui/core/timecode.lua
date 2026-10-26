
--------------------------------------------------------------------
-- Timecode control
--
-- Represents a timecode display
--------------------------------------------------------------------

Timecode = Control:subclass('Timecode')
Timecode.color = 'black'
Timecode.align = 'center'
Timecode.style = 'timecode'
Timecode.font = 'timecode'
function Timecode:__init(items)
    super(Timecode).__init(self, items)
    self.title = items[1]
    self.setting = SettingFind(items.setting)
    if not self.setting then
        self.setting = Setting{ timecode=true }
    end
--  self.prevtext = ''
    self:setText("12:34:56:00")
end

function Timecode:onTouchRelease()
    self:onClick()
end

