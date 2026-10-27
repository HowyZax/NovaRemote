local fonts = require 'ui.widgets.qt.fonts'

-- Update font size, which is handled by the underlying widget
function TextEdit:updateFont()
    local r = self:rect()
    self.widget:setFont( fonts.fitFont(self.maxchars or 20, r.w, r.h, self.font or 0.85) )
end

-------------------------------------------------------------------
-- Following implement the same methods as KeyboardEditorBuffer
-- to allow the underlying QLineEdit widget to act as the text buffer
-- for KeyboardEditor-based dialog.
--
-- This allows the use of standard QLineEdit mechanisms, including
-- keyboard input and copy & paste, in addition to KeyboardEditor's virtual
-- keyboard buttons.

function TextEdit:reset(text, maxchars)
    self.maxchars = maxchars
    self.widget:setMaxLength(maxchars)
    self.widget:setText(text)
    self.widget:selectAll()
    self:updateFont()
end

function TextEdit:cursorMove(delta)
    self.widget:cursorMove(delta)
end

function TextEdit:insert(ch)
    self.widget:insert(ch)
end

function TextEdit:del()
    self.widget:del()
end

function TextEdit:backspace()
    self.widget:backspace()
end

function TextEdit:value()
    return self.widget:text()
end

function TextEdit:onResize(w, h)
    self:updateFont()
end

