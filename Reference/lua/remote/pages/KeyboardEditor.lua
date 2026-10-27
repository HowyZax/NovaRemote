-------------------------------------------------------------------
-- Delegate installed on a Keyboard control to handle interaction
-- with a text character buffer during editing.
-- Used for text, number and timecode editing.
KeyboardEditor = Object:subclass("KeyboardEditor")
KeyboardEditor:attributes{ 'notify', 'keyboard', 'buffer' }

local KeyboardEditorBuffer = Object:subclass('KeyboardEditorBuffer')

function KeyboardEditor:__init(items)
    super(KeyboardEditor).__init(self, items)
    self.buffer = items.buffer or KeyboardEditorBuffer()
    self.shift, self.caps = false, false
    assert(self.keyboard, 'KeyboardEditor requires a keyboard control')
    assert(self.notify, 'KeyboardEditor requires notification target object')
    self.keyboard:setDelegate(self)
end
function KeyboardEditor:completed(text)
    if self.notify and self.notify.completed then self.notify:completed(text) end
end
function KeyboardEditor:textChanged()
    if self.notify and self.notify.textChanged then self.notify:textChanged(self:text(), self.cursor) end
end
function KeyboardEditor:reset(text, maxchars)
    self.shift = false
    self.keyboard:setState(self.shift, self.caps)
    self.buffer:reset(text, maxchars)
    self:textChanged()
end
function KeyboardEditor:text()
    return self.buffer:value()
end
function KeyboardEditor:cursorMove(delta)
    self.buffer:cursorMove(delta)
end
function KeyboardEditor:insert(ch)
    assert(ch)
    self.buffer:insert(ch)
    if self.shift then
        self.shift = false
        self.keyboard:setState(self.shift, self.caps)
    end
    self:textChanged()
end

function KeyboardEditor:eventForButton(button)
    local text, tag = button:text(), button.tag
    if      tag  == 'Shift'  then return tag
    elseif  tag  == 'Caps'   then return tag
    elseif  text == 'Del'    then return tag
    elseif  text == 'Bsp'    then return text
    elseif  tag  == 'Bsp'    then return tag
    elseif  text == 'Ent'    then return text
    elseif  tag  == '->'     then return tag
    elseif  tag  == '<-'     then return tag
    elseif  tag  == 'Space'  then return tag
    elseif #text == 1        then return text
    end
end

function KeyboardEditor:eventForKey(key)
    if _TARGET == 'qt' then
        if     key == 'enter'   then return 'Ent'
        elseif key == 'escape'  then return 'Esc'
        --elseif key == 'left'    then return '<-'
        --elseif key == 'right'   then return '->'
        -- Other keys are handled by the underlying QLineEdit widget
        end
    else
        tprint(keys)
        print(keys.KEY_ASCII_FIRST)
        local ch = widget.keyToDigit(key)
        if ch then
        return string.char(ch + string.byte('0'))
        elseif key >= 0x20 and key <= 0x7F then
            return string.char(key)
        elseif key == keys.KEY_LEFT then return '<-'
        elseif key == keys.KEY_RIGHT then return '->'
        elseif key == keys.KEY_RETURN then return 'Ent'
        elseif key == keys.KEY_BACKSPACE then return 'Bsp'
        elseif key == keys.KEY_DELETE then return 'Del'
        elseif key == keys.KEY_ESCAPE then return 'Esc'
        end
    end
end

function KeyboardEditor:editEvent(event)
    if not self.keyboard:supports(event) then
        print(event,  'not supported')
        return
    end
    local ch
    if event == 'Shift' then
    self.shift = not self.shift
    self.keyboard:setState(self.shift, self.caps)
    elseif event == 'Caps' then
        self.caps = not self.caps
        self.keyboard:setState(self.shift, self.caps)
    elseif event == 'Del' then
        self.buffer:del()
    elseif event == 'Bsp' then
        self.buffer:backspace()
    elseif event == 'Ent' then
        self:completed(self:text())
        return
    elseif event == 'Esc' then
        self:completed()
        return
    elseif event == '->' then
        self:cursorMove(1)
    elseif event == '<-' then
        self:cursorMove(-1)
    elseif event == 'Space' then
        ch = ' '
    elseif #event == 1 then
        ch = event
    end
    if ch then self:insert(ch)
    else self:textChanged() end
end

function KeyboardEditor:onClickKeyboardButton(button)
    self:editEvent( self:eventForButton(button) )
end

function KeyboardEditor:onKeyPress(key)
    self:editEvent( self:eventForKey(key) )
end

-------------------------------------------------------------------
-- Raw keyboard character buffer
-- Note that Qt imlementation does not use this. Instead it uses the
-- underlying QLineEdit that implements the same methods
--
function KeyboardEditorBuffer:__init()
    self.cursor = 1
    self.characters = { }
end

function KeyboardEditorBuffer:reset(text, maxchars)
    self.maxchars = maxchars
    self.cursor = 1
    self.characters = { }
    for ch in text:sub(1, self.maxchars):gmatch('.') do table.insert(self.characters, ch) end
    self:cursorMove(#self.characters)
end

function KeyboardEditorBuffer:cursorMove(delta)
    self.cursor = self.cursor + delta
    if self.cursor < 1 then self.cursor = 1 end
    if self.cursor > #self.characters+1 then
        self.cursor = #self.characters+1
    end
    if self.cursor > self.maxchars then
        self.cursor = self.maxchars
    end
end

function KeyboardEditorBuffer:insert(ch)
    print(self.maxchars, self.cursor)
    tprint(self.characters)
    table.insert(self.characters, self.cursor,  ch)
    assert(self.maxchars)
    while #self.characters > self.maxchars do
        table.remove(self.characters, #self.characters)
    end
    self:cursorMove(1)
end

function KeyboardEditorBuffer:del()
    table.remove(self.characters, self.cursor)
end

function KeyboardEditorBuffer:backspace()
    if #self.characters == self.maxchars and self.cursor == self.maxchars then --
    else self:cursorMove(-1) end
    table.remove(self.characters, self.cursor)
end
function KeyboardEditorBuffer:value()
    return table.concat(self.characters)
end

