require 'class'

-- Helper class to layout child controls
--
-- Layout class replaces the Nomad Page:layout implementation in uicore32/init.lua.
-- For layouts that don't rely Qt QLayout objects.
-- Coordinates and dimensions can be in absolute pixels or fractional (relative
-- to the screen size).
--
-- Lays out the controls on this page. Performs semi-intelligent positioning
-- of the controls so the UI definition does not need to specify screen
-- coordinates for all controls.
-- Also performs other processing of the page controls prior to code
-- generation.

Layout = class('Layout')
function Layout:__init(parentRect)
    assert( isinstance(parentRect,  Rect) )
    self.parentRect = parentRect
end

local function visibleChildren(parent)
    local t = {}
    for _,o in ipairs(parent) do
        if o.visible == nil or o.visible then table.insert(t, o) end
    end
    return t
end

function Layout:layoutChildren(parent, children)
    if children == nil then
        children = visibleChildren(parent)
    end
    if #children == 0 then return end

    local left, top, width, height = self.parentRect:unpack()
    local prevrect

    if parent.padding then
        left, top, width, height = self.parentRect:shrink(parent.padding):unpack()
    end

    local function subz(x, y)
        return x > y and x - y or 0
    end

    local wfloat, hfloat = width, height

    -- Setup variables for grid layouts. Vertical and horizontal layouts
    -- are just a special case of a grid with rows=1 or columns=1
    -- If the parent does not specify a grid-like layout then the
    -- layout algorithm is based on Nomad "vertical flow" layout in which
    -- the next layout rect is the same size as the previous rect but
    -- directly  below it, unless the control specifies a change to one
    -- or more of the rect parameters (x,y,w,h)
    -- TODO: Maybe don't count controls with custom onLayout handlers?
    local row, col = 0, 0
    local rows, columns
    local dflt = { }
    local spacing = parent.spacing or 0
    if parent.columns then
        assert(parent.columns > 0)
        columns = parent.columns
        rows = math.floor((#children+columns-1)/columns)
        if columns == 1 then
            local floating= 1.0
            local fixed = 0
            local n = 0
            for i,control in ipairs(children) do
                if control.h and control.h <= 1.0 then
                    floating = floating - control.h
                elseif control.h and control.h > 1.0 then
                    fixed = fixed + control.h
                else n = n + 1 end
                if i > 1 then floating = floating - spacing end
            end
            assert(floating >= 0)
            if n then dflt.h = floating/n end
            hfloat = subz(hfloat, fixed)
            spacing = spacing*hfloat
            --print('dflt.h = ',  dflt.h, floating, n, spacing)
        end
    elseif parent.rows then
        assert(parent.rows > 0)
        rows = parent.rows
        columns = math.floor((#children+rows-1)/rows)
        if rows == 1 then
            local floating = 1.0
            local fixed = 0
            local n = 0
            for i,control in ipairs(children) do
                if control.w and control.w < 1.0 then
                    floating = floating - control.w
                elseif control.w and control.w >= 1 then
                    fixed = fixed + control.w
                else n = n + 1 end
                if i > 1 then floating = floating - spacing end
                assert(floating >= 0)
            end
            if n then dflt.w = floating/n end
            wfloat = subz(wfloat, fixed)
            spacing = spacing*wfloat
            --print('dflt.w = ',  dflt.h, floating, n, spacing)
        end
    end

    -- Calcuations are performed relative to the parent origin
    -- but delivered to the control in absolute coorindates

    local r = Rect(0, 0, columns and width/columns or width, rows and height/rows or height)

    --print('layout:layoutChildren', parent.id, #children, parent.__classname, parent.widget, self.parentRect)

    -- Set the control geometry
    for _,control in ipairs(children) do
        r.w = dflt.w or r.w
        r.h = dflt.h or r.h
        local rect = Rect(control.x, control.y, control.w, control.h,  r)
        rect = rect:absolute(wfloat, hfloat)

        if control.bottom then
            local bottom = control.bottom <= 1.0 and control.bottom*height or control.bottom
            rect.h = bottom - rect.y + 1
        end

        -- Advance for next control
        if parent.columns then
            col = col + 1
            if col >= columns then
                r.x, r.y = 0, rect.y + rect.h
                row, col = row + 1, 0
                r.y = r.y + spacing
            else
                r.x = rect.x + rect.w
            end
        elseif parent.rows then
            row = row + 1
            if row >= rows then
                r.x, r.y = rect.x + rect.w, 0
                row, col = 0, col + 1
                r.x = r.x + spacing
            else
                r.y = rect.y + rect.h
            end
        else
            r.x, r.y = rect.x, rect.y + rect.h
        end
        r.w, r.h = rect.w, rect.h

        -- Allow control to make layout customizations

        if control.onLayout then
            local newrect, nextrect = control:onLayout(self, rect, r)
            if newrect then rect = newrect:absolute(wfloat, hfloat) end
            if nextrect then r = nextrect:absolute(wfloat, hfloat) end
        end

        -- Clip control to screen

        rect:clip(width, height)
        rect:translate(left, top)

        --print('....layout =', control.__classname, control.widget, rect.x, rect.y, rect.w, rect.h, control.x, control.y, control.w, control.h, control.widget, control.xoffset, control.yoffset)

        -- Set new geometry and layout control's children
        if control.constrain then
--          print('CONSTRAIN!!!', control.__classname)
        end
        local mrect = rect:constrained(control.constrain)
        mrect = mrect:shrink(control.margin or 2)
--      print (control.__classname, 'setGeometry', mrect)
        control:setGeometry(mrect)
        if control.layout then
            control:layout(mrect)
        end

        prevrect = rect:copy()
    end
end

-------------------------------------------------------------------
GridLayout = class('GridLayout')
function GridLayout:__init(parentRect)
    assert( isinstance(parentRect,  Rect) )
    self.parentRect = parentRect
end
function GridLayout:layoutChildren(parent, children)
    if children == nil then
        children = visibleChildren(parent)
    end
    if #children == 0 then return end

    local left, top, width, height = self.parentRect:unpack()
    if parent.padding then
        left, top, width, height = self.parentRect:shrink(parent.padding):unpack()
    end
    local rows, columns = parent.rows, parent.columns
    assert(rows or columns, 'GridLayout requires number of rows or columns (or both) to be specified for ' .. parent.__classname)
    rows = rows or math.floor((#children + columns-1)/columns)
    columns = columns or math.floor((#children + rows-1)/rows)
    local hints = parent.hints
    local r,c = 1,1
    local wcell = math.floor(width/columns)
    local hcell = math.floor(height/rows)
    local xoffset = math.floor((width  - (wcell * columns))/2)
    local yoffset = math.floor((height - (hcell * rows))/2)

    width = width - xoffset*2
    height = height - yoffset*2
    left = left + xoffset
    top = top + yoffset

    local usedtbl = { }
    local function used(r,c,v)    -- Get/set state of row,col
        local idx = (r-1)*columns + c
        --print(r,c,idx,columns)
        if v == nil then return usedtbl[idx]
        else usedtbl[idx] = v end
    end
    local function inc(r,c)
        c = c + 1
        if c > columns then r,c = r+1,c - columns end -- wrap to next row
        return r,c
    end
    for _,control in ipairs(children) do
        local p = hints and hints[control:text()] or { }
        while used(r,c) do r,c = inc(r,c) end
        local x = (c-1)*wcell
        local y = (r-1)*hcell
        local w = wcell * (p.w or 1)
        local h = hcell * (p.h or 1)
        local rect = Rect(x,y,w,h)

        --print(k or '', r, c, x, y, w, h, xoffset, yoffset)
        assert(c <= columns)

        rect:clip(width, height)
        rect:translate(left, top)

        -- Set new geometry and layout control's children

        local mrect = rect:constrained(control.constrain)
        mrect = mrect:shrink(control.margin or 2)
        control:setGeometry(mrect)
        control:layout(mrect)

        for i=1,(p.h or 1) do
            for j=1,(p.w or 1) do used(r+i-1,c+j-1,true); end
        end
    end
end


