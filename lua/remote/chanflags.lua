local _M = { }

local ASSIGNED  = 0x01
local FADER     = 0x02
local PHASE     = 0x04
local CHAN_SHIFT = 8

function _M.unpack(value)
    value = value or 0
    local cf = {
        chan = (value >> CHAN_SHIFT),
        assigned = (value & ASSIGNED) ~= 0,
        phase    = (value & PHASE) ~= 0,
        fader    = (value & FADER) ~= 0,    -- true if channel modified by a fader (i.e. post fade)
    }
    cf.chan = cf.assigned and cf.chan + 1 or 0  -- 1-based channel if assigned
    return cf
end
local pretty = require 'pl.pretty'
function _M.pack(t)
    local value = 0
    if t then
        local chan = t.chan or 0
        if chan >= 1 then chan = chan - 1 end -- stored channel is 0-based
        value = chan << CHAN_SHIFT
        if t.assigned then value = value | ASSIGNED end
        if t.phase    then value = value | PHASE end
        if t.fader    then value = value | FADER end
    end
    print('PACK:',  value,  pretty.write(t, ''))
    return value
end

return _M

