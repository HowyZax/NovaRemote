---------------------------------------------------------------------
-- MenuItem class
--
-- Represents a single entry in a menu of values

MenuItem = class()

-- Constructor
function MenuItem:__init(items)
    -- Make sure that items is a table
    if type(items) == 'string' then items = { items } end
    assert(type(items) == 'table')
    -- Basic menu item attributes
    self.text = items[1]        -- Text for this menu item
    self.value = items.value    -- Value associated with this menu item (optional)
    self.color = items.color    -- Color coding for this item
    assert(self.text, 'Missing text for MenuItem')
end

---------------------------------------------------------------------
-- Menu class
--
-- Represents a list of MenuItems for an editable menu setting

Menu = class()
menus = {}                   -- Array of defined menus
function Menu:__init(items)
    self.items = {}
    local nvalues = 0
    for i,v in ipairs(items or {}) do
        local item = MenuItem(v)
        table.insert(self.items, item)
        assert(item.text)
        if item.value then nvalues = nvalues + 1 end
    end
    assert(nvalues == 0 or nvalues == #self.items, 'Inconsistent use of value field in menu items')
    --assert(#self.items > 0, 'Empty Menu')
    self.hasValues = (nvalues > 0)
    table.insert(menus, self)    -- add to master list of defined menus
end

-- Fallback method to find index of menu item with value closest to
-- the specifified value. This can happen if there is an incorrect
-- value, or a value that is no longer supported, or if the value is
-- a floating point number with rounding differences
function Menu:indexForValueClosest(v)
    local mindiff, idx = math.maxinteger, 1
    for i,item in ipairs(self.items) do
        local diff = math.abs(item.value - v)
        if diff < mindiff then
            mindiff, idx = diff, i
        end
    end
    return idx
end

-- Note: indeces are 1-based; values are typically 0-based
function Menu:indexForValue(v)
    v = v or self.items[1].value or 0
    if self.hasValues then
        -- Menu items store the corresponding values
        local idx = 1
        for i,item in ipairs(self.items) do
            assert(item.value, 'Missing value in menu item') -- Should not happen
            if item.value == v then
                return i
            end
        end
        return self:indexForValueClosest(v)
    else
        assert(type(v) == 'number')
        if v <= 0 then return 1
        elseif v >= #self.items then return #self.items
        else return v + 1 end
    end
end

function Menu:indexForText(s)
    for i,item in ipairs(self.items) do
        if item.text == s then
            return i
        end
    end
    return 1
end

function Menu:valueForIndex(idx)
    assert(idx > 0 and idx <= #self.items)
    if self.hasValues then
        return self.items[idx].value
    else
        return idx - 1
    end
end

function Menu:valueForText(s)
    local idx = self:indexForText(s)
    return self:valueForIndex(idx)
end

function Menu:textForValue(v)
    return self.items[self:indexForValue(v)].text
end

function Menu:incValue(v, step)
    local idx = self:indexForValue(v)
    idx = idx + (step or 1)
    if step == nil then -- allow wrap (HphMix relies on this!)
        if idx <= 0 then idx = #self.items
        elseif idx > #self.items then idx = 1
        end
    else    -- don't allow wrap if step provided as probably using inc/dec button
        if idx <= 0 then idx = 1
        elseif idx > #self.items then idx = #self.items
        end
    end
--  idx = (idx + 1 <= #self.items) and idx + 1 or 1
    return self:valueForIndex(idx)
end

function Menu:textList()
    local t = { }
    for _,item in ipairs(self.items) do
        table.insert(t, item.text)
    end
    return t
end

