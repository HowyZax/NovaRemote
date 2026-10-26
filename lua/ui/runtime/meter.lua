local floor = math.floor
local message = require 'message'
local config = require 'config'
local updateTimer = ElapsedTimer()

-- Following values are updates from settings
local METER_HOLD_TIME = 0.3
local METER_DECAY_TIME = 2.5 -- Time (secs) for main bars to decay from full-scale to zero

local PEAK_HOLD_TIME  = 1.5 -- Time (secs) to hold peaks
local PEAK_DECAY_TIME = 4.0 -- Time (secs) for peaks to decay from full-scale to zero

local METERS_TIMEOUT = 1.5  -- Time (secs) before setting all meter levels to zero due to comms timeout

local MeterPeakHold = class('MeterPeakHold')

function MeterPeakHold:__init()
--    self.level = 0
    self.peak = 0
    self.hold = 0
end

function MeterPeakHold:setLevel(level, holdTime)
    if     level < 0.0 then level = 0.0
    elseif level > 1.0 then level = 1.0
    end
--    self.level = level
    if level >= self.peak then
        self.peak = level
        self.hold = holdTime
    end
end

function MeterPeakHold:update(elapsed, decayTime)
    if self.hold > 0 then
        self.hold = self.hold - elapsed
--      if self.hold < 0 then self.hold = 0 end
    else
        self.peak = self.peak - (elapsed / decayTime)
--          self.peak = self.peak *0.995 --- - (elapsed / decayTime)
--      if self.peak < self.level then
--          self.peak = self.level
--      end
        if self.peak < 0 then
          self.peak = 0
        end
    end
end

MeterState = class('MeterState')
MeterState.ALL = { }
function MeterState:__init()
    self.level = MeterPeakHold()
    self.peak = MeterPeakHold()
    self.compress = MeterPeakHold()
    table.insert(MeterState.ALL, self)
end

function MeterState:setLevel(level)
    self.level:setLevel(level, METER_HOLD_TIME)
    self.peak:setLevel(level, PEAK_HOLD_TIME)
end

function MeterState:setCompressLevel(level)
    self.compress:setLevel(level, METER_HOLD_TIME)
end

function MeterState:update(elapsed)
    self.level:update(elapsed, METER_DECAY_TIME)
    self.peak:update(elapsed, PEAK_DECAY_TIME)
    self.compress:update(elapsed, METER_DECAY_TIME)
end

--local TestMeterState = MeterState()

local meterState = { }      -- hash of arrays of meter state; key = 'meters.in' etc
--local allMeterState = { }   -- array every named meter state. Does not include MeterState owned by a Meter.
do
    local function createMeterState(kind, n, timeoutId)
        local t = { timeout = 0 }
        for i = 1,n do
            local state = MeterState()
            table.insert(t, state)
            table.insert(MeterState.ALL, state)
        end
        meterState[kind] = t
    end
    createMeterState('meters.in',  config.nomad and 6+4+8 or 16)
--  createMeterState('meters.bus', config.nomad and 12 or 8)
    if config.ntracks then
        createMeterState('meters.bus', config.ntracks)
    end
    createMeterState('meters.out', 8)
    createMeterState('meters.mp3', 1)
    createMeterState('meters.bridge', 16)   -- for Meters page
    createMeterState('meters.input', 1)     -- for InputConfigure
end


local function get_meter_state(meter)
    local which = meter:index() or 1
    assert(meter.source)
    local source = meter.source
--  print('get_meter_state', source,  which, meterState[source][which])
    assert(source)
    assert(which)
    return meterState[source][which]
end

function MetersSetLevel(level)
    -- TODO: remove MetersSetLevel
end

local function MetersRandomize()
    local deltaRange = 0.1
    local delta = (math.random() * 2*deltaRange) - deltaRange
--  TestMeterState:setLevel(TestMeterState.level + delta)
end

local function onMetersReceiveCompressor(msg, meters)
    local state = meterState[msg:gsub('.compressor', '')]
    assert(state ~= nil)
    for i,m in ipairs(state) do
        m:setCompressLevel( meters[i] or 0. )
    end
    state.timeout = METERS_TIMEOUT
end

local function onMetersReceive(msg, meters)
    local state = meterState[msg]
    assert(state ~= nil)
    for i,m in ipairs(state) do
        m:setLevel( meters[i] or 0. )
    end
    state.timeout = METERS_TIMEOUT
end

message.subscribe('meters.in',  onMetersReceive)
message.subscribe('meters.bus', onMetersReceive)
message.subscribe('meters.out', onMetersReceive)
message.subscribe('meters.mp3', onMetersReceive)
message.subscribe('meters.bridge', onMetersReceive)
message.subscribe('meters.input', onMetersReceive)

message.subscribe('meters.in.compressor', onMetersReceiveCompressor)
message.subscribe('meters.bus.compressor', onMetersReceiveCompressor)
message.subscribe('meters.out.compressor', onMetersReceiveCompressor)
message.subscribe('meters.bridge.compressor', onMetersReceiveCompressor)
message.subscribe('meters.input.compressor', onMetersReceiveCompressor)
--[[
local function MetersUpdateTimeout(elapsed)
    for _,state in pairs(meterState) do
        if state.timeout > 0 then
            state.timeout = state.timeout - elapsed
            if state.timeout <= 0 then
                for _,m in ipairs(state) do
                    m:setLevel(0)
                    m:setCompressLevel(0)
                end
                state.timeout = 0
            end
        end
    end
end
--]]
local haveMeterSettings
local function MetersUpdate()
    local elapsed = updateTimer:restart()

    if haveMeterSettings == nil then
        haveMeterSettings = (SettingFind('meters.meter.hold', false) ~= nil)
    end

    if haveMeterSettings then
        METER_HOLD_TIME  = settings.meters.meter.hold
        METER_DECAY_TIME = settings.meters.meter.decay
        PEAK_HOLD_TIME   = settings.meters.peak.hold
        PEAK_DECAY_TIME  = settings.meters.peak.decay
    else
        METER_HOLD_TIME  = 0.30
        METER_DECAY_TIME = 1.25
        PEAK_HOLD_TIME   = 1.50
        PEAK_DECAY_TIME  = 4.00
    end

    --MetersUpdateTimeout(elapsed)

    for _,state in ipairs(MeterState.ALL) do
        state:update(elapsed)
    end
    for _,meter in ipairs(allMeters) do
        if meter:isVisible() then
            meter:update()
        end
    end
end

message.subscribe('meters.update', MetersUpdate)

local function PaintMeterMarkers(painter, segments, wantText, vertical, fontscale)
    if wantText == nil then wantText = true end

    if wantText then
        if vertical then
            painter:fitFont('MMM', painter:rect().w, painter:rect().h//5, 0.95)
        else
            painter:fitFont('MMM', painter:rect().w//5, painter:rect().h, fontscale or 0.5)
        end
    end

    for i,segment in ipairs(segments) do

        local r = segment.rect
        local text = wantText and segment.text
        local x,y = r.x, r:vcenter()
        if vertical then x,y = r:hcenter(), r:bottom() end

        if segment.marker then
            if vertical then
                for x=0,r.w,4 do painter:box(Rect(x,y,1,1), 'lightgray') end
            else
                for y=0,r.h,4 do painter:box(Rect(x,y,1,1), 'lightgray') end
            end
        end

        if text then
            local wtext, htext = painter:textBounds(text)
            local xtext, ytext = x, y
            if     segment.align == 'left'  then xtext,ytext = x + wtext/2, y - htext/2
            elseif segment.align == 'right' then xtext,ytext = x - wtext/2, y + htext/2
            end
            if vertical then xtext = x
            else             ytext = y
            end
            local rtext = Rect(xtext-wtext/2,ytext-htext/2, wtext, htext)
            --painter:fillRect(rtext:expand(1), 'white')
            painter:text{text, rect=rtext}
        end
    end
end

-- Translate a rectangle in a horizontal meter to corresponding
-- rectangle in a vertical meter.
local function translateToVertical(r, rect)
    return Rect(r.y, rect.w - r.x - r.w, r.h, r.w)
end

-- Build table of segments/regions in the meter.
local function MeterSegments(r1, vertical)
    local tbl = { }
    local n = 4
    local dbrange = 40
    -- Compute segments assuming a horizontal meter
    local rect = r1:copy()
    if vertical then rect.w, rect.h = rect.h, rect.w end
    do
        local r = rect:copy()
        for i = 1,n do
            local segment = { }
            local x = floor(i*rect.w/n)
            assert(x >= r.x)
            r.w = x - r.x
            segment.rect = r:copy()
            local db = (i-n-1) * dbrange/n
            segment.text = tostring(db)
            segment.marker = true
            if i == 1 then segment.align = 'left' end
            if     db >= -10 then segment.color = 'red'
            elseif db >= -20 then segment.color = 'yellow'
            else                  segment.color = 'green'
            end
            table.insert(tbl, segment)
            r.x = x
        end
        r.w = 0
        local segment = { }
        segment.rect = r:copy()
        segment.text = '0'
        segment.align = 'right'
        segment.marker = false
        table.insert(tbl, segment)
    end
    -- Translate rectangles for vertical meter
    if vertical then
        for _,segment in ipairs(tbl) do
            assert(segment.rect:isClean())
            segment.rect = translateToVertical(segment.rect, rect)
        end
    end

    return tbl
end


function Meter:onRender(painter, which)
    local r = painter:rect() --:adjusted(0,0,-1,-1)
    local segments = MeterSegments(r, self.vertical)
    if which == 'off' then
        painter:fill('black')
    elseif which == 'peak' then
        painter:fill('white')
    elseif which == 'compress' then
        painter:fill('white')
    else
        for i,segment in ipairs(segments) do
            if segment.color then
                painter:fillRect(segment.rect, segment.color)
            end
        end
    end

    if not self.vertical and self.strikethru then
        local r1 = r:shrink{0.02, 0.3}
        local step = r1.h/2
        r1.h = 1
        for i = 1,3 do
            painter:fillRect(r1, 'orangered')   -- tan
            r1.y = r1.y + step
        end
    end

    if self.label then
        painter:setPen('darkgray')
        painter:text{ self.label, x=0.04,y=0,w=r.w/2.,h=r.h, fit={10,}, font=0.9, align='left' }
    end
    painter:setPen('lightgray')
    PaintMeterMarkers(painter, segments, self.scale, self.vertical, 0.5)
end

function Meter:key(which, r)
    return which .. r.w .. (self.vertical and 'v' or 'h') .. (self.strikethru and 'x' or '-') .. r.h .. (self.label and self.label or '')
end

-- TODO: discard old images!!

function Meter:image(which, r)
    local key = self:key(which, r)
    self.img = self.img or { 0 }

    if self.img[1] > 1024 then
        print 'DISCARD METER IMAGE CACHE'
        self.img = { 0 }
    end

    if not self.img[key] then
        local img = Image.create(r.w, r.h)
        local painter = Painter.create(img)
        self:onRender(painter, which)
        self.img[key] = img
        self.img[1] = self.img[1] + 1
    end
    return self.img[key]
end

function Meter:getState()
    if self.source ~= nil then
        return get_meter_state(self)
    else
        if not self.state then
            self.state = MeterState()
        end
        return self.state
    end
 end

function Meter:onPaint(painter)
    local state = self:getState()
    if not state then return end
    local r = painter:rect()
    local level = state.level.peak
    local peak  = state.peak.peak
    local compress  = state.compress.peak
    if not self:isEnabled() then level, peak, compress = 0, 0, 0 end
    local function max(x, y) return x > y and x or y end
    local function vblit(which, offset, len, thickness)
        if len == nil then len = r.h - offset end
        local x,w = 0,r.w
        if thickness then x,w = (x+w - w*thickness)/2, w*thickness end
        offset = r.h - offset - len
        painter:image(x, offset, self:image(which, r), x, offset, w, len)
    end
    local function hblit(which, offset, len, thickness)
        if len == nil then len = r.w - offset end
        local y,h = 0,r.h
        if thickness then y,h = (y+h - h*thickness)/2, h*thickness end
        painter:image(offset, y, self:image(which, r), offset, y, len, h)
    end
    local blit = self.vertical and vblit or hblit
    local len = self.vertical and r.h or r.w
    local lenPeak = max(len * 0.015, 1)//1      -- size of the peak indicator
    local offset = floor(len * level);       -- position of end of illuminated meter
    local offsetPeak = floor(len * peak)     -- position of peak
    local offsetCompress = floor(len * compress)

    blit('on',  0,   offset)
    blit('off', offset)
    if offsetPeak then
        offsetPeak = offsetPeak - lenPeak   -- top (not bottom) of peak at peak level
        blit('peak', offsetPeak, lenPeak)
    end
    blit('compress', len - offsetCompress, len, 0.25)

end

function Meter:create(parent)
    --print('','', 'Meter.create in parent', parent.__classname)
    local w = widgets.create(self, 'widget', parent)
    if w.setVertical then w:setVertical(self.vertical) end
    return w
end

function Meter:index()
    if self.tag then return self.tag
    else return super(Meter).index(self)
    end
end


function MeterScale:onPaint(painter)
    painter:fill('black')
    painter:setPen('lightgray')
    PaintMeterMarkers(painter, MeterSegments(painter:rect(),  self.vertical), true, self.vertical, not self.vertical and 1.0    )
end

function MeterScale:create(parent)
    --print('','', 'MeterScale.create in parent', parent.__classname)
    local w = widgets.create(self, 'widget', parent)
    w:setVisible()
    return w
end

