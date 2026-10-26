local colors = require 'colors'
--{{{ Timecode:create
--[[
function Timecode:create(parent)
    --print('','', 'Timecode.create in parent', parent.__classname)
    local w = widgets.create(self, 'textedit', parent)

    return w
end
--]]
--}}}

function Timecode:onPaint(painter)
    local text = self:text() or ''
    local r = painter:rect()
    --print('Timecode:onPaint',  text)
    --painter:fitFont('00:00:00:00', r.w, r.h, self.font)
    painter:fitFont('0', r.w//9.5, r.h, self.font)
    --painter:box('yellow')
    local w = painter:textBounds('0')
    local ws = painter:textBounds(':')
    local tw = w*8 + ws*3
    --r.w = w
    local j = 1
--  local cursorx = self.cursor and 0
    r.x = r.x + (r.w - tw)//2
    for i = 1,11 do
        local c = text:sub(i,i) or '0'
--      local cprev = self.prevtext:sub(i,i) or 'X'
--        local c = ':'
--        if (i % 3) ~= 0 then
----          if cursorx ~= nil and self.cursor == j then cursorx = r.x end
--            c = text:sub(j,j)
--            c = #c > 0 and c or '?'
--            j = j + 1
--        end
        r.w = c==':' and ws or w
--      if c ~= ':' then
--          painter:text{'0', rect=r, align='right', fit=false, color=colors.translate('darkgreen')} --, color='black', background=toggle and 'blue' or 'red'}
--      end
--      local r1 = r:copy()
--      r1:setTop(r1:top() + 2)
--      r1:setLeft(r1:left() + 2)
--      painter:text{c, rect=r1, align='right', fit=false, color='darkgray'} --, color='black', background=toggle and 'blue' or 'red'}
        painter:text{c, rect=r, align='right', fit=false }
--      local offcolor = colors.translate('white') | 0xD8000000
--      painter:text{c == ':' and c or '0', rect=r, align='right', fit=false, color=offcolor} --, color='black', background=toggle and 'blue' or 'red'}
--      painter:text{c, rect=r, align='right', fit=false, color=colors.translate(self.color) } --, color='black', background=toggle and 'blue' or 'red'}
        r.x = r.x+r.w
    end
--  self.prevtext = text
--[[
    if cursorx then
        painter:setBrush('blue')
        painter:fillRect(Rect(cursorx,r.h/5,3,3*r.h/5))
    end
--]]
    return true
end
--[[
end
--]]


