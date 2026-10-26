local id = require 'settings.id'
local pretty = require 'pl.pretty'
local tablex = require 'pl.tablex'

pretty.dump(id)
for i,v in ipairs(id) do id[v] = i end

local function arrayToString(tbl)
    return type(tbl) == 'table' and pretty.write(tbl, '') or ''
end

local function indices(arr, init)
    init = init or { }
    local function it(a, idx)
        --print ('it', #a, a, #idx, idx, '['..table.concat(idx, ':') .. ']')
        assert(#a == #idx)
        for i = #a,#init+1,-1 do
            idx[i] = idx[i] + 1
            if idx[i] > a[i] then idx[i] = 1
            else return idx end
        end
    end
    local function fail(a, idx) return nil end
    local function single(a, idx)
        return idx == nil and a or nil
    end
    if not arr then return fail end

    --assert(#arr > 0, 'Unexpected empty table')
    if #init > #arr then return fail end

    local idx = { }
    for i,v in pairs(init or { }) do
        if v <= 0 or v > arr[i] then return fail end
        table.insert(idx, v)
    end
    if #init == #arr then return single, idx, nil end
    while #idx < #arr do table.insert(idx, 1) end
    idx[#idx] = idx[#idx]-1
    --print('idx', pretty.write(idx, ''), #arr, #init)
    return it, arr, idx
end

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

local function packValue(value, nbytes)
    if type(value) == 'number' then
        value = value // 1 -- TODO: remove
        local fmt = 'i' .. tostring(nbytes or 1)
        --print('pack', fmt, value)
        return string.pack(fmt, value)
    elseif type(value) == 'string' then
        assert(nbytes == 0, 'Packing ' .. value)
--      return string.pack('s1', value)
        return string.pack('z', value)
    elseif type(value) == 'table' then
        local b = ''
        for k,v in pairs(value) do
            b = b .. string.pack('z', tostring(k)) .. string.pack('z', tostring(v))
        end
        return b .. string.pack('x')
    else
        --print (type(value))
        error('Attempting to pack', tostring(value))
        return string.pack('z', type(value))
    end
end

local function unpackTable(buf, pos)
    local t = { }
    while true do
        local k, v
        k, pos = string.unpack('z', buf, pos)
        if #k == 0 then break end
        v, pos = string.unpack('z', buf, pos)
        t[k] = v
    end
    return t, pos
end

local function packArray(a, valoffset, nosize)
    local b = ''
    valoffset = valoffset or 0
    a = a or { }
    if not nosize then
        b = b .. packValue(#a, 1)
    end
    for _,v in ipairs(a) do
        b = b .. packValue(v+valoffset)
    end
    return b
end

local function packIndex(idx)
    return packArray(idx, -1)
end

local function getPackBytes(setting)
    if setting.range then
        local min,max,step=setting.range[1], setting.range[2], setting.range.step
        if step ~= nil and step < 1 then
            min, max = min/step, max/step
        end
        if min < -0x8000 or max > 0x7FFF then return 4
        elseif min < -0x80 or max > 0x7F then return 2
        else return 1 end
    elseif isinstance(setting, SettingMenu) then
        return 1
    elseif isinstance(setting, SettingText) then
        return 0
    elseif isinstance(setting, SettingTable) then
        return 5
    elseif type(setting.default) == 'string' or setting.maxchars then
        return 0
    end
    return 4
end

local function getValue(setting, idx)
    local value = setting:value(idx)
    if setting.range and setting.range.step and setting.range.step < 1 then
        value = value/setting.range.step
    end
    value = value or setting:defaultValue(idx) -- or 0
    if setting.protocol and setting.protocol.offset then
        value = value + setting.protocol.offset
    end
    return value
end

local function packSetting(setting, idx)
    local setting = SettingFind(setting)
    if not id[setting.name] then return end
    print (string.rep('-', 64))
    --print(setting.name, setting.storage, arrayToString(setting.array), setting:defaultValue(idx))
    local n = getPackBytes(setting)
    local b = packValue(id[setting.name], 2)
    idx = type(idx) == 'number' and { idx } or idx
    print('name:', setting.name)
    print('id:', hex( packValue(id[setting.name], 2) ) )
    print('type:', hex( packValue(n, 1) ) )
    print('range:', arrayToString(setting.range) )

    if not id[setting.name] then
        print ('*** setting id not assigned ***')
    else
        assert(id[setting.name] < 0x7FFF)
    end

    if setting.array then
        local array = setting.array
        if isinstance(setting, SettingTable) then
            assert(#array == 1)
            if setting:count() > array[1] then
                array = tablex.copy(array)
                array[1] = setting:count()
            end
        end

        print( 'array:', arrayToString(array), setting:count())
        print( 'index:', arrayToString(idx) )
        b = b .. packValue(n, 1) .. packArray(array) .. packIndex(idx)
        local bidx = idx and tablex.copy(idx) or {} -- base index for part of array being sent
        local widx = ((idx == nil) or (#idx < #array)) and {} or nil -- empty table if need to write indices
        --print('WIDX', widx)
        for idx in indices(array, idx) do
            --print(pretty.write(idx,''), setting:value(idx) or nil)
            local value = getValue(setting, idx)
            if type(value) == 'number' and value == 0 then
                -- pass
            elseif type(value) == 'string' and #value == 0 then
                -- pass
            else
                if widx then    -- sending indices?
                    local x = tablex.sub(idx, 1, -2)    -- new long index required? ignores last field in index (aka column)
                    if not tablex.compare(x, widx, '==') then   -- yes!
                        if #widx > 0 then b = b .. string.pack('B', 0xFF) end -- 0xFF indicates new long index - not required for first long index
                        widx = x    -- update active long index
                        assert(#widx == #idx-1)
                        local pidx = tablex.sub(widx, #bidx+1)  -- only pack parts of index not in the base index
                        print('bidx:', arrayToString(bidx), 'widx:', arrayToString(widx), 'pidx:', arrayToString(pidx))
                        b = b .. packArray(pidx, -1, true)
                    end
                    b = b .. packValue(idx[#idx]-1, 1) -- write column part of index
                end
                print('packValue', arrayToString(idx), arrayToString(widx), idx[#idx], value)
                --b = b .. packValue(0x55, 1)
                b = b .. packValue(value, n)
            end
        end
    else
        b = b .. packValue(n, 1) .. packValue(0, 1) .. packValue(getValue(setting, idx), n)
    end
    print( 'bytes:', #b)
    print( hex(b) )
    return b
end

local function unpackArrayBounds(buf, pos, valoffset, n)
    local arr = { }
    pos = pos or 1
    valoffset = valoffset or 0
    local arraysize = n
    if not arraysize then
        arraysize,pos = string.unpack('B', buf, pos)
    end
    --print('arraysize', arraysize)
    for i = 1,arraysize do
        local v = string.unpack('B', buf, pos+i-1)
        v = v + valoffset
        table.insert(arr, v)
    end
    return #arr > 0 and arr or nil
end


local function unpackSetting(buf)
    print('unpackSetting',  #buf,  hex(buf))
    local which,kind = string.unpack('<I2b', buf)
    print(which, kind, id[which])
    local arr = unpackArrayBounds(buf, 4)
    local idx = arr and unpackArrayBounds(buf, 4+1+#arr, 1) or nil
    print('kind:', kind, kind==0 and 'string' or '')
    print('array:', arrayToString(arr))
    print('idx:', arrayToString(idx))
    local n = kind
    if n >= 0 and n <= 5 then
        local fmt = (n == 0 and 'z') or (n == 5 and 'kv') or 'i'..n
        local i = 3 + 1 + 1
        if arr then
            i = i + #arr + 1
            if idx then i = i + #idx end
        end
        local val
        local scale
        local setting = SettingFind(id[which])
        print('format:', fmt)
        if setting.range and setting.range.step and setting.range.step < 1 and n >= 1 and n <= 4 then
            scale = setting.range.step
        end
        if arr then
            local ok,msg = pcall(function()
                print('clr:', arrayToString(idx))
                setting.storage:clear(idx)
                local bidx = idx and tablex.copy(idx) or { }
                local index = nil
                while i <= #buf do
                    --print ('??', i, #buf)
                    local j
                    local i1 = i
                    if index == nil then     -- read long index
                        --print('len:', #arr - #bidx, #arr, #bidx)
                        index = tablex.copy(bidx)   -- copy base index
                        local pidx = unpackArrayBounds(buf, i, 1, #arr - #bidx) -- load rest of index from buffer
                        if pidx then    -- Append part index to base index
                             tablex.icopy(index, pidx, #bidx+1)
                             i = i + #pidx
                        end
                        print('pidx:', arrayToString(pidx), 'index:', arrayToString(index))
                        assert(#index == #arr) -- should have a full index now
                    else    -- read short index
                        j, i = string.unpack('B', buf, i)
                        if j == 0xFF then index = nil else index[#index] = j + 1 end
                    end
                    if index ~= nil then
                        if fmt == 'kv' then
                            val, i = unpackTable(buf, i)
                            print('unpack kv:', arrayToString(index), i1, i, pretty.write(val, ''))
                        else
                            val, i = string.unpack(fmt, buf, i)
                            print('unpack:', arrayToString(index), i1, i, val)
                        end
                        if scale then val = val * scale end
                        setting.storage:setValue(val, index)
                    end
                end
            end )
            if not ok then print(ok, msg) end
            setting:sendChangeNotification(idx) -- send one notification on completion
        else
            if fmt == 'kv' then
                val, i = unpackTable(buf, i)
                print('unpack kv:', i, pretty.write(val, ''))
            else
                val, i = string.unpack(fmt, buf, i)
            end
            if scale then val = val * scale end

            if setting.protocol and setting.protocol.offset then
                print('Apply offset', setting.protocol.offset)
                val = val - setting.protocol.offset
            end

            print('value:',  val)
            setting:setValue(val)
        end
        print('i:',  i, #buf)
        assert(i == #buf+1)
    else
        error 'Unknown data kind'
    end
end

local function unpackRequest(buf)
    print('unpackRequest',  #buf,  hex(buf))
    local which
    local idx
    if #buf == 0 then return end
    which = string.unpack('<I2', buf)
    if #buf > 2 then
        idx = unpackArrayBounds(buf, 3, 1)
    end
    print('request:', which, id[which], arrayToString(idx))
    return SettingFind(id[which]), idx

end

local function packRequest(setting, idx)
    local b = ''
    if setting then
        local name = type(setting) == 'string' and setting or setting.name
        b = packValue(id[name], 2)
        idx = type(idx) == 'number' and { idx } or idx
        if idx then
            b = b .. packIndex(idx)
        end
    end
    return b
end

return {
    pack = packSetting,
    unpack = unpackSetting,
    packRequest = packRequest,
    unpackRequest = unpackRequest,
    version = assert(id.version),
}

