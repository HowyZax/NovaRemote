require 'settings'
local serialize = require 'serialize'
local server = require 'network.server'

local sequence = 0
function server:sendSetting(setting, index)
    if index == nil and isinstance(setting.storage, Storage) then
        if setting.matrix then
            for i = 1,setting.matrix[1] do
                self:sendSetting(setting, i)
            end
        elseif setting.index then
            for i = 1,SettingFind(setting.index).range[2] do
                self:sendSetting(setting, i)
            end
        end
    else
        self:broadcast( serialize.pack{ seq = sequence, name = setting.name, index = index, value = setting:value(index), text = setting:text(index) } )
        sequence = sequence + 1
    end
end

function server:sendAll()
    for _,setting in ipairs(SettingsList()) do
        self:sendSetting(setting)
    end
end
-------------------------------------------------------------------
local meterlevel = 0
function server:sendMeters()
    local delta = 20 - (math.random(41) - 1)
    meterlevel = meterlevel + delta;
    if meterlevel < 0 then meterlevel = 0
    elseif meterlevel > 255 then meterlevel = 255 end
    self:broadcast( serialize.pack{ meter=meterlevel} )
end

-------------------------------------------------------------------
function server:onSettingChanged(setting, index)
    -- Note: server always broadcasts changes in-case there are multiple clients
--  if self.noSend then print('client:onSettingChanged - Skip sending ' .. setting.name); return; end
    self:sendSetting(setting, index)
end
-------------------------------------------------------------------
function server:onReceive(s)
    print('server:onReceive ', s)
    if s:sub(1,1) == '{' then
        local status, r = serialize.unpack(s)
        if not status then
            print(r)
        elseif type(r) == 'table' then
            local function setValue(r)
                SettingFind(r.name):setValue(r.value, r.index)
            end
--          setValue(r)
            --self.noSend = true
            local status, msg = pcall(setValue, r)
            if not status then
                print('Failed to set value: ' .. msg)
            end
            --self.noSend = false
        else
            print('rejected input (expecting table, received ' .. type(r) .. ')')
        end
    elseif s then
        print('>>', s)
        if s == 'sendAll' then
            self:sendAll()
        end
    end
end

-------------------------------------------------------------------
--server:broadcast('Hello')
SettingsGlobalNotify(server.onSettingChanged, server)

