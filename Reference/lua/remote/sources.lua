-- Work in progress - intend to use this more in future!

local _M = { }

local pretty = require 'pl.pretty'
local config = require 'config'

-- Describe source configuration

local sources
if config.nova then
    -- Nova hardware supports 8 outputs, although only 6 currently used.
    sources = {
        { name="Analog",   n=4, shortname='IN',      abbrv='a',    graphic='sine',  trim=true, },       -- input
        { name="Return",   n=2, shortname='RET',     abbrv='r',    graphic=nil,     trim=true, },       -- input
        { name="Digital",  n=8, shortname='DIG',     abbrv='d',    graphic='pulse', trim=true, },       -- input
        { name="Receivers",n=8, shortname='RX',      abbrv='x',    graphic=nil,                },       -- input
        { name="Output",   n=8, shortname='OUT',     abbrv='o',    graphic=nil,     color='outputmix'}, -- bus
      --{ name="Zaxnet",   n=30, shortname='ZNET',   abbrv='z',    graphic=nil,     },
      --{ name="Comm",     shortname='COMM',    abbrv='c',    graphic=nil,     },            -- (slate, talk1,talk2, TONE)
        { name="Track",    n=config.ntracks, shortname='TRK', abbrv='t', graphic=nil,  color='plum'},    -- bus
      --{ name="Dante",    shortname='DNT',     abbrv='e',    graphic='dante', },                -- input
    }
elseif config.nomad then
    sources = {
        { name="Analog",  n=6,  shortname='IN',      abbrv='a',    graphic='sine',  trim=true, },       -- input
        { name="Return",  n=4,  shortname='RET',     abbrv='r',    graphic=nil,     trim=true, },       -- input
        { name="Digital", n=8,  shortname='DIG',     abbrv='d',    graphic='pulse', trim=true, },                  -- input
        { name="Output",  n=6,  shortname='OUT',     abbrv='o',    graphic=nil,     color='outputmix'}, -- bus
        --{ name="Zaxnet",  n=30, shortname='ZNET',    abbrv='z',    graphic=nil,     },
        --{ name="Comm",    shortname='COMM',    abbrv='c',    graphic=nil,     },            -- (slate, talk1,talk2, TONE)
        { name="Track", n=config.ntracks, shortname='TRK', abbrv='t', graphic=nil,  color='plum'},    -- bus
        --{ name="Dante",   shortname='DNT',     abbrv='e',    graphic='dante', },                -- input
    }
else
    sources = {
        { name="Analog",  n=16, shortname='IN',      abbrv='a',    graphic='sine',  },                -- input
        --{ name="Return",  n=0,  shortname='RET',     abbrv='r',    graphic=nil,     },                -- input
        { name="Digital", n=16, shortname='DIG',     abbrv='d',    graphic='pulse', },                -- input
        --{ name="Output",  n=6,  shortname='OUT',     abbrv='o',    graphic=nil,     color='outputmix'},    -- bus
        --{ name="Zaxnet",  n=99, shortname='ZNET',    abbrv='z',    graphic=nil,     },    -- trim
        --{ name="Comm",    shortname='COMM',    abbrv='c',    graphic=nil,     },            -- (slate, talk1,talk2, TONE)
        --{ name="Track",   shortname='TRK',     abbrv='t',    graphic=nil,     color='plum'},    -- bus
        --{ name="Dante",   shortname='DNT',     abbrv='e',    graphic='dante', },                -- input
    }

end

-- Abbreviations for input types that are configurable in InputConfigure page
local inputConfigurable = 'ardx'

-- Prefix for InputConfigure page
local inputConfigurePrefix = {
    a = 'ANALOG',
    r = 'RET',
    d = 'DIGITAL',
    x = 'RX'
}

-- Generate table of unique source identifiers for fast translation between input
-- descriptions and its unique ID.

local srcs = { }
do
    local i = 1
    for _,source in ipairs(sources) do
        srcs[source.abbrv] = source
        source.first = i
        for j = 1,source.n do
            print (source.abbrv, j, i)
            srcs[i] = source.abbrv
            i = i + 1
        end
    end
end

pretty.dump(sources)
--pretty.dump(srcs)

local function sourceInfo(chan)
    local source = srcs[ srcs[chan] or '' ]
    if source then
        return source, (chan - source.first + 1)
    end
    return
end
--
-- chan is 1-based
--
local function inputAbbrev(chan) 
    assert(type(chan) == 'number')
    return srcs[chan] or ''
end

function _M.isInput(chan)           return inputAbbrev(chan) ~= ''  end
function _M.isAnalogInput(chan)     return inputAbbrev(chan) == 'a' end
function _M.isReturnInput(chan)     return inputAbbrev(chan) == 'r' end
function _M.isDigitalInput(chan)    return inputAbbrev(chan) == 'd' end
function _M.isZaxnetInput(chan)     return inputAbbrev(chan) == 'z' end
function _M.isReceiverInput(chan)   return inputAbbrev(chan) == 'x' end
function _M.isOutput(chan)          return inputAbbrev(chan) == 'o' end


local function sourceCount(kind)
    local source = srcs[kind]
    return source and source.n or 0
end

function _M.numberOfAnalogInputs()  return sourceCount('a') end
function _M.numberOfReturns()       return sourceCount('r') end
function _M.numberOfDigitalInputs() return sourceCount('d') end
function _M.numberOfZaxnetInputs()  return sourceCount('z') end
function _M.numberOfReceiverInputs()return sourceCount('x') end

function _M.numberOfTracks()        return config.ntracks   end
function _M.numberOfOutputs()       return config.noutputs  end

function _M.numberOfSources()       return #srcs            end

-- Return the unique input id (index) of specified input
-- @param s   single-letter input abbreviation (see source.abbrv)
-- @param i   channel number eg. 3 , 1-based
-- @return index, or nil
function _M.inputIndex(s, i)
    assert(i > 0)
    assert(#s == 1)
    local abbrev = s:lower()
    local source = srcs[ abbrev ]
    if source then
         assert(i <= source.n, 'Source out of range')
         return source.first-1 + i              
    end                  
    error 'Unknown input'
end

-- @param which  input id, 1-based
function _M.inputDescriptionWithGraphic(chan)
    local source, idx = sourceInfo(chan)
    if source then
        if source.graphic then
            return tostring(idx), source.graphic
        else
            return source.shortname .. idx
        end
    end
end

-- Check if source supports trim operation
-- @param which  input id, 1-based
function _M.inputHasTrim(which)
    local source = sourceInfo(which)
    return source and source.trim
end

function _M.inputConfigureCount()
    local n = 0
    for _,source in ipairs(sources) do
        if inputConfigurable:find(source.abbrv) then
            n = n + source.n
        end       
    end
    return n
end

-- Get the raw input name for source channel, as used
-- in InputConfigure
function _M.inputNameRaw(chan)
    local source, idx = sourceInfo(chan)
    if source then
        local prefix = inputConfigurePrefix[source.abbrv]
        if prefix then
            return prefix .. ' ' .. tostring(idx)
        end
    end
    return '?' + tostring(chan)
end

return _M

