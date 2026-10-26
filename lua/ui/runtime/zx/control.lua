
function Control:isVisible()
    return self.visible and self.parent:isVisible()
end

--function TextEdit:onClick()
--    return self.setting and self.setting:edit()
--end

