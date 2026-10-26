print 'serial.slave'
local config = require 'config'
local message = require 'message'
local serial = require 'serial'
local serialprotocol = require 'serial.protocol'
local command = serialprotocol.command

require 'settings'
local protocol = require 'settings.protocol'

local port
local buf = ''      -- incoming buffer
local noSendSetting = false
local METER_RANGE = 40.         -- Range (dB) of on-screen meters

local timer = ElapsedTimer()

--{{{ Sending/despatching commands for active port
--local print = dprint or print
local function sendPacket(pkt)
    port:write(pkt)
    message.send('serial.transmit', pkt)
end

local function send(cmd, data)
    local pkt = serialprotocol.pack(cmd, data)
    sendPacket(pkt)
end

local function sendSetting(setting, index)
    local data = protocol.pack(setting, index)
    if data then send(command.cSetting, data) end
end

local function sendEvent(which)
    send(command.cEvent, which)
end

local counter = 0
local prevStatus = -1

local meterCmd = {  -- table mapping command to meter category string
    [ command.cInPeaksPC        ] = 'meters.in',
    [ command.cBusPeaks         ] = 'meters.bus',
    [ command.cOutPeaks         ] = 'meters.out',
    [ command.cInPeaksMix8      ] = 'meters.bridge',
    [ command.cInPeaksPC_COMP   ] = 'meters.in.compressor',
    [ command.cBusPeaks_COMP    ] = 'meters.bus.compressor',
    [ command.cOutPeaks_COMP    ] = 'meters.out.compressor',
    [ command.cInPeaksMix8_COMP ] = 'meters.bridge.compressor',
                                 -- 'meters.input'  ... single meter for InputConfigure generated from cinPeaksMix8
                                 -- 'meters.input.compressor'  ...   "
}

local forwardCmd = {   -- table mapping command to message-type string. Packets are broadcast to any module that is interested
    [ command.cQUICKLEVELS ]    = 'rx12.packet',
    [ command.cSUBCH]           = 'rx12.packet',
--    [ command.cQRXPROGRAM ]     = 'rx12.packet',
--    [ command.cVERSION ]        = 'rx12.packet',
}

local function sendVersion()
    local s --  = 'PCD32'
    local suffx  = 'D32'
    local tabletchar = ' '
    if config.nomad   then suffx      = 'N'    end -- 'PCNT'    or  'PCNO'     old nomad code - we will never make this version again since nomad is so old
    if config.nova    then suffx      = 'nova' end  -- 'PCnovaT' or 'PCnovaO'   note both .nomad and .nova will be true here - make sur enomad does not try to connect to this version
    if config.tablet  then tabletchar = 'T'    end  -- 'PCNT'
    if config.surface then tabletchar = 'O'    end  -- 'PCNO'
  --  end
    s = 'PC' .. suffx .. tabletchar .. '-' .. config.version .. '#' .. protocol.version
    print('sendVersion:', s)
    send(command.cVersion, s)
end

local function despatch(cmd, data, packet)
--  print (string.format('%5d: CMD 0x%02X', counter, cmd), #data, 'bytes')
    if meterCmd[cmd] ~= nil then
        local tbl = { }
        for i = 1,#data do
            local ch = data:byte(i)
            if ch < 255 then ch = ch + 1 end  -- workaround for off-by-1dB at -20dB
            local val = (ch - (255-METER_RANGE))/METER_RANGE   -- 40dB range; full scale = 255; 1dB per step
            if val < 0 then val = 0. end
            tbl[i] = val
        end
        message.send(meterCmd[cmd], tbl)
        if #tbl == 17 then
            if      cmd == command.cInPeaksMix8 then message.send('meters.input',  { tbl[17] } )
            elseif  cmd == command.cInPeaksMix8_COMP then message.send('meters.input.compressor',  { tbl[17] } )
            end
        end
    elseif cmd == command.cTimecode then
        if #data == 11 then
            local status = string.byte(data, 11)
            if status ~= prevStatus then
                prevStatus = status

                local s
                if      status == 3 then s = 'record'
                elseif  status == 2 then s = 'play'
                else                     s = 'stop'
                end
                print ('transport.status',  s)
                message.send('transport.status', s)
            end
        end
        message.send('timecode.raw.generator', data)
    elseif cmd == command.cTimecodeReader then
        message.send('timecode.raw.reader', data)
    elseif cmd == command.cRequest then
        local ok,setting,idx = pcall( function() return protocol.unpackRequest(data) end )
        print('cRequest',  ok, setting, idx)
        if ok then
            if setting then
                sendSetting(setting, idx)
            elseif #data == 0 then
                for _,setting in ipairs(SettingsList()) do
                    sendSetting(setting)
                end
            end
        else
            print(ok,  setting)
        end
    elseif cmd == command.cSetting then
        noSendSetting = true
        pcall( function() protocol.unpack(data) end )
        noSendSetting = false
    elseif cmd == command.cGotoPage then
        print('cGotoPage', data)
        if Page.Exists(data) then
            Page.Enter(data)
        end
    elseif cmd == command.cPopup then
        local success, errormsg = pcall(function()
            local timeout, s = string.unpack('<I2z', data)
            message.send('popup.msg', {timeout=timeout, s })
        end)
        if not success then print('cPopup:', errormsg) end
    elseif cmd == command.cVersion then
        if #data == 0 then
            sendVersion()
         end
    elseif cmd == command.cDebug then
        local success, errormsg = pcall(function()
            local s = string.unpack('<z', data)
            print(s)
        end)
        if not success then print('cDebug:', errormsg) end
    elseif cmd == command.cCommand then
        local success, errormsg = pcall(function()
            local s = string.unpack('<z', data)
            print('cmd.' .. s)
            message.send('cmd.' .. s)
        end)
        if not success then print('cCmd:', errormsg) end
    elseif forwardCmd[cmd] then    
        message.send(forwardCmd[cmd], cmd, data, packet)
    end
    counter = counter + 1
end
--}}}
--{{{ Common code
local function onReadyRead(self)
    local rx = self:read()
    local ok, errors
    message.send('serial.receive', rx)
    --print('serial.receive',  #rx)
    buf, ok, errors = serialprotocol.parse( buf .. rx, despatch )
    if ok then
        self.timeout = self.defaultTimeout or 3
        if not self.online then
            self.online = true
            sendVersion()
            message.send('serial.online', true)
        end
    end
    if errors > 0 then
        message.send('serial.receive.errors', errors)
    end
end

--}}}
--{{{ serialport
local serialport = serial.create()

serialport.timeout = 0
serialport.online = false
serialport.onReadyRead = onReadyRead

serialport.onError = function(self)
--  print 'serialport.onError'
    self.timeout = 1
end

serialport.tryOpen = function(self, which)
    print('Open COM port', which)
    local failed = not self:open( which )
    self.timeout = 1
end
serialport.disconnect = function(self)
    self:close()
end
--}}}
--{{{ tcpport
local core = require 'qt.core'
local TcpSocket = require 'serial.tcp'
local tcpport = TcpSocket.create()

tcpport.onReadyRead = onReadyRead
tcpport:subscribe('_zaxhub._tcp')

local othertcp = { }

--local rx12port = TcpSocket.create()
--rx12port.onReadyRead = onReadyRead
--rx12port:subscribe('_zaxhub-rx12._tcp')
--table.insert(othertcp, rx12port)

--}}}

  
--{{{ main
local scan = 0
function tryOpen()
    if port and port.online then
        port.online = false
        message.send('serial.online', false)
    end
    if port then
        port:disconnect()
    end
    local which = SettingFind('serial.port'):text()
    if which:lower() == 'auto' then
        local ports = serial.ports()
        table.insert(ports, 'TCP')
        if #ports > 0 then
            scan = scan < #ports and scan + 1 or 1
            which = ports[scan]
--          print('Scan COM port', which)
        end
    end
    if which == 'TCP' then
        tcpport:tryOpen()
        port = tcpport
    else
        serialport:tryOpen(which)
        port = serialport
    end
end

if _STANDALONE then
    serialport:open( 'COM4' )
    port = serialport
else
    tryOpen()

    SettingNotify('serial.port', tryOpen)

    message.subscribe( 'timer.tick', function()
        local elapsed = timer:restart()
        if port.timeout >= 0 then
            port.timeout = port.timeout - elapsed
        else
            print('port comms timeout', port.__classname)
            tryOpen()
        end

        -- Service other TCP ports to Hub (e.g. RX12)
        for _,p in ipairs(othertcp) do
            if p.timeout >= 0 then
                p.timeout = p.timeout - elapsed
            else
                print('port comms timeout', p.__classname)
                p:tryOpen()
            end
        end

    end )

end

message.subscribe( { 'setting.changed', noMsg=true }, function(setting, index)
    if noSendSetting then
        print('serial.slave: Block sending of setting', setting.name)
        return
    end
    print('serial.slave: Send setting',  setting.name)
    sendSetting(setting, index)

end )

message.subscribe( { 'action', noMsg=true }, function(which)
    print('action: ',  which)
    sendEvent(which)
end )

message.subscribe( 'rx12.send', function(_, pkt)
    sendPacket(pkt)
end )

message.subscribe( 'timer.second', function()
    if counter > 0 then
        message.send('serial.receive.packets', counter) 
        counter = 0
    end
end )

--}}}
