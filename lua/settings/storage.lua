require 'class'

---------------------------------------------------------------------
-- General purpose storage class
--
-- Can store a single value or an array of arbitrary dimensions determined by
-- the size of the index field used to access the storage
-- Index can be formed from numbers, string keys or a combination
-- For example index = { x, y } for a 2D array
--             index = { x, y, z } for 3D array etc.
-- Can also be used as 1D array using index = { x } or index = x
-- For simple single-value store, use index=nil or empty
Storage = class('Storage')
function Storage:__init(items)
    self.data = items
end

function Storage:locate(index, create)
    assert(index ~= nil)
    --if index == nil then return self.data, index end
    if self.data == nil then self.data = { } end
    if type(index) ~= 'table' then return self.data, index end
    assert(#index > 0)
    local tbl = self.data
    for i = 1,#index-1 do
        local x = index[i]
        if tbl[x] == nil then
            if create then tbl[x] = { }
            else return end
        end
        tbl = tbl[x]
    end
    return tbl, index[#index]
end

function Storage:setValue(v, index)

    if index == nil then
        assert(self.data == nil or type(self.data) == type(self.data), "Unexpected attempt to change type of storage value from " .. type(self.data) .. " to " .. type(v))
        self.data = v
        return
    end

    local tbl, i = self:locate(index, true)
    if i == nil then
        assert(tbl == nil or type(tbl) == type(v), "Unexpected attempt to change type of storage value from " .. type(tbl) .. " to " .. type(v))
        tbl = v
    else
        assert(v == nil or tbl[i] == nil or type(tbl[i]) == type(v), "Unexpected attempt to change type of storage value from " .. type(tbl[i]) .. " to " .. type(v))
        tbl[i] = v
    end
end

function Storage:value(index)
    if index == nil then return self.data end
    local tbl, i = self:locate(index)
    return tbl and (i == nil and tbl or tbl[i])
end

function Storage:clear(index)
    if index == nil then self.data = nil; return; end
    local tbl, i = self:locate(index)
    if tbl then tbl[i] = nil end
end

function Storage:count(index)
    if index == nil then
        if self.data then return #self.data end
    else
        local tbl, i = self:locate(index)
        if tbl then return #tbl end
    end
    return 0
end


