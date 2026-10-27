local _M = { }
local parse = require 'serial.parse'

-- Note: src was used as MSbyte of 16-bit length in earlier GUI/Nomad software but this
-- is incompatible with other products (eg. RX12). Now it is the responsibility of Nomad
-- to ask for data in a manner that does not generate large packets. This particulary affects
-- the reading of large arrays. Future software might need to use a different MAGIC number to
-- allow packets with larger payload.

local HEADER_SIZE = 6   -- AA sum len src dst cmd
local MAGIC = 0xAA

_M.command = {
    --cInPeaks  = 0x10,
    cInPeaksMix8 = 0x10, -- 16+1 meters for meter bridge (same order as fader assing); last meter is for Input Configure
    cBusPeaks = 0x11,
    cOutPeaks = 0x12,
    cTimecode = 0x16,   -- generator timecode & transport status
    cInPeaksPC = 0x18,
    cInPeaksMix8_COMP = 0x19,
    cBusPeaks_COMP    = 0x1A,
    cOutPeaks_COMP    = 0x1B,
    cInPeaksPC_COMP   = 0x1C,   -- meters
    cTimecodeReader = 0x1D, -- reader timecode

    cVersion  = 0x20,

    cMsgReturnStr   = 0x26, -- Not used by GUI
    cMsgReturnVal   = 0x27, -- Not used by GUI
    cErrString      = 0x28, -- Not used by GUI
    cMsgHandlerCall = 0x29, -- Not used by GUI

    cReserved1 = 0x2A,
    cReserved2 = 0x2B,
    cReserved3 = 0x2C,
    cReserved4 = 0x2D,
    cReserved5 = 0x2E,
    cReserved6 = 0x2F,

    cGotoPage = 0x52,   -- Goto page
    cPopup    = 0x53,   -- Popup message box
    cRequest  = 0x54,   -- Request setting(s)
    cSetting  = 0x55,   -- Change setting
    cDebug    = 0x56,   -- Show debug message on console/log file
    cEvent    = 0x57,   -- Notify event from GUI (e.g. button press)
    cCommand  = 0x58,   -- Adhoc command to GUI (e.g. start MP3 recording)

    -- For RX12
    cQUICKLEVELS  =  0,
    cERRVCO       =  1,
    cSPECTRUM     =  2,
    cSUBCH        =  3, -- sub channel block for RXA and RXB
    cCHANNEL      =  4, --  cmd code to change channel ---
    cIDCODE       =  5, --  cmd code to  change idcode
    cLNA          =  6, --  cmd code to  change LNA  off/on
    cTONE         =  7, --  cmd code to  change Tone off/on
    cSINGLEMODE   =  8, --  cmd code to  change SingleMode off/on
    cAESMODE      =  9,
    cQRXPROGRAM   = 10, -- triggers QRXprogram mode and provides status to RX12
    cSCAN_MODE    = 11,
    cSCAN_RESULTS = 12,
    cOUTPUTMODE   = 13,
    cSOFTVER      = 14,
    cSayWhat15    = 15,

    cQRXTX         = 0x50, -- 80 embeddded QOPT command sent via QOPT MCASP packet channel (from RS485 port)
    cENCAPSULATE   = 0x51, -- encapsulated command in the old sony format
    cCHANNEL_SHORT = 0x52, -- for IFB200 efficiency channel16,unitcode8

}

local function hex(b)
    local r = ''
    local s = ''
    for c in b:gmatch('.') do
        s = s .. string.format('0x%02X ', string.byte(c))
        if #s >= (16*5) then r = r .. s .. '\n'; s = '' end
    end
    r = r .. s
    return r
end

function _M.pack(cmd, data, src, dst)
    src = src or 0
    dst = dst or 0
    data = data or ''
    if #data > 0xFF then
        print( string.format('WARNING: cmd 0x%02X payload too large (%d bytes)',  cmd,  #data) )
        return ''
    end
    assert(#data < 0x7FFF)
    assert(#data <= 0xFF)
    assert(cmd < 0x7F)
    local sum = 0
    local h = string.pack('<I1BBB', #data, src, dst, cmd)
    for i = 1,#h do
        sum = sum + h:byte(i)
    end
    for i = 1,#data do
        sum = sum + data:byte(i)
    end
    local pkt = string.pack('<BB', MAGIC, sum & 0xFF) .. h .. data
    return pkt
end


--
-- Parse start of buffer for complete command packet.
--
-- Despatches complete commands by calling the supplied function 'fn'
--
-- Returns: (1) nil if buffer does not start with a valid command, or part of one
--              otherwise the number of of bytes to consume
--                    or, 0 if possible incomplete packet in buffer
--          (2) true if failed to process command
function _M.parser(buf, fn)
    --print('parse:',  #buf,  hex(buf))
    if #buf == 0 or buf:byte(1) ~= MAGIC then
        return
    end
    if #buf >= HEADER_SIZE then
        if #buf >= HEADER_SIZE + string.unpack('<I1', buf, 3) then
            local len = string.unpack('<I1', buf, 3)
            local src = buf:byte(4)
            local dst = buf:byte(5)
            local sum = 0
            for i = 3, HEADER_SIZE+len do
                sum = sum + buf:byte(i)
            end
            sum = sum & 0xFF
            --print(string.format('Checksum: 0x%02X', sum))
            if buf:byte(2) ~= sum then
                --print(string.format('Checksum BAD, cmd:%02X 0x%02X (0x%02X)', buf:byte(6), sum, buf:byte(2)))
                return
            end
            local packet = buf:sub(1, HEADER_SIZE+len)
            local cmd, data = packet:byte(HEADER_SIZE), packet:sub(HEADER_SIZE+1)
            --print(string.format('Checksum OK, cmd:%02X (%d bytes)', cmd, #data))
            local result,msg = pcall(fn, cmd, data, packet, src, dst)
            if not result then
                print('serial.protocol.parse failed:', msg)
            end
            return HEADER_SIZE+len, (not result)
        end
    end
    return 0
end


-- Parse incoming buffer
-- Modifies buf string containing incoming data stream
-- Note that Sony-style version requests can be appear on the same serial stream.
local parsers = { _M.parser }
function _M.parse(buf, fn)
    return parse.parse(buf, fn, parsers)
end

local commandNames = { }
for k,v in pairs(_M.command) do
    commandNames[v] = k
end
function _M.cmdname(x)
    return commandNames[x] or 'cUnknown'
end

return _M
