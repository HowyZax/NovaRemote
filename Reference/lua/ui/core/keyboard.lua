require 'class'

-------------------------------------------------------------------
-- Character sets

local charset = {
    lower       = 'abcdefghijklmnopqrstuvwxyz',
    upper       = 'ABCDEFGHIJLKMNOPQRSTUVWXYZ',
    numeric     = '0123456789',
    punctuation = '!@#$%^&*()-[],.?=_{}<>\'+',
    whitespace  = ' ',
    hex         = '0123456789abcdefABCDEF'
}
charset.alpha  = charset.lower .. charset.upper

-------------------------------------------------------------------
local KeyboardBase = Control:subclass("KeyboardBase")

function KeyboardBase:__init(items)
    super(KeyboardBase).__init(self, items)
    self.id = self.id or "Keyboard"

    -- Build table of supported keys, which can include extra keys
    -- provided by external keyboard and panel
    local tbl = { }
    for c in self.supported.keys:gmatch('.') do
        tbl[c] = c
    end
    for _,c in ipairs(self.supported.control or { }) do
        tbl[c] = c
    end
    self.supported.all = tbl
end

function KeyboardBase:create(parent)
    --[[
    for i,v in ipairs(self.keys) do
        self[#self+1] = Button{ font=0.2, text = v, margin=1, delegate=self.delegate, id=self.id .. 'Button', tag=v, parent=self, canFocus = false }
    end
    --]]
    for i,v in ipairs(self.keys) do
        local margin = 0
        local fit = 3
        if v == 'Shift' or v == 'Caps' then
            margin = { 0.1, 0.04, 0.1, 0.04 }
            fit = 5
        elseif v == 'Space' then
            fit = 8
        end

        local graphic, background
        if      v == '<-'   then graphic = 'left'
        elseif  v == '->'   then graphic = 'right'
        elseif  v == 'Bsp'  then graphic = 'backspace'
        elseif  v == 'Ent'  then background = 'forestgreen'
        end

        if graphic then
            self[#self+1] = ButtonEx{ graphic=graphic, background=background, margin=margin, delegate=self.delegate, id=self.id .. 'Button', tag=v, parent=self, canFocus=false }
        else
            self[#self+1] = Button{ fit=fit, v,  background=background, margin=margin, delegate=self.delegate, id=self.id .. 'Button', tag=v, parent=self, canFocus=false }
        end
    end
    assert(#self == #self.keys)
    Control.createChildren(self, parent)
end

function KeyboardBase:layout(rect)
    local layout = GridLayout(rect) -- Create layout manager for control rect
    layout:layoutChildren(self)     -- Layout the children
end

function KeyboardBase:setDelegate(delegate)
    local olddelegate = self.delegate
    self:traverse( function(o)
        if isinstance(o, Control) then
            assert(o.delegate == olddelegate);
            o.delegate = delegate
        end
    end )
end
function KeyboardBase:setState()
    -- do nothing; placeholder
end
function KeyboardBase:supports(event)
    return self.supported.all[event] ~= nil
end
-------------------------------------------------------------------
Keyboard = KeyboardBase:subclass("Keyboard")

Keyboard.keymap = {
    -- Base keys definitions, from left-to-right & top-to-bottom
    -- Following is written in convenient short-hand that is expanded
    -- at startup to create table/array 'keys' with one key per entry
    '1234567890', 'Bsp',
    'qwertyuiop-',
    '[asdfghjkl', 'Ent',
    ']zxcvbnm,.',
    'Shift', 'Caps', 'Space', '<-', '->', '?', '=',
}

-- Key translations for "Caps" state
Keyboard.caps = {
    { charset.lower, charset.upper } ,
    { "-[],.?=", "_{}<>'+" },
    ["Bsp"]  = "Del",
--  ["Caps"]  = "[CAPS]",
}

-- Key translations for "Shift" state.
Keyboard.shift = {
    { charset.numeric, "!@#$%^&*()" },
    -- caps translations are also added at startup
--  ["Shift"]  = "[SHIFT]",
}
-- Layout hints
-- These properties are used to guide the button layout process
-- The table is indexed using the base key name (not shifted etc)
Keyboard.hints = {
    ["Shift"] = { w = 2 },
    ["Ent"]   = { h = 2 },
    ["Caps"]  = { w = 2 },
    ["Space"] = { w = 3 },
}
Keyboard.columns = 11
Keyboard.rows = 5

Keyboard.supported = {
    control = { 'Shift', 'Ent', 'Caps', 'Space', 'Bsp', 'Del', '<-', '->', 'Esc' },
    keys = charset.alpha .. charset.numeric .. charset.punctuation .. charset.whitespace
}

local function translate(k, map)
    return map and map[k] or k
end

local function expand(map)
    for _,v in ipairs(map) do
        assert(#v[1] == #v[2])
        for i = 1,#v[1] do
            local from, to = v[1]:sub(i,i), v[2]:sub(i,i)
            assert(not map[from])
            map[from] = to
        end
    end
end

local function append(destmap, srcmap)
    for k,v in pairs(srcmap) do
        if type(k) == 'number' then table.insert(destmap, v)
        else destmap[k] = v end
    end
end

function Keyboard:__init(items)
    super(Keyboard).__init(self, items)
    self.keys = { }
    local keys = self.keys
    for _,k in ipairs(self.keymap) do
        if #k > 6 then
            for ch in k:gmatch('.') do table.insert(keys, ch) end
        else
            table.insert(keys, k)
        end
    end

    append(self.shift, self.caps)
    expand(self.shift)
    expand(self.caps)

    for i,v in ipairs(self.keys) do
        assert(v == translate(v))
        print(i, v, translate(v, self.shift), translate(v, self.caps))
    end

end

function Keyboard:layout(rect)
    local layout = GridLayout(rect) -- Create layout manager for control rect
    layout:layoutChildren(self)     -- Layout the children
end

function Keyboard:setState(shift, caps)
    local map
    if shift then map = self.shift
    elseif caps then map = self.caps end
    for _,button in ipairs(self:findByClass(Button)) do
        button:setText(translate(button.tag, map))
    end

    self:findByTag('Shift', true).background = shift and 'midnightblue' or nil
    self:findByTag('Caps', true).background = caps and 'midnightblue' or nil

    self:update()
end

-------------------------------------------------------------------
Keypad = KeyboardBase:subclass("Keypad")
Keypad.rows = 4
Keypad.columns = 4
Keypad.keys = {
    '1', '2','3', '-',
    '4', '5','6', '',
    '7', '8','9', '.',
    'Ent',  '0', 'Bsp'
}

Keypad.supported = {
    keys = charset.numeric .. '-.',
    control = { 'Ent', 'Bsp', '<-', '->', 'Esc' },
}

-------------------------------------------------------------------
KeypadTimecode = KeyboardBase:subclass("KeypadTimecode")
KeypadTimecode.rows = 4
KeypadTimecode.keys = {
    '1', '2','3',
    '4', '5','6',
    '7', '8','9',
    'Ent', '0', 'Bsp'
}
KeypadTimecode.supported = {
    keys = charset.numeric,
    control = { 'Ent', 'Bsp', '<-', '->', 'Esc' },
}

-------------------------------------------------------------------
KeypadUserbits = KeyboardBase:subclass("KeypadUserbits")
KeypadUserbits.columns = 5
KeypadUserbits.rows = 4
KeypadUserbits.keys = {
    '1', '2','3', 'A', 'B',
    '4', '5','6', 'C', 'D',
    '7', '8','9', 'E', 'F',
    'Ent',  '0',  'Bsp',
    --'', ''
}
KeypadUserbits.supported = {
    keys = charset.hex,
    control = { 'Ent', 'Bsp', '<-', '->', 'Esc' },
}

-------------------------------------------------------------------

