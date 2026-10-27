Painter = class('Painter')

local LcdCanvasPainter
function Painter.create(o)
    if o.__classname == 'CANVAS' then
        local painter = Painter(o)
        painter:initFrom(o)
        return painter
    end
    if not LcdCanvasPainter then
        LcdCanvasPainter = Painter(lcdCanvas)
    end
    LcdCanvasPainter:initFrom(o)
    return LcdCanvasPainter
end

function Painter:__init(canvas)
    print('Painter.__init',  self,  canvas.__classname)
    self.canvas = canvas
    self:initFrom(canvas)
end

function Painter:initFrom(o)
    print('Painter:initFrom', o.__classname)
    if o.geometry then
        self.x, self.y, self.w, self.h = o:geometry():unpack()
    else
        self.x, self.y, self.w, self.h = 0, 0, o:size()
    end
    self.pencolor = 'on'
    self.brushcolor = 'white'
    self:setFont(o.font or 'normal')
end

function Painter:rect()
    return Rect(0, 0, self.w, self.h)   -- Return client rect
end

function Painter:setPen(pen)
    self.pencolor = pen
end

function Painter:setFont(font)
    self.font = font
end

function Painter:setBrush(brush)
    self.brushcolor = brush
end

function Painter:fillRect(r, color)
    self.canvas:fillRect(self.x + r.x,
                         self.y + r.y,
                         self.x + r.x + r.w - 1,
                         self.y + r.y + r.h - 1,
                         color or self.brushcolor)
end

function Painter:fill(color)
    self.canvas:fillRect(self.x,
                         self.y,
                         self.x + self.w - 1,
                         self.y + self.h - 1,
                         color or self.brushcolor)
end

function Painter:box(...)
    local arg1 = ...

    if type(arg1) == 'string' or arg1 == nil then
        local color = ...
        self.canvas:box(self.x,
                        self.y,
                        self.x + self.w - 1,
                        self.y + self.h - 1, color or self.pencolor)
    else
        local r, color = ...
        self.canvas:box(self.x + r.x,
                        self.y + r.y,
                        self.x + r.x + r.w - 1,
                        self.y + r.y + r.h - 1, color or self.pencolor)
    end
end

function Painter:text(...)
    local arg1 = ...
    local x, y, s, align
    local r

    if type(arg1) == 'number' then
        x, y, s, align = ...                -- Argument format 1
        -- Change optional relative coordinates to absolute
        if x < 1 and x > 0 then x = x * self.w end
        if y < 1 and y > 0 then y = y * self.h end
    elseif isinstance(arg1, Rect) then
        r,s,align = ...                     -- Argument format 2
    else
        s,align = ...                       -- Argument format 3
        r = self:rect()
    end

    if r then
        align = align or 'center'
        x,y = r:center()
        local w,h = self:textBounds(s)
        y = y - math.floor(h/2)
        if align == 'right' then x = r:right()
        elseif align == 'left' then x = r:left() end
    else
        if align == 'center' then
            local w,h = self:textBounds(s)
            y = y - math.floor(h/2)
        elseif align == 'vcenter' then
            local w,h = self:textBounds(s)
            y = y - math.floor(h/2)
            align = 'left'
        end
    end
    local xref, yref = x,y

    local lines = (type(s) == 'table') and s or { s } -- s can be a string or a multi-line table of strings
    for _,line in ipairs(lines)  do
        local x1, y1 = x, y
        local w,h = self.canvas:textBounds(line, self.font)
        if align == 'right' then x1 = x - w
        elseif align == 'center' then x1 = x - math.floor(w/2) end
        self.canvas:text(self.x + x1, self.y + y1, line, self.font, self.pencolor)
        y = y + h
    end
end

function Painter:textBounds(s)
    local htotal = 0
    local wmax = 0
    local lines = type(s) == 'table' and s or { s }
    for _,line in ipairs(lines) do
        local w, h = self.canvas:textBounds(line, self.font)
        if w > wmax then wmax = w end
        htotal = htotal + h
    end
    return wmax, htotal
end

function Painter:image(...)
    local x, y, img, r = ...
    assert(img.__classname == 'CANVAS') -- currently assume actually a Canvas object
    if r then
        if type(r) == 'table' then
            self.canvas:copy(self.x + x, self.y + y, img, r:unpack())
        else
            local x, y, img, xother, yother, width, height = ...
            self.canvas:copy(self.x + x, self.y + y, img, xother, yother, width, height)
        end
    else
        self.canvas:copy(self.x + x, self.y + y, img)
    end
end

function Painter:update()
    self.canvas:update(self.x, self.y, self.w,self.h)
end


