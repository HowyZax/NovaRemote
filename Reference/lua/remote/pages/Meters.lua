local config = require 'config'
local chanflags = require 'remote.chanflags'
local sources = require 'remote.sources'
local pretty = require 'pl.pretty'

local function MeterSourceIndex(control)
    do  -- Try meters.input to allow Nomad to control what is shown on meter bridge, including extra channels
        local setting = SettingFind('meters.source')
        local chan = setting:value(control.tag)
        if chan >= 0 then return chan end -- -1 of not assigned, 0 for disabled, 1-N for input source
    end
    do  -- Fallback to mixer.source (ie. FaderAssign routing)
        local setting = SettingFind('mixer.source')
        local index = { settings.mixer.bank+1, control.tag }
        local chan = setting:value(index)
        return chan
    end
end

InputNameInfoButton = InfoButton:subclass('InputNameInfoButton')
function InputNameInfoButton:__init(items)
    if not items.elements then
        items.elements = LayoutElement {
            --rows = 1,
            InputNameElement { id = '.text' },
        }
    end
    super(InfoButton).__init(self, items)
end


local function MeterBridge(items)
    local n = items.columns -- or 16

    local meterPanel = Panel {
        rows = 1,
        margin = 0,
        id = 'MeterBridge',
    }
    local function subIndex(n)
        return function(c)  return { MeterSourceIndex(c), n } end
    end
    for i = 1,n do
        table.insert(meterPanel,
            Panel {
                columns = 1,
                margin = 0,
                tag = i,
                InputNameInfoButton {
                    'INPUT NAME',  tag=i, id='InputName',
                    setting='input.name', index=MeterSourceIndex,
                    margin = { 0, 2 },
                },
                Panel {
                    rows = 1,
                    h = 0.6,
                    margin = 0,
                    Panel {
                        columns = 1,
                        margin = { 0.1,0,0,0},
                        id = 'LatchPanel',
                        tag = i,
                        LatchedButton { 'EQ',   fit=4, margin=0, tag=i, setting='input.eq', index=MeterSourceIndex, id='EQ', conditional=not config.nomad  },
                        LatchedButton { 'C/L',  fit=4, margin=0, tag=i, setting='input.compress.enable',   index=MeterSourceIndex, id='CL'   },
                        LatchedButton { 'HPF',  fit=4, margin=0, tag=i, setting='input.hpf.enable',  index=MeterSourceIndex, id='HPF'  },
                        LatchedButton { 'NTC1', fit=4, margin=0, tag=i, setting='input.notch.enable', index=subIndex(1), id='NTC1' },
                        LatchedButton { 'NTC2', fit=4, margin=0, tag=i, setting='input.notch.enable', index=subIndex(2), id='NTC2' },
                        LatchedButton {
                            id='Phase', tag=i, margin=0, setting='input.phase', index=MeterSourceIndex,
                            highlight=0xC00000,
--                          highlight='darkred',
                            elements = LayoutElement {
                                GraphicElement {
                                    graphic = 'phase',
                                    margin = 0.15,
                                    constrain = 1,
                                },
                            }
                        },
                        LatchedButton { 'DLY',  fit=4, tag=i, margin=0, setting='input.delay.enable', index=MeterSourceIndex, id='DLY' },
--                      LatchedButton { 'WARN', fit=4, margin=0, tag=i, setting='input.warning', index=MeterSourceIndex, id='WARN' },
                    },
--                  Meter { vertical = true, margin={0.2,0,0.1,0}, source='meters.in', tag=i, index=MeterSourceIndex },
                    Meter { vertical = true, margin={0.2,0,0.1,0}, source='meters.bridge', tag=i },
                },
                Panel {
                    columns = 1,
                    h = 0.3,
                    margin = 0,
                    InfoButton {
                        margin = 0, tag=i, id='InputButton',
                        setting='input.source', index=MeterSourceIndex,
                        fit={3},
                        --GraphicElement { w=0.4, margin={0.4,0.2,0,0.1}, },
                        --TextElement { fit=4, align='left' },
                        onClick = function(self) settings.input.channel = MeterSourceIndex(self) end,
                    },
                    InfoButton {
                        'ISO ??',  fit=6, margin = 0, tag=i, id='IsoButton',
                        -- setting='input.source', index=MeterSourceIndex,
                        onClick = doNothing,
                    },
                    LatchedButton {
                        string.format('%d', i), h=0.5, margin={0.15,0,0.15,0}, fit=2, tag=i, id='ChannelButton',
                        setting='mixer.enable', index=i
                    }
                }
            } )
    end

    return meterPanel
end

local _M = Page {
    name = 'Meters',
    --title = "Meters",
    delegate = true,
    depends = {
        'trackmix.isomatrix',
        'trackmix.matrix',
        'trackmix.preset',
        'meters.source',
        'mixer.source',
        'mixer.bank',
        'track.name', 'track.color',
        'input.name', 'input.color',
        'input.trim.mode',
        'input.warning',
    },
    Panel {
        columns = 1,
        margin = 0,
        MeterBridge { columns = 16 },
    },

}
function _M:onCreated_Page()
    SettingNotify('meters.count', self.onChangedMetersCount, self)
end

function _M:onEnter_Page()
    self:onChangedMetersCount()
end

function _M:onChangedMetersCount()
    local panel = self.ui.meterbridge
    local count = settings.meters.count
    for _,c in ipairs(panel) do
        c:setVisible(c.tag <= count)
    end
    self:layout()
end

function _M:onUpdate()
    local panel = self.ui.meterbridge
    for _,c in ipairs(panel) do
        c:setEnabled( MeterSourceIndex(c) > 0 )
    end
end

local isoTrackCache = nil
SettingNotify('trackmix.matrix', function() isoTrackCache = nil end )
SettingNotify('trackmix.preset', function() isoTrackCache = nil end )
local function isoTracks()

    if isoTrackCache then return isoTrackCache end

    local setting = SettingFind('trackmix.matrix')
    local tbl = setting:value(settings.trackmix.preset)
    local isocf = nil   -- chanflags with extra attribute track=i

    local chans = { }

    for i,trk in pairs(tbl or { }) do   -- iterate tracks in aribrary order
        local chan, val = nil, 0x00 -- assigned channel and associated routing value
        -- Check if this track has only one source assigned
        for src,x in pairs(trk or {}) do
            if x ~= 0 then
                if chan then    -- already have a channel?
                    chan = nil  -- yes, so must be multiple assignments
                    break       -- ... so stop considering this track
                end
                chan, val = src, x
            end
        end
        if chan then -- track has only one source assigned
            local cf = chanflags.unpack(val)
            assert(chan)
            assert(cf.assigned)
            cf.track = i    -- add extra attribute
            cf.chan = chan
--          cf.value = val
            -- Is the
            local other = chans[chan]   -- Should we override other track which has chan as its only source?
            if (not other)              -- ... this is the first track that has this source as its only source
                or (other.fader and not cf.fader)  -- or, other track has source post-fader; this track is pre-fader... so use this track
                or ((other.fader == cf.fader) and cf.track < other.track) -- or, choose lowest track number
            then
                chans[chan] = cf    -- store this track as the ISO for source chan
            end
        end
    end
    isoTrackCache = chans   -- Keep the result in cache
    print('isoTrackCache:')
    pretty.dump(isoTrackCache)
    print'---------------------'
    return isoTrackCache
end

local function isoTrackForSource(chan)
    local chans = isoTracks()
    local cf = chans[chan]
    return cf and cf.track or nil
end

function _M:onUpdate_IsoButton(button)
    local chan = MeterSourceIndex(button)
    local text = '-'
    if chan > 0 then
        if config.nomad then
            local trk = isoTrackForSource(chan)
            if trk then text = 'ISO ' .. trk end
        else
            local setting = SettingFind('trackmix.isomatrix')
            local tbl = setting:value(settings.trackmix.preset)
            for i,x in pairs(tbl or { }) do
                local cf = chanflags.unpack(x)
                if cf.assigned and cf.chan == chan then
                    text = 'ISO ' .. (i+8)
                    break
                end
            end
        end
    end
    button:setText(text)

end
-- Update input/track name button at top of channel strip
function _M:onUpdate_InputName(button)
    local chan = button:index()
    local text, color
    if config.nomad then
        local trk = isoTrackForSource(chan)
        if trk then
            text, color = TrackDescription(trk)
        end
    else
        text, color = InputName(chan)
    end

    if SettingFind('input.warning'):value(chan) > 0 then
        button.style = 'button.warning'
    else
        button.style = 'button'
    end

    button:setText(text)
    button:setBackground(color)

end

-- Update name button at bottom of channel strip
function _M:onUpdate_InputButton(button)
    local src = button:index()
    print('button:index',  src)
    local text, graphic
    if src and src > 0 then
        text, graphic = sources.inputDescriptionWithGraphic(src) --InputDescription(src) --InputName(chan)
    end
    button.fit = graphic and 3 or 6
    button:setGraphic(graphic)
    button:setText(text)
end

function _M:onUpdate_ChannelButton(button)
    local setting = SettingFind('input.trim.mode')
    local value = setting:value( MeterSourceIndex(button) )
    local enable = button:value()
    local color = nil
    if enable > 0 then
        color = value > 0 and 'dodgerblue' or LatchedButton.highlight
    end
    button:setBackground(color)
end

function _M:onUpdate_LatchPanel(panel)
    local chan = MeterSourceIndex(panel)
    panel:setEnabled(chan > 0 and not sources.isDigitalInput(chan))
end

