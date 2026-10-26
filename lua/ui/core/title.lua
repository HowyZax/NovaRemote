---------------------------------------------------------------------
-- Title control
--
-- Represents a title string
---------------------------------------------------------------------
Title = Control:subclass('Title')
Title:attributes{ 'text', 'font', 'align' }
Title.align = 'center'
Title.canFocus = false
Title.color = 'gold'
function Title:__init(items)
    Control.__init(self, items)
    self._text = items[1]
    if not self.w then self.w = 0.5 end
    if not self.h then self.h = 0.1 end
    if not self.x then self.x = self.align == 'center' and (1-self.w)/2 or 0 end
end

-- Layout callback to allow Title control to override the
-- default position of the next control
-- For center-aligned title, allow next control to start
-- on next line down
-- For left-aligned title, allow next control to start
-- to the right of the title
function Title:onLayout(layout, rect, nextrect)
    return rect, Rect(0, rect.h, 1.0, 1.0)
    --[[
    if self.align == 'center' then
        return rect, Rect(0, rect.h, 1.0, 1.0)
    else
        return rect, Rect(rect.w, 0, 1.0, 1.0)
    end
    --]]
end


