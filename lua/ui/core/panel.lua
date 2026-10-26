--------------------------------------------------------------------
-- Panel
--------------------------------------------------------------------
Panel = Control:subclass('Panel')
Panel:attributes{ '[]', 'columns', 'rows', 'spacing', 'grid', 'background' }
Panel.style = 'panel'
Panel.background = 0x505050
function Panel:__init(items)
    if items.background and items.background == true then
        items.background = Panel.background
    end

    Control.__init(self, items)

    self.visual = self.visual or (items.background ~= nil)

end

function Panel:__initdone()
    -- Messy workaround because this stuff was done too early if
    -- Panel{} is being constructed programmatically

    if type(self.columns) == 'table' then
        local widths = self.columns
        self.columns = #widths
        for i,c in ipairs(self) do
            if not c.w then c.w = widths[((i-1) % #widths) + 1] end
        end
    end
    if type(self.rows) == 'table' then
        local heights = self.rows
        self.rows = #heights
        for i,c in ipairs(self) do
            if not c.h then c.h = heights[((i-1) % #heights) + 1] end
        end
    end

--  if #self == 1 and not self.columns and not self.rows then self.rows = 1 end
    if not self.columns and not self.rows then
        if #self == 1 then self.rows = 1
        else self.columns = 1 end
    end

    if self.grid == nil then
        local grid = true
        for i,c in ipairs(self) do
            if c.x or c.y or c.w or c.h or c.onLayout then
                grid = false
                break
            end
        end
        self.grid = grid
    end
end

function Panel:setVisible(state)
    if self.widget then
        -- If panel is backed by a widget then just change the state of the panel
        super(Panel).setVisible(self, state)
    else
        -- Otherwise panel is for layout purposes so, change visibility 
        -- of immediate children
        self.visible = state
        for _,c in ipairs(self) do
            c:setVisible(state)
        end
    end
end

-- In current implementation, Spacer's are just panels used to
-- create empty space between controls
function Spacer(...)
    return Panel(...)
end
function HSpacer(tbl)
    local width = tbl and tbl[1] or 0.05
    return Spacer{w=width, id=tbl.id}
end
function VSpacer(tbl)
    local height = tbl and tbl[1] or 0.05
    return Spacer{h=height, id=tbl.id}
end


