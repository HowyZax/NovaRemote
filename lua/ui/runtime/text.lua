
function Label:create(parent)
    --print('','', 'Text.create in parent', parent.__classname)
    local w = widgets.create(self, 'widget', parent)
    w:setVisible(true)
    return w
end

function TextEdit:create(parent)
    --print('','', 'TextEdit.create in parent', parent.__classname)
    local w = widgets.create(self, 'textedit', parent)
    w:setText(self:text())
    w:setVisible(true)
    return w
end
function TextEdit:onPaint(painter)
    return true
end
function TextEdit:setGeometry(r)
    super(TextEdit).setGeometry(self, r)
    --self:updateFontSize()
end

function Title:create(parent)
    --print('','', 'Title.create in parent', parent.__classname)
    local w = widgets.create(self, 'widget', parent)
    w:setVisible(true)
    return w
end

--[[
function TextBox:create(parent)
    --print('','', 'TextEdit.create in parent', parent.__classname)
    local w = widgets.create(self, 'textbox', parent)
    w:setText(self:text())
    w:setVisible(true)
    return w
end
--]]
local fonts = require 'ui.widgets.qt.fonts'

function TextBox:append(s) self.widget:append(s) end
function TextBox:clear() self.widget:clear() end
function TextBox:text() return self.widget:text() end
-- Update font size, which is handled by the underlying widget
function TextBox:updateFont()
    local r = self:rect()
    self.widget:setFont( fonts.fitFont(self.maxchars or 20, r.w, r.h/10, self.font or 0.85) )
end
    
function TextBox:onResize(w, h)
    self:updateFont()
end


