require 'settings'
local client = require 'client'
local serialize = require 'serialize'

function client:update()
    local s = self:mcreceive()
    if s and s:sub(1,1) == '{' then
        local status, r = serialize.unpack(s)
        if not status then
            print(r)
        elseif type(r) == 'table' then
            if r.meter then
                --widgets.setMeter(r.meter)
                MetersSetLevel(r.meter)
            else
                local function setValue(r)
                    SettingFind(r.name):setValue(r.value, r.index)
                end
    --          setValue(r)
                self.noSend = true
                local status, msg = pcall(setValue, r)
                if not status then
                    print('Failed to set value: ' .. msg)
                end
                self.noSend = false
            end
        else
            print('rejected input (expecting table, received ' .. type(r) .. ')')
        end
    elseif s then
        print('>>', s)
    end
end

function client:onSettingChanged(setting, index)
    if self.noSend then print('client:onSettingChanged - Skip sending ' .. setting.name); return; end
    local s
    if index == nil then
        s = serialize.pack{ name = setting.name, value = setting:value()}
    else
        s = serialize.pack{ name = setting.name, index = index, value = setting:value(index), text = setting:text(index), rawtext=setting:rawtext(index), graphic=setting:graphic() }
    end
    print('client sending:',  s)
    self:send(s)
end

--SettingsGlobalNotify(client.onSettingChanged, client)

