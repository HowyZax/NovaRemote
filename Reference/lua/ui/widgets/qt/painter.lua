local core = require 'qt.core'
local colors = require 'colors'
local fonts = require 'ui.widgets.qt.fonts'
require 'class'

local QPainter = core.QPainter

Painter = class('Painter')

local LcdCanvasPainter
function Painter.create(o)
    if o.__classname == 'Image' then
        local painter = Painter( QPainter.create(o) )
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
    --print('Painter.__init',  self,  canvas.__classname)
    self.canvas = canvas
    self:initFrom(canvas)
end

function Painter:initFrom(o)
--  print('Painter:initFrom', o.__classname)
    if o.geometry then
        self.x, self.y, self.w, self.h = o:geometry():unpack()
    else
        self.x, self.y, self.w, self.h = 0, 0, o:size()
    end
    self.pencolor = 'on'
    self.brushcolor = 'white'
    self.font = nil
    --self.canvas:setFont('Arial', 'bold')
end

function Painter:rect()
    return Rect(0, 0, self.w, self.h)   -- Return client rect
end

function Painter:setPen(pen)
    self.pencolor = colors.translate(pen)
    self.canvas:setPen(self.pencolor)
end

function Painter:setFont(font)
    assert(isinstance(font, 'QFont'))
    self.font = font
    self.canvas:setFont(font)
end

function Painter:setBrush(brush)
    self.canvas:setBrush(brush)
end

function Painter:fillRect(r, color)
    self.canvas:fill(r.x, r.y, r.w, r.h, colors.translate(color))
end

function Painter:fill(arg)
    if type(arg) == 'table' then
        local r = arg.rect or Rect(arg.x or 0, arg.y or 0, arg.w or self.w, arg.h or self.h)
        r = r:absolute(self.w, self.h)
        local color = arg[1] or arg.color or 'red'
        self:fillRect(r, color)
    else
        local color = arg
        local w, h = self.canvas:size()
        self.canvas:fill(0, 0, w, h, color)
    end
end

function Painter:gradient(...)
    local arg = ...
    if type(arg) == 'table' then
        local r = arg.rect or Rect(arg.x or 0, arg.y or 0, arg.w or self.w, arg.h or self.h)
        r = r:absolute(self.w, self.h)
        local color = arg[1] or arg.color or 'red'
        local style = arg[2] or arg.style
        local state = arg[3] or arg.state
        self.canvas:gradient(r.x, r.y, r.w, r.h, color, style, state)
    else
        local color, style, state = ...
        local w, h = self.canvas:size()
        self.canvas:gradient(0, 0, w, h, color, style, state)
    end
end

function Painter:box(...)
    local arg1 = ...
    if type(arg1) == 'string' or arg1 == nil then
        local color = ...
        self.canvas:box(0, 0, self.w, self.h, color)
    else
        local r, color = ...
        self.canvas:box(r.x, r.y, r.w, r.h, color)
    end
end

function Painter:text(...)
    local arg1 = ...
    local x, y, s, align
    local r
    local opts = { }

    if type(arg1) == 'number' then
        x, y, s, align = ...                -- Argument format 1
        -- Change optional relative coordinates to absolute
        if x < 1 and x > 0 then x = x * self.w end
        if y < 1 and y > 0 then y = y * self.h end
    elseif isinstance(arg1, Rect) then
        r,s,align = ...                     -- Argument format 2
    elseif type(arg1) == 'table' then
        opts = arg1
        s = opts[1] or ''
        r = opts.rect
        if not r then
            r = Rect(opts.x or 0, opts.y or 0, opts.w or self.w, opts.h or self.h):absolute(self.w, self.h)
        end
        align = opts.align or 'center'

        local fit
        if opts.fit then
            if type(opts.fit) == 'boolean' then
                fit = s
            elseif type(opts.fit) == 'table' then
                if #s < opts.fit[1] then fit = opts.fit[1]
                elseif opts.fit[2] and #s > opts.fit[2] then fit = opts.fit[2]
                else fit = #s end
            else
                fit = opts.fit
            end
        elseif opts.fit == nil then
            fit = s
        end
        if fit then
            r:clip(self.w, self.h)
            self:fitFont(fit, r.w,  r.h, opts.font or 1.0)
        end
        if opts.background then
            self:fillRect(r, opts.background)
        end
        if opts.color then
            self:setPen(opts.color)
        end
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
        local w,h = self:textBounds(line)
        if align == 'right' then x1 = x - w
        elseif align == 'center' then x1 = x - math.floor(w/2) end
        if opts.overline then
            --self.font:setFont('overline')
            ---[[
            local yb = y1-h/8
            if yb <= 0 then yb = 1 end
            local oh = h/8
            if oh > 2 then oh = 2 end
            if oh < 1 then oh = 1 end
            self.canvas:fill(x1,yb,w,oh, colors.translate(self.pencolor))
            --]]
        end
        self.canvas:text(x1, y1, line)
        y = y + h
    end
end

function Painter:fitFont(s, w, h, font)
    self:setFont( fonts.fitFont(s, w, h, font) )
end

function Painter:textBounds(s)
    local htotal = 0
    local wmax = 0
    -- TODO: assume single line of text
    --assert(type(s) ~= 'table')
    local lines = type(s) == 'table' and s or { s }
    for _,line in ipairs(lines) do
        assert(self.font)
        local w, h = self.font:textBounds(line)
        if w > wmax then wmax = w end
        htotal = htotal + h
    end
    return wmax, htotal
end

function Painter:image(...)
    self.canvas:image(...)
end

function Painter:graphic(...)
    local arg = ...
    if type(arg) == 'table' then
        local graphic = arg[1]
        local r = Rect(arg.x or 0, arg.y or 0, arg.w or self.w, arg.h or self.h):absolute(self.w, self.h)
        r:clip(self.w, self.h)
        self.canvas:graphic( graphic, r )
    else
        self.canvas:graphic(...)
    end
end

function Painter:update()
    -- do nothing
end
