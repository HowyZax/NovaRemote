local message = require 'message'
local config = require 'config'
local recorder = config.recorder and require 'recorder'

local nTrackMeters, nOutMeters = config.ntracks, config.noutputs

local mp3Transport = config.recorder    -- true for local control of MP3 recorder

local Button1Line = Button:subclass('Button1Line')
function Button1Line:__init(items)
    if not items.elements then
--      items.elements = LayoutElement {
--          rows = 1,
--          TextElement { text=items.text or items[1], id='.text', color='white', w=0.3, fit={4,} },
--          TextElement { id='.value', color='gold', fit=items.fit or {7,}, align='left' },
--      }
        items.elements = LayoutElement { columns = 1 }
        if items.graphic then
            items.elements:insert( GraphicElement { graphic=items.graphic, x=6,y=2, h=0.25, constrain={1,1,align='left'}, margin=0 } )
        else
            items.elements:insert( TextElement { text=items.text or items[1], id='.text', color='white', x=6,y=2, h=0.25, align='left', fit=true, margin=0, } )
        end
        items.elements:insert( TextElement { id='.value', color='gold', fit=true, margin={0.1, 0.05} } )
    end
    super(Button1Line).__init(self, items)
--  self.fit = items.fit
end

function Button1Line:setContent(options)
    if options.text then
        local c = self.elements:findById('.text')
        if options.text then c:setText(options.text) end
    --  if options.textAlign then c.align = options.textAlign end
    end

    local c = self.elements:findById('.value')
    if options.value then c:setText(options.value) end
    if options.color then c:setColor(options.color) end
    if options.background then self:setBackground(options.background) end

    self:layout()
end

--[[
function Button1Line:onPaint(painter)
    local _,_,w,h = painter:rect():unpack()
    local c = self.elements:findById('.text')
    painter:text{ x=6,y=2,w=w,h=h/4., fit=true, c.text, align='left' }
    c = self.elements:findById('.value')
    local r = Rect(0, 0.25*h, w, 3*h/4.)
    r = r:shrink{ 0.2, 0.05 }
    painter:text{ rect=r, fit=true, font=0.8, c.text, color='gold', background='red' }
end
--]]

local function MeterPanel()
    local nmeters = nTrackMeters
    local metersrc = 'meters.bus'
    local panel = Panel { columns = 1 }
    do
        local meterpanel = Panel {
            margin = 1,
            columns = 1,
            id = 'MeterPanel',
--          MeterScale { margin = 1 },
        }
        meterpanel:insert( MeterScale { margin = 1, h=1.0/(1.4*nmeters) } )
        for i = 1,nmeters do
            meterpanel:insert( Meter { margin = 1, source=metersrc, tag=i } )
        end
        panel:insert(meterpanel)
    end

    panel:insert( Panel {
        rows = 1, h = 0.1,
        Button { setting='home.meters.source', id='MeterSelect' },
    } )
    return panel
end

--{{{ Home1 - with meters at right - manual layout
local _M = Page {
    name = 'Home',
    delegate = true,
    depends = {
        'scene.current',
        'take.current',
        'note.current',
        'sample.rate',
        'input.name', 'input.color', 'trackmix.preset', 'trackmix.matrix', 'trackmix.isomatrix',
        'track.name', 'track.color',
        'hpmix.names', 'hpmix.preset', 
        'record.tracks',
        'home.meters.count'
    },
    --margin = 0,
    columns = 1,
    Panel {
        rows = 1,
        margin = 0,
        Panel {
            --margin = 0,
            Panel {
                h = 0.15,
                rows = 1,
                Button { 'STOP', fit=4, w=0.5, page='MainMenu', id='StatusButton' },
                Button1Line { 'Headphone', --page='HeadphoneMix',
                    id='Headphone',
                    graphic='headphones',
                },
                Button1Line { 'Battery', setting='battery.status' },
            },
            Timecode { margin=0, '00:00:00:00', background=0x000020, color=0xF0F0FF, id='timecode', page='Timecode' }, -- disk
            Panel {
                rows = 1,
                h = 0.15,
                Button {
                    'Drive', id='Drive', align='left',
                    elements = LayoutElement {
                        margin = 0,
                        LayoutElement {
                            columns = 1,
                            margin = not config.nomad and {0.2, 0 } or nil,
                            LayoutElement {
                                rows = 1,
                                TextElement { w = 0.4, text = 'TLOC', fit = 5, },
                                TextElement {
                                    text = '000:00:00',
                                    align = 'left',
                                    color = 'white',
                                    id = 'remaining',
                                    font = 'timecode.small',
        --                          background = 'green',
                                },
                            },
                            LayoutElement {
                                rows = 1,
                                TextElement {
                                    w = 0.4,
                                    text = 'Elapsed',
                                    fit = 7,
                                    color = 'white',
                                },
                                TextElement {
                                    text = '000:00:00',
                                    align = 'left',
                                    font = 'timecode.small',
                                    color = 'darkgray',
        --                          background = 'blue',
                                    id = 'elapsed',
                                },
                            },
                        },
                    },
                },
                config.recorder and Panel {
                    rows = 1,
                    margin = 0,
                    w=0.4,
                    conditional = config.nomad,
                    Button1Line { 'MP3', id='MP3Rec' },
                    Meter { w=0.15, margin=0.02, vertical=true, source='meters.mp3' },
                } or Spacer { w=0 },
            },
            Panel {
                rows = 1,
                h = 0.1,
                margin = 0,
                Button1Line { 'SR', setting='sample.rate', page='SetupMenu' },
                Button1Line { 'FR', setting='timecode.framerate', page='Timecode' },
                Button1Line { 'Pre Record', setting='record.preroll', page='SetupMenu' },
            },
            Panel {
                rows = 1,
                margin = 0,
                Button1Line { 'Cur',     setting='store.current' }, --, page='Cue'  },
                Button1Line { 'Segs',    setting='store.segment' },
                Button1Line { 'Folder',  setting='store.folder'   },
            },
            Panel {
                rows = 1,
                margin = 0,
                Button1Line { 'Scene', w=0.6, setting='scene.current', page='SceneTake' },
                Button1Line { 'Take', setting='take.current', page='SceneTake' },
            },
            Button1Line { 'Note', setting='note.current', fit=20, page='Note' },
        },
        MeterPanel{ },
    },
    Label { h=0.06, margin=0, setting='status.text', font='status', id='Status'  },
}
--}}}

function _M:onUpdate_Drive(button)
    button.ui.remaining:setText(settings.drive.time.remaining)
    button.ui.elapsed:setText(settings.drive.time.elapsed)
end

function _M:onEnter_Page()
    message.subscribe('timecode.generator', function(msg, tc)
            self.ui.timecode:setText( tc:text() )
            self.ui.timecode:update()
        end)

    message.subscribe('transport.status', function(msg, status)
            local text, color, bkg, tcbkg, tccolor
            if status == 'play' then
                text, tcbkg, tccolor = 'PLAY', 0x003000, 0x20F020
            elseif status == 'record' then
                text, tcbkg, tccolor = 'REC', 0x300000, 'orangered'
            else
                text, tcbkg, tccolor, color, bkg = 'STOP', 0x000020, 0xF0F0FF, 'white', 'black'
            end
            bkg = bkg or tcbkg

            local button = self.ui.statusbutton
            button:setText(text)
            button:setColor(color or tccolor)
            button:setBackground(bkg)
            button:update()

            local tc = self.ui.timecode
            tc:setBackground(tcbkg)
            tc:setColor(tccolor)
            tc:update()

            if mp3Transport and self.ui.mp3rec and settings.mp3record.mode == 0 then
                if status == 'record' then recorder:record()
                else  recorder:recordstop()
                end
            end

        end)
    SettingNotify('home.meters.source', self.onChanged_MeterSource, self)
    SettingNotify('home.meters.count', self.onChanged_MeterSource, self)
    self:onChanged_MeterSource()

    SettingNotify('drive.time.elapsed', function() self.ui.drive:update() end )
    SettingNotify('drive.time.remaining', function() self.ui.drive:update() end )

    if self.ui.mp3rec then
        message.subscribe('mp3.recorder.status', function(msg, status)
                self.ui.mp3rec:update()
            end)
		if recorder then
			recorder:start()       -- Start streaming MP3 recorder, for metering
		end
    end

    SettingNotify('record.tracks', function() self.ui.meterpanel:update() end )

    message.subscribe('serial.online', function(msg, status)
            self.online = status
            self.ui.status:update()
    end )

end

function _M:setMeterSource(source, nmeters)
    for _,c in ipairs(self.ui.meterpanel:findByClass(Meter)) do
        c.source = source
        c:setVisible(c.tag <= nmeters)
        if source == 'meters.out' then
            c.label = 'OUT' .. ' ' .. c.tag
            c.strikethru = false
        else
            c.label = TrackDescription(c.tag)
            c.strikethru = (SettingFind('record.tracks'):value(c.tag) == 0)
        end
    end
    self.ui.meterpanel:layout()
end

function _M:onChanged_MeterSource()
    local which = Setting('home.meters.source'):value()
    if which == 0 then
        self:setMeterSource('meters.bus', settings.home.meters.count)
    else
        self:setMeterSource('meters.out', nOutMeters)
    end
end

function _M:onUpdate_MeterPanel()
    self:onChanged_MeterSource()
end

function _M:onUpdate_MP3Rec(c)
    local value, bkg, color = 'STOP', 'black', 'white'
    if recorder and recorder.status == 'record' then
        value, bkg, color = 'REC', 0x300000, 'orangered'
    end
    c:setContent{
        value = value,
        color = color,
        background = bkg,
    }
end

function _M:onUpdate_Headphone(c)
    local s = SettingFind('hpmix.names'):text(settings.hpmix.preset)
    if not s or s == '' then
        s = 'HP ' .. settings.hpmix.preset
    end
    c:setContent{
        value = s
    }
end

if mp3Transport then
    function _M:onClick_MP3Rec(c)
        if settings.mp3record.mode ~= 0 then
            if recorder.status == 'record' then recorder:recordstop()
            else recorder:record()
            end
        end
        c:update()
    end
else
    function _M:onClick_MP3Rec(c)
        if recorder then
            message.send('action', recorder.status == 'record' and 'mp3.stop' or 'mp3.rec')
        end
    end
end

local hphTimer = ElapsedTimer()
function _M:onTouchPress_Headphone(c)
    hphTimer:restart()
end
function _M:onTouchRelease_Headphone(c)
    if hphTimer:elapsed() > 500 then
        settings.hpmix.preset = settings.hpmix.preset - 1
    else
        settings.hpmix.preset = settings.hpmix.preset + 1
    end
end
function _M:onRightClick_Headphone(c)
    settings.hpmix.preset = settings.hpmix.preset - 1
end

function _M:onUpdate_Status(c)
    if self.online then
        c:setBackground(0x606060)
        c:setText(c:value())
    else
        c:setBackground('red')
        c:setText('Offline')
    end

end
function _M:onUpdate_MeterSelect(c)
    local setting = SettingFind('home.meters.source')
    local text = setting:text()
    if setting:value() == 0 then
        -- TRACK xx
        text = text .. ' 1-' .. settings.home.meters.count
    else
        -- OUT xx
        text = text .. ' 1-' .. config.noutputs
    end
        c.ui['.value']:setText(text)
end

