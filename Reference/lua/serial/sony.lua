-- Howy: These should be all the possible old Sony commands that I should ever
-- see from a Mix8 or an FP8 (or maybe rx12 via en embedded cQOPT cmd) these
-- packets do not have an AA header byte and are kind of stupid.

local _M = { }
local parse = require 'serial.parse'

local command = {
    SONY_VERSION = 0x7A29, -- version
    SONY_FADER   = 0x2225, -- fader
    SONY_KEY     = 0x2224, -- mix12 key press
    SONY_ENCODER = 0x2226, -- encoder
    SONY_STOP    = 0x2000, -- transport key from MIX12 only "stop cmd\n"
    SONY_PLAY    = 0x2001, -- transport key from MIX12 only "play cmd\n"
    SONY_RECORD  = 0x2002, -- transport key from MIX12 only "record cmd
    SONY_METER   = 0x6022, -- meter sense command
    SONY_TIME    = 0x610c, -- time sense
    SONY_STATUS  = 0x6120, -- status sense
    SONY_QOPT    = 0x7B2B, -- embedded QOPT command

    SONY_REQUEST        = 0x2129, -- + 0xFF request version; + 0x1C request faders
    SONY_SET_PEAKS      = 0x7C23,
    SONY_SET_BRIGHTNESS = 0x2126,
    SONY_SET_LIGHTS     = 0x7128,

}

_M.command = command

local commandNames = { }
for k,v in pairs(_M.command) do
    commandNames[v] = k
end

function _M.cmdname(x)
    return commandNames[x] or 'SONY_UNKNOWN'
end

-- Look for packet at start of buffer
-- return: nil if no valid packet
--         0 if partial packet
--         n (>0) if processed n-bytes
function _M.parser(buf, fn)
    if #buf >= 3 then
        local count = buf:byte(1) & 0xF
        local len = 3 + count
        if #buf >= len + count then
            local cmd = string.unpack('>H', buf)
            local sum = 0
            for i = 1, len-1 do
                sum = sum + buf:byte(i)
            end
            sum = sum & 0xFF
            if sum == buf:byte(len) or  -- Correct checksum?
                    (buf:byte(1)==0x7A and sum==0x07) then -- or, allow bad Mix8 version command through (checksum is wrong)
                local packet = buf:sub(1, len)
                buf = buf:sub(len+1)
                local data = packet:sub(3, len-1)
                --print(#packet, string.format('%04X', cmd), 'OK')
                local result,msg = pcall(fn, cmd, data, packet)
                if not result then
                    print('sony.parse failed:', msg)
                end
                return len, (not result)
            else
                --print(string.format('sony.parse: Checksum BAD, cmd:%04X 0x%02X (0x%02X)', cmd, sum, buf:byte(len)))
                return
            end
        end
    end
    return 0
end

--
-- Parse incoming packet
--
-- Modifies buf string containing incoming data stream
--
-- Note that Sony-style version requests can be appear on the same serial stream.
--
-- @return: packet or nil; true if positively identified protocol; number of errors
local parsers = { _M.parser }
function _M.parse(buf, fn)
    return parse.parse(buf, fn, parsers)
end

function _M.pack(cmd, data)
    data = data or ''
    local count = (cmd & 0x0F00) >> 8
    assert(count == #data)
    local packet = string.pack('>H', cmd)
    packet = packet .. data
    local sum = 0
    for i = 1, #packet do
        sum = sum + packet:byte(i)
    end
    packet = packet .. string.pack('B', sum & 0xFF)
    return packet
end

function _M.packVersionRequest()
    return _M.pack(0x2129, '\xFF')
end

return _M
