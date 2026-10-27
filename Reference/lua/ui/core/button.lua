---------------------------------------------------------------------
-- Button controls
---------------------------------------------------------------------

Button = Control:subclass('Button')
Button:attributes{ 'text', 'font', 'align', 'page', 'graphic', 'fit', 'action' }
Button.align = 'center'
Button.background = 'black'
Button.style = 'button'
Button.canFocus = true         -- Cursor can focus on this control type
Button.fit = {8,}


local function TextElementsAdd(tbl, src, fit, font)
    local elements = { }
    if src and type(src) == 'string' then
        local smax = ''
        for line in string.gmatch(src or '', "[^|\n]+") do
            if #line > #smax then smax = line end
            table.insert(elements, TextElement{ text=line, id='.text', margin=0, font=font } )
        end
        if fit == nil and #smax > 0 then fit = smax end
        if fit then
            for _, element in ipairs(elements) do
                element.fit = fit
            end
        end
        for _,element in ipairs(elements) do
            table.insert(tbl, element)
        end
    end
end

function Button:__init(items)
    if not items.elements then
        items.elements = LayoutElement{ columns = 1, padding={ 0.05, 0.15 } }
        TextElementsAdd(items.elements, items[1], items.fit or self.fit, items.font or self.font)
        if items.graphic then
            table.insert(items.elements, GraphicElement{ graphic=items.graphic, margin=0.2, constrain=1, id='.graphic' } )
        end
        if items.setting then
            table.insert(items.elements, TextElement{ color='gold', id='.value', margin=0, fit={10,} } )
        end
    end
    super(Button).__init(self, items)
    self.page = items.page
end

---------------------------------------------------------------------
ButtonEx = Button:subclass('ButtonEx')

function Button:setText(text)
    local element = self.elements and self.elements:findById('.text', false)
    if element then element:setContent(text) end
end

function Button:setGraphic(graphic)
    local element = self.elements and self.elements:findById('.graphic', false)
    if element then
        element:setContent(graphic)
        element:setVisible(graphic ~= nil)
        self:layout(self:rect())
    end
end

-- TODO: remove Button:text()?
function Button:text(text)
    local element = self.elements and self.elements:findById('.text', false)
    return element and element.text or ''
end

--[[
function Button:graphic(text)
    local element = self.elements and self.elements:findById('.graphic', false)
    return element and element.graphic
end
--]]

-------------------------------------------------------------------
CheckedButton = Button:subclass('CheckedButton')
CheckedButton.constrain = {0.7, 1.5}
--CheckedButton:attributes{ 'group' }
function CheckedButton:__init(items)
    super(CheckedButton).__init(self, items)
    self.checked = items.checked or false
    --table.insert(self, GraphicElement { graphic = 'phase', margin = 0.2, } )
end

function CheckedButton:onClick()
    -- Note: This wont be called if button is in a CheckedGroup
    self:incValue()
end

function CheckedButton:onUpdate()
    if self.setting then
        self.checked = self:value()
    end
    --self.background = self.checked and 'green' or nil
end

-------------------------------------------------------------------
LatchedButton = ButtonEx:subclass('LatchedButton')
LatchedButton:attributes{ 'highlight' }
LatchedButton.highlight = 0xCF5300 --'orange' --0xC06000 --'darkorange'
LatchedButton.color = 'white'
LatchedButton.style = 'button.highlight'
function LatchedButton:__init(items)
    if not items.elements then
        items.elements = LayoutElement {
            TextElement {
                text = items[1] or '?',
                fit = items.fit,
                margin=0.1,
            }
        }
    end
    super(LatchedButton).__init(self, items)
    self.checked = items.checked or false
    --table.insert(self, GraphicElement { graphic = 'phase', margin = 0.2, } )
end

function LatchedButton:onUpdate()
    if self.setting then
        local value = self:value()
        self.checked = value and value==1
    end
    self.background = self.checked and self.highlight or nil
end

function LatchedButton:onClick()
    -- Note: This wont be called if button is in a CheckedGroup
    if self.setting then
        self:incValue()
    else
        self.checked = not self.checked
    end
    self:update()
end
-------------------------------------------------------------------
InfoButton = ButtonEx:subclass('InfoButton')

function InfoButton:__init(items)
    if not items.elements then
        items.elements = LayoutElement {
            rows = 1,
            GraphicElement { visible=false, w = 0.4, id = '.graphic', margin=0.05, constrain=1 },
            TextElement { id = '.text', fit=items.fit, margin={0.02, 0.05} },
        }
    end
    super(InfoButton).__init(self, items)
end

function InfoButton:onUpdate()
    if self.setting then
        local index = self:index()
        local text, graphic = '', nil
        if self.setting:indexIsValid(index) then
            text, graphic = self.setting:text(index), self.setting:graphic(index)
        end
        self:setText(text)
        self:setGraphic(graphic)
    end
end

function InfoButton:setGraphic(graphic)
    local ge = self.elements:findById('.graphic')
    ge:setVisible(graphic ~= nil)
    ge:setContent(graphic)
    local te = self.elements:findById('.text')
    te.align = graphic and 'left' or 'center'
    self:layout(self:rect())
end

-------------------------------------------------------------------
IncDecButton = ButtonEx:subclass('IncDecButton')
IncDecButton:attributes{
    'names',    -- setting mapping value to user-defined names
    'graphic',  -- array[2] of up/down graphics
}
IncDecButton.graphic = { 'down', 'up' }

function IncDecButton:__init(items)
    local text = items.text or items[1] or ''
    local graphic = items.graphic or self.graphic

    items.elements = LayoutElement{
         rows = 1,
         padding={ 0.05, 0.15 }
         --margin = {0.05, 0.1 },
--       GraphicElement{ graphic=graphic[2], constrain=1, w=0.17, background='red' },
--       GraphicElement{ graphic=graphic[1], constrain=1, w=0.17 },
     }

    table.insert(items.elements, GraphicElement{ graphic=graphic[1], constrain=1, w=0.17, id='inc' } )
--  table.insert(items.elements, RectElement{ w=0.01, color='gray', margin={0,0.18} } )
    do
        local textElements = LayoutElement{ columns = 1 }
        TextElementsAdd(textElements, text, items.fit or self.fit)
        if items.setting then
            table.insert(textElements, TextElement{ fit=true, color='gold', id='.value' } )
        end
        table.insert(items.elements, textElements)
    end
--  table.insert(items.elements, RectElement{ w=0.01, color='gray', margin={0,0.18} } )
    table.insert(items.elements, GraphicElement{ graphic=graphic[2], constrain=1, w=0.17, id='dec' } )

    super(IncDecButton).__init(self, items)

    if self.names then self.names = SettingFind(self.names) end
end

EditButton = IncDecButton:subclass('EditButton')
function EditButton:__init(items)
    super(EditButton).__init(self, items)
    self.selected = false
end


