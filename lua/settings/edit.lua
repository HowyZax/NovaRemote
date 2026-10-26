
-- Invoke editor for setting
--
-- control = originating control
-- target = object where edit result is sent, or nil to send the setting (self)
function SettingBase:edit(control, target)
    local editor = self.editor
    if type(editor) == 'string' then
        assert(pages[editor])
        editor = pages[editor].delegate
    end
    if editor then
        --editor:show(target or self, self.prompt or 'Enter ' .. (self.description or self.name or self.__classname), self.maxchars, control)
        local target = target or self
--      editor:show(target:text(), completion,
--                  self.prompt or 'Enter ' .. (self.description or self.name or self.__classname), self.maxchars, control)
        print ('SettingBase.edit', self:text(),  self:value(),  self:rawtext(),  target:text())
        editor:show{
            text = target:text(),
            callback = function(s) target:setValue(s) end,
            prompt = self.prompt,
            setting = self,
            control = control
        }
        return true
    end
    return false
end

