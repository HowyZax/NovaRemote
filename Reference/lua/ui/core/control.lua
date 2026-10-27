--==================================================================
-- CONTROLS
--==================================================================
-- The following classes represent the Control objects used in the
-- C code.

--------------------------------------------------------------------
-- Control class - bass class with common support for all controls
--------------------------------------------------------------------
Control = Object:subclass('Control')
Control:attributes{ 'x','y','w','h', 'bottom', 'name', 'margin', 'padding', 'onLayout', 'onPaint', 'id', 'tag', 'onClick', 'parent', 'setting', 'index', 'canFocus', 'style', 'drag', 'drop', 'readonly', 'constrain', 'page', 'background', 'color', 'style', 'elements' }
Control.compound = false

function Control:__init(items)
    Object.__init(self, items)

    assert(items.text == nil)   -- .text is a function!

    self.w = items.width or items.w
    self.h = items.height or items.h

    if self.visible == nil then
        self.visible = true
    end

    if items.setting then
        self.setting = SettingFind(items.setting)
    end
    if type(self.index) == 'number' then
        -- TODO: remove in due course?
        local n = items.index
        self.index = function(self) return n end
    elseif type(self.index) == 'string' then
        local x = SettingFind(items.index)
        self.index = function(self) return x:value() end
    end
    assert(type(self.index) == 'function')

    self:buildUITable()
end

-------------------------------------------------------------------
function Control:buildUITable()
--  if self.ui then return end
    assert(not self.ui or type(self.ui) == 'table')
    self.ui = { }
    function addChildIds(parent)
        for _,o in ipairs(parent) do
            if o.id then
                local k = o.id:lower()
                self.ui[k] = self.ui[k] or o
                if self.compound then
                    assert(not o.delegate)
                    o.delegate = self
                end
            end
            if not o.compound then
                addChildIds(o)
            end
        end
    end
    if self.elements then
        assert(isinstance(self.elements, LayoutElement))
        addChildIds(self.elements)
    end
    if self.compound then
        addChildIds(self)
    end
end

-------------------------------------------------------------------
-- Data access methods for associated setting, if any

function Control:value()
    if self.setting then
        return self.setting:value(self:index())
    end
end
function Control:setValue(v)
    if self.setting then
        self.setting:setValue(v, self:index())
    end
end
function Control:incValue(step)
    if self.setting then
        self.setting:incValue(self:index(), step)
    end
end
function Control:index()
    return self:perform('onIndex')
end
function Control:valueAsText()
    if self.setting then
        return self.setting:text(self:index())
    end
end
function Control:valueAsGraphic()
    if self.setting then
        return self.setting:graphic(self:index())
    end
end
function Control:text()
    if not self._text and self.ui['.text'] then
        return self.ui['.text'].text or ''  -- use text from most likely TextElement
    end
    return self._text -- or self:valueAsText() or '???'
end

-------------------------------------------------------------------
-- Custom control layout
-- Layout child controls in the specified rectangle, which is relative
-- to this control. The position of this control is not modified.
function Control:layout(rect)
    if not rect then rect = self:rect() end
    if rect then
--      print ('Control:layout', self.__classname, rect, self.rect)
        local r = self.widget and Rect(0,0,rect.w,rect.h) or rect:copy()
        -- Create layout manager for control rect
        local layout = self.grid and GridLayout(r) or Layout(r)
        layout:layoutChildren(self)     -- Layout the children
    end
    if self.elements then
        local r = self:rect()
        local layout = Layout( Rect(0, 0, r.w, r.h) )
        layout:layoutChildren(self.elements)
    end

end

function Control:setGeometry(rect)
    -- Function called from layout manager to set the position of
    -- the control & associated widget relative to its parent
    -- Avoid calling this directly from anywhere but the layout
    -- manager to ensure that the control is positioned according
    -- to the various layout rules
--  print('Control:setGeometry', rect, self.__classname, self.id)
    if self.widget then
        self.widget:setGeometry(rect:unpack())
    else
        self.currentGeometry = rect
    end
end

function Control:geometry()
    if self.widget then
        local x,y,w,h = self.widget:geometry()
        return Rect(x,y,w,h)
    else
        return self.currentGeometry:copy()
    end
end

function Control:setVisible(visible)
    if visible == nil then visible = true end
    if self.widget then self.widget:setVisible(visible) end
    self.visible = visible
end

function Control:isVisible()
    return self.visible
end

function Control:parentWidget()
    local parent = self.parent
    while parent do
        if parent.widget then return parent.widget end
        parent = parent.parent
    end
end

function Control:setParent(parent)
    self.parent = parent
    if self.widget then
        self.widget:setParent( self:parentWidget() )
    end
end

