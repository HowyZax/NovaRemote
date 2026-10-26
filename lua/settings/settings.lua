local serialize = require 'serialize'
local message = require 'message'
require 'pl'
local pretty = require 'pl.pretty'
local file = require 'pl.file'
local path = require 'pl.path'
local config = require 'config'
--local core = require 'qt.core'                -- For preferences
--local Preferences = core.Preferences
local settingsByName = {}           -- Array of defined settings, keyed by name
local settingsList = { }            -- List of settings, in order registered
local autosettingCount = 0          -- Counter for auto-naming
local locked = false
local loadedValues = { }            -- Recent loaded values, for SettingRegister()

_SETTINGS_CFG = _SETTINGS_CFG or config.utils.filename('settings')

local function onMovieSlateSettingChanged(msg, settingname, value)
    print ('onMovieSlateSettingChanged', msg, settingname, value)
    local function setValue()
        SettingFind(settingname):setValue(value)
    end
    local status, err = pcall(setValue)
    if not status then
        print('Failed to set value: ' .. err)
    end
end
message.subscribe('movieslate.setting.changed', onMovieSlateSettingChanged)

local function container()
    local tbl = { }
    local mt = {
        __index = function(t, k)
            local v = tbl[k]
            if isinstance(v, SettingBase) then
                return v:value()
            end
            if v == nil then
                assert(not locked,  "Attempt to access missing setting field: " .. k)
            end
            return v
        end,
        __newindex = function(t,k,v)
            local s = tbl[k]
            if s == nil then
                assert(type(v) == 'table')
                assert(not locked,  "Settings are locked! Cannot add field: " .. k)
                tbl[k] = v
            elseif isinstance(s, SettingBase) then
                s:setValue(v)
            else
                error('Settings are locked! Illegal attempt to write setting field:' .. k, 2)
            end
        end
    }
    return setmetatable({ }, mt)
end

settings = container()     -- Nested tree of settings, for table-like read/write of values

function SettingRegister(setting)
    --local prevLocked = locked
    locked = false
    -- Register the setting by name, in settingsByName table

    assert(setting.name, 'Cannot register unnamed setting')
    assert(not getmetatable(settingsByName), 'Settings are locked! Cannot register setting: ' .. setting.name)
    assert(not settingsByName[setting.name],  'Duplicate setting ' .. setting.name)
    settingsByName[setting.name] = setting
    table.insert(settingsList, setting)

    -- Install the setting in the nested hierarchy of settings
    --
    -- e.g diskmix.phase is installed in
    -- settings = {
    --   diskmix = {
    --     phase = Setting ....
    --     ...
    --   }
    -- }
    local fields = {}
    for s in setting.name:gmatch('%w+') do
        fields[#fields+1] = s
    end
    local t = settings
    for i = 1,#fields do
        local k = fields[i]
        --print(i, k)
        if t[k] == nil then
            t[k] = (i == #fields) and setting or container()
        elseif isinstance(t[k], SettingBase) then
            assert(false, 'Setting name class' .. setting.name .. ' ' .. t[k].name)
        end
        if i < #fields then
            assert(type(t[k]) == 'table', setting.name)
            t = t[k]
        end
    end

    -- Check for initial value, particulary if setting is registered after initial values have been loaded
    if loadedValues[setting.name] then
        setting:setValue( loadedValues[setting.name] )
    end
    --[[
    for _,v in ipairs(loadedValues) do
        if v.name and v.name == setting.name then
            setting:setStorage(v.value)
        end
    end
    --]]
    locked = true
end

function SettingsShow()
    --[[
    print('--- Settings by name ---')
    tprint(settingsByName)
    print('--- Settings hierarchy ---', getmetatable(settings) and 'SETTINGS LOCKED' or '')
    tprint(settings)
    print('--- Settings --- ')
    print('name', 'value', 'rawtext', 'text')
    for name,setting in pairs(settingsByName) do
        print(name, setting:value(), setting:rawtext(), setting:text())
    end
    print('--- End of Settings ---')
    --]]
end
--[[
function _SettingsRegister(t, prefix)
    prefix = prefix and (prefix..'.') or ''
    for k,v in pairs(t) do
        --print(k, prefix, v, isinstance(v, SettingBase))
        if isinstance(v, SettingBase) then
            print("Setting: " .. string.format('%-16s', v.__classname) .. prefix ..  k)
            v.name = prefix .. k
            SettingRegister(v)
        else
            SettingsRegister(v, prefix .. k)
        end
    end
end
function SettingsRegister(t, prefix)
    local prevlocked = locked
    locked = false
    _SettingsRegister(t, prefix)
    locked = prevlocked
end
--]]

function SettingsLocked()
    locked = true
end

-- Find an existing setting by name
function SettingFind(setting, strict)
    if isinstance(setting, SettingBase) then return setting end
    local t = settingsByName[setting]
    if strict == nil or strict == true then
        assert(not setting or t, 'Setting not defined: ' .. '[' .. tostring(setting) .. ']')
    end
    return t
end

function SettingNotify(setting, method, ...)
    SettingFind(setting):subscribe(method, ...)
end

function SettingNotifyEx(setting, method, ...)
    SettingNotify(setting, method, ...)
    -- Send an initial notification notice
    setting = SettingFind(setting)
    method(setting, ...)
end

function SettingsGlobalNotify(method, ...)
    message.subscribe( { 'setting.changed', noMsg=true }, method, ...)
end

function SettingsList()
    return settingsList
end

local function _SettingsFileName(which)
    if type(which) == 'string' then return which end
    local s = config.utils.filename(_SETTINGS_CFG or 'settings', which)
    print ('SettingsFileName', s)
    return s
end

function SettingsSave(which)
    local filename = _SettingsFileName(which)

    --[[
    -- Method 1: Nested table, similar to settings definitions
    local tbl = { }
    for i,setting in ipairs(SettingsList()) do
        print ('>', setting.name, setting.storage.data)
        local fields = { }
        for x in setting.name:gmatch('[^.]+') do
            table.insert(fields, x)
        end
        if setting.storage.data ~= nil then
            local t = tbl
            for i,v in ipairs(fields) do
                if i == #fields then
                    assert(not t[v])
                    t[v] = setting.storage.data
                else
                    t[v] = t[v] or { }
                    t = t[v]
                end
            end
        end
    end
    for i,setting in ipairs(SettingsList()) do
        table.insert(tbl, setting.name)
    end
    local ofile = io.open('settings1.cfg', 'wb')
    ofile:write( pretty.write(tbl) )
--  ofile:write( serialize.pack(tbl) )
    ofile:close()
    --]]

    --[[
    -- Method 2: table of key,value pairs where key is setting name
    -- Advantage: simplest implementation
    -- Disadvantage: unordered, so settings file cannot define the order
    -- in which settings are applied

    for i,setting in ipairs(SettingsList()) do
        print ('>', setting.name, setting.storage.data)
        tbl[setting.name] = setting.storage.data
    end

    --]]

    ---[[
    -- Method 3: array of entries { name=setting.name, value=setting.value }
    -- Advantage: consistent with information sent to clients (see client.lua)
    --            Order of settings is preserved
    -- Disadvantage: Slightly more complex than method 1; larger file
    local tbl = { }
    for i,setting in ipairs(SettingsList()) do
        if setting.save and setting.storage.data ~= nil then
            print ('>', setting.name, setting.storage.data)
            table.insert(tbl, { name = setting.name, value = setting.storage.data } )
        end
    end

    file.write(filename, serialize.packEx(tbl) )    -- ... better/logical ordering of table members
    --]]


    --[[
    local s = serialize.pack(tbl)
    print '**=============================**'
    print(s)
    print '**=============================**'
    Preferences.write('settings/' .. which, s)
    --]]
    --[[
    -- Method 4: save each setting as a separate item in a section of the ini file
    -- Advantage: Cleaner syntax in .ini file
    -- Disadvantage: Assumes back by .ini file or something similar
    tbl = { }
    for i,setting in ipairs(SettingsList()) do
        local data = setting.storage.data
        -- if data ~= nil and (type(data) ~= 'table' or next(data)) then
        -- end
        if type(data) == 'table' and not next(data) then data = nil end
        Preferences.write('settings.' .. which .. '/' .. setting.name, data and serialize.pack(data))
    end

    tbl = { }
    for i,setting in ipairs(SettingsList()) do
        if setting.storage.data ~= nil then
            print ('>', setting.name, setting.storage.data)
            table.insert(tbl, { name = setting.name, value = setting.storage.data } )
        end
    end
    --]]
    --[[
    local ofile = io.open('settings.cfg', 'wb')
    ofile:write( pretty.write(tbl) )
    ofile:write( serialize.pack(tbl) )
    ofile:close()
    --]]

end

function SettingsLoad(which)
    local filename = _SettingsFileName(which)
    print('---SettingsLoad ---', which, filename)

    --[[ Method 1
    local s = Preferences.read('settings/1')
    print(s)
    print(type(s))
    local status, tbl = serialize.unpack(s)
    if status and type(tbl) == 'table' then
        tprint(tbl)
        for i,setting in ipairs(SettingsList()) do
            print ('>', setting.name, setting.storage.data)
            local fields = { }
            local t = tbl
            for x in setting.name:gmatch('[^.]+') do
                print(setting.name, x)
                if t[x] ~= nil then
                    t = t[x]
                else
                    t = nil
                    break
                end
            end
            if t ~= nil then
                print ('Apply setting', setting.name, t)
                setting:setStorage(t)
            end

        end
    end
    --]]
    --[[ Method 2
    local s = Preferences.read('settings/2')
    print(s, type(s))
    local status, tbl = serialize.unpack(s)
    if status and type(tbl) == 'table' then
        for i,setting in ipairs(SettingsList()) do
            if tbl[setting.name] ~= nil then
                print ('Apply setting', setting.name, t)
                setting:setStorage( tbl[setting.name] )
            end
        end
    end
    --]]

    ---[[ Method 3
--  local s = Preferences.read('settings/' .. which)
    local s = file.read(filename)
    local status, tbl = serialize.unpack(s or '')
    if status and type(tbl) == 'table' then
        for _,v in ipairs(tbl) do
            local setting = SettingFind(v.name, false)
            if setting then
                print ('Apply setting', setting.name, v.value)
                setting:setStorage( v.value )
            end
            loadedValues[v.name] = v.value
        end

    end
    --]]

    --[[ Method 4
    for i,setting in ipairs(SettingsList()) do
        local s = Preferences.read('settings.' .. which .. '/' .. setting.name)
        local status, v = serialize.unpack(s)
        print('# Apply setting', setting.name, s, status, v)
        if status then
            setting:setStorage( v )
        end
    end
    --]]

end

function SettingReference(setting, index, cached)
    local value
    setting = SettingFind(setting)
    assert(isinstance(setting, SettingTable))

    print(setting.name, index)

    local function read_if(cond) if cond then value = setting:value(index) end end
    local function write_if(cond) if cond then setting:setValue(value, index) end end

    read_if(cached)

    return setmetatable({}, {
            __index     = function(_, k) read_if(not cached); return value[k] end,
            __newindex  = function(_, k, v) read_if(not cached); value[k] = v; write_if(not cached) end,
            load        = function() read_if(true) end,
            save        = function() write_if(true) end,
        })
end

function SettingReferenceCached(setting, index)
    return SettingReference(setting, index, true)
end

