local message = require 'message'
local WirelessRx = require 'rx12.view'
local parser = require 'rx12.parser'

local page = WirelessRx {
    name = 'RX12',
    title = 'RX12',
}

message.subscribe('rx12.packet', function(msg, cmd, data, packet)
        local t = parser.parse(cmd, data, packet)
        if t then
            page:updateContent(t)
        end
    end)

-- vim: foldmethod=marker
