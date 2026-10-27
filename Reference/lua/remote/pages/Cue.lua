local message = require 'message'
Page {
    title = 'Cue',
    delegate = true,
    columns = 1,
    constrain = { 0.75, 2 },
    Panel {
        Panel {
            h = 0.35,
            margin = 0.05,
            columns = { 0.6, 0.4 },
--          constrain = { 2, 4 },
            background = true,
--          margin = 0.05,
            Timecode { margin=0.1, background=0x282800, color=0xFFFBE6, setting='timecode.reader.tc', id='timecode' }, Button { margin=0.1, "Set Cue T.C." },
            Timecode { margin=0.1, background=0x282800, color=0xFFFBE6, setting='timecode.reader.ub', id='ubits'    }, Button { margin=0.1, "Set Cue U.B." },
        },
        Panel {
            rows = 1,
            h = 0.4,
            background = true,
--          margin = { 0.2, 0.15, 0.2, 0.15 },
            margin = 0.08,
            Panel {
                rows = 1,
                margin = 0.1,
                Button { 'REW' },
                Button { 'PREV|SEG' },
                Button { 'NEXT|SEG' },
                Button { 'FFW'},
            }
        },
        Panel {
            rows = 1,
            Button { 'WIRELESS|AUDITION' },
            Button { 'WIRELESS|RE-RECORD' },
            Spacer { },
            Button { 'ENTER|SEGMENT' },
        },
    },

    onEnterPage = function(self)
        message.subscribe('timecode.generator', self.onTimecodeDisplay, self)
    end,

    onTimecodeDisplay = function(self, msg, tc)
        self.ui.timecode:setText( tc:text() )
        self.ui.timecode:update()
        self.ui.ubits:setText( tc:ubText() )
        self.ui.ubits:update()
    end,

}



