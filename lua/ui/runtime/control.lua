local colors = require 'colors'

function Control:create(parent, ...)
    -- Create widget. Defaults to non-visible.
    local w = widgets.create(self, self.widgetclass or 'widget', parent, ...)
    super(Control).create(self, w)

    -- Make visible by default, unless control specifies self.visible = false
    assert(self.visible ~= nil)

    if self.visible then    
        self:setVisible(self.visible)
    end

    return w
end

-- Set the text color
function Control:setColor(color)
    self.color = colors.translate(color)
end

-- Set the background color/style
function Control:setBackground(color)
    self.background = colors.translate(color)
end

-- Default paint handler for a control
function Control:onPaint(painter)
    --  self:paintBackground(painter)
    if self.elements then
        self.elements:onPaint(painter)
    end
    return true
end
function Control:onPaintBackground(painter)
    self:paintBackground(painter)
end

-- Return rect occupied by Control relative to parent
function Control:rect()
    return self:geometry()
end

function Control:draw()
    if self.widget then
        self.widget:draw()
    end
end
function Control:update()
    self:perform('onUpdate')
    self:draw()
    for _,c in ipairs(self) do
        if c.update then c:update() end
    end
end
function Control:setText(s)
    self._text = s and tostring(s) or ''
    if self.widget and self.widget.setText then
        return self.widget:setText(s)
    end
end

function Control:onClick()
    if self.page then
        Page.Enter(self.page, self:findParentByClass(Page))
    elseif not self.readonly then
        local setting = self.setting
        print('Control:onClick',  self.__classname, self.id, self.tag, setting and setting.name)
        return setting and setting:edit(self, self)
    end

end

function Control:hasFocus()
    return self.widget and self.widget:hasFocus() or false
end
function Control:hasTouchFocus()
    return self.widget and self.widget:hasTouchFocus() or false
end

--[[
function Control:onTouchPress(x, y)
    print 'Control.onTouchPress'
    self:update()
    return true
end
function Control:onTouchRelease(x, y)
    self:update()
    return true
end
--]]
function Control:onTouchMove(x, y)
    print('onTouchMove', self, x, y, self.widget:globalPos(x,y))
    if self.drag then
        self.widget:dragDrop()
        return true
    end
end

function Control:onDragEnter()
    print('Control:onDragEnter()', self.__classname)
end

function Control:onDragMove()
    print('Control:onDragMove()', self.__classname)
end

function Control:onDragLeave()
    print('Control:onDragLeave()', self.__classname)
end


function Control:onDrop( _srcWidget)
    local source = _srcWidget:owner()
    print('Control:onDrop()', self.__classname, 'from source', source.__classname, source.dropValue and source:dropValue())
end

function Control:onDropValue()
    return self:valueAsText()
end

function Control:dropValue()
    return self:perform('onDropValue')
end

