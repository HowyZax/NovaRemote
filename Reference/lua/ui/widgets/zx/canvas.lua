-- Extend library and class tables. See lcanvas.cpp

local core = require 'canvas.core'

Canvas = core.CANVAS
Image = Canvas
lcdCanvas = PageCanvas()

assert(Canvas.__classname == "CANVAS")

assert(Canvas.update)

local Canvas_update = Canvas.update

-- Per Canvas state
local State = setmetatable({ }, { __mode = 'k' })
local function state(c)
    local t = State[c]
    if not t then t = { }; State[c] = t end
    return t
end

-- Following replaces the default Canvas:update() method
-- with a version that caches updates until next call to
-- Canvas:sync()
function Canvas:update(x, y, w, h)
    local SELF = state(self)
    SELF.pending = SELF.pending or  { }
    if type(x) == 'number' then
        if not SELF.pending.full then
            local newrect = Rect(x,y,w,h)
            for _,r in ipairs(SELF.pending) do
                if r == newrect then
                    --print('duplicate rect',  newrect)
                    return
                end
            end
            table.insert(SELF.pending, newrect)
        end
    else
        SELF.pending = { full = true }
    end
    --print("Canvas:update pending:",  #self.pending, self.pending.full)
    --self:sync()
end

function Canvas:sync()
    local SELF = state(self)
    --print('Canvas:sync', #self.pending, self.pending.full)
    if SELF.pending.full then
        Canvas_update(self)
        SELF.pending  = { }
    elseif #SELF.pending then
        for _,r in ipairs(SELF.pending) do
            Canvas_update(self, r.x, r.y, r.w, r.h)
        end
        SELF.pending  = { }
    end
end

Canvas.image = Canvas.copy

return core
