local message = require 'message'
Page {
    name = 'Timecode',
    columns = 1,
--  delegate = TimecodeDelegate{ },
    delegate = true,
    constrain = { 0.75, 2 },
    Panel {
        h=0.55,
        columns = { 0.6, 0.4 },
        constrain = { 2, 4 },
        background = true,
--      margin = 0.05,
        --spacing = 0.2,
        Timecode { margin=0.1, background=0x003000, color=0x20F020, setting='timecode.reader.tc', id='ReaderTC' },      Label { "Reader T.C.", fit=16 },
        Timecode { margin=0.1, background=0x003000, color=0x20F020, setting='timecode.reader.ub', id='ReaderUB' },     Label { "Reader U.B.", fit=16 },
        Timecode { margin=0.1, background=0x300000, color='orangered', setting='timecode.generator.tc', id='GenTC' }, Label { "Generator T.C.", fit=16 },
        Timecode { margin=0.1, background=0x300000, color='orangered', setting='timecode.generator.ub', id='GenUB' }, Label { "Generator U.B.", fit=16 },
    },
    Panel {
        columns = 4,
        Button { "T.C. Out", setting='timecode.out' },
        Button { "T.C. Displayed", setting='timecode.displayed' },
--      Button { "Timecode Run Mode", id='RunMode', setting='timecode.runmode'},
        --Button { "Timecode|Run Mode", page='TimecodeRunMode'},
        Button { "Run Mode", setting='timecode.runmode'},
        Button { "Frame Rate", setting='timecode.framerate' },
        Button { "Enter|Timecode", id='EnterTC' },
        Button { "Enter|User Bits", id='EnterUB' },
        Button { "Auto Date Jam", setting='timecode.autojam' },
        Button { "Inc User Bits", setting='timecode.incuserbits' },
    },
    Panel {
        rows = 1,
        h = 0.15,
        Button { "JAM T.C.", action='jam.tc' },
        Button { "JAM U.B.", action='jam.ub' },
        Button { "JAM Date", action='jam.date' },
        Button { "JAM Time", action='jam.time' },
    },
    -----------------------------------------------------
    onEnter_Page = function(self)
        message.subscribe('timecode.reader', function(msg, tc)
                self.ui.readertc:setText( tc:text() )
                self.ui.readertc:update()
                self.ui.readerub:setText( tc:ubText() )
                self.ui.readerub:update()
            end)
        message.subscribe('timecode.generator', function(msg, tc)
                self.ui.gentc:setText( tc:text() )
                self.ui.gentc:update()
                self.ui.genub:setText( tc:ubText() )
                self.ui.genub:update()
            end )
    end,

    onClick_EnterTC = function(self)
            self.ui.gentc:onClick()
        end,
    onClick_EnterUB = function(self)
            self.ui.genub:onClick()
        end,

}
--[[
Page {
    name = 'TimecodeRunMode',
    popup = true,
--  exclusive = true,
    delegate = true,
    constrain = { 1, 2 },
    Panel {
        background = true,
        style = 'msgbox.panel',
        margin = 0.05,
        Title { 'Timecode Run Mode', align='center', w=1.0 },
        Panel {
            margin = 0.1,
            CheckedGroup {
                h = 0.6,
                setting = 'timecode.runmode',
                style='button.flat',
            },
            Spacer { h=0.15 },
            Button { margin={0.2,0.2,0.2,0}, 'Close', id='Close', background='green' }
        },
    },
--  onEnter = function(self) self:traverse( function(o) o:update() end ) end,
    onClick_Close = function(self) self:exit() end
}
--]]


