---------------------------------------------------------------------
-- Text control
--
-- Represents a static text element
---------------------------------------------------------------------

Label = Control:subclass('Label')
Label:attributes{ 'elements', 'text', 'font', 'align', 'fit' }
Label.align = 'left'
Label.fit = true
--Label.color = 'white'
function Label:__init(items)
    self._text =  items and items[1]
    if not self.elements then
        self.elements = LayoutElement {
            TextElement{ text=self._text, background=items.background, id='.text', font=items.font, fit=items.fit or self.fit, align=items.align or self.align or 'left' }
        }
    end
    super(Label).__init(self, items)
end

function Label:setText(text)
    local element = self.elements and self.elements:findById('.text', false)
    if element then element:setContent(text) end
end
function Label:setFont(font)
    local element = self.elements and self.elements:findById('.text', false)
    if element then element.font = font end
end

function Label:onUpdate()
    local text = self:valueAsText()
    if text then self:setText(text) end
end

--[[
function Label:setBackground(color)
    local element = self.elements and self.elements:findById('.text', false)
    if element and element.setBackground then element:setBackground(color) end
end
function Label:text(text)
    local element = self.elements and self.elements:findById('.text', false)
    return element and element.text or ''
end
--]]

---------------------------------------------------------------------
Text = Label:subclass('Text')
Text.background = 0x909090
Text.color = 'white'
Text.style = 'textbox'

function Text:__init(items)
    if not self.elements then
        self.elements = LayoutElement {
            TextElement{ text=self._text, margin=0.04, id='.text', font=items.font or self.font, fit=items.fit }
        }
    end
    super(Text).__init(self, items)
end

function Text:onTouchRelease()
    if self:isEnabled() then
        self:perform('onClick')
    end
--  self:onClick()
end

---------------------------------------------------------------------

TextEdit = Control:subclass('TextEdit')
TextEdit:attributes{ 'text', 'font', 'align' }
TextEdit.canFocus = true           -- This type of control can receive focus (cursor)
TextEdit.align = 'center'
TextEdit.font = 'edittext'

TimecodeEdit = TextEdit:subclass('TimecodeEdit')
--TimecodeEdit.font = 'large'
function TimecodeEdit:create(parent, ...)
    super(TimecodeEdit).create(self, parent, ...)
    self.widget:setInputMask('00:00:00:00')
    self.widget:setAlignment('center')
end

UserbitsEdit = TextEdit:subclass('UserbitsEdit')
--TimecodeEdit.font = 'large'
function UserbitsEdit:create(parent, ...)
    super(UserbitsEdit).create(self, parent, ...)
    self.widget:setInputMask('>hh:hh:hh:hh')
    self.widget:setAlignment('center')
end


---------------------------------------------------------------------

TextBox = Control:subclass('TextBox')
TextBox.widgetclass = 'textbox' 


