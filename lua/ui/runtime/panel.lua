function Panel:create(parent)

    self:__initdone() -- TODO: review!

    if self.visual then
        -- If panel has a background then it must be backed by a widget
        local w = super(Panel).create(self, parent)
        --w:setMsgFilter('draw', false)
        return w
    else
        -- Otherwise, panel is container for layout purposes only
        if self.visible == nil then self.visible = true end
        return super(Panel).createChildren(self, parent)
    end

end
--[[
function Panel:onPaint(painter)
    self:paintBackground(painter)
end
--]]


