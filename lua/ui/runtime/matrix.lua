
--{{{ MatrixCell:create
function MatrixCell:create(parent)
    --print('','', 'MatrixCell.create in parent', parent.__classname)
    local w = widgets.create(self, 'button', parent)
    w:setVisible(true)
    return w
end
--}}}
--{{{ Matrix:newCell
function Matrix:newCell(row, col)
    local setting = self.setting
    if self.settings then
        assert(not self.setting)
        setting = self.settings[(row-1)*self.columns + col]
    end
    return self.cellClass{ row = row, col = col, matrix=self, delegate=self.delegate, id=(self.id or '')..'Cell', drag=self.drag, drop=self.drop, style=self.style, setting=setting, index=self.cellIndex, fit=self.fit }
end
--}}}
--{{{ Matrix:create
function Matrix:create(parent)
    -- Create a sub-panel populated with matrix cells and text labels
    local panel = Panel { w = 1.0, h = 1.0 }
    self[#self + 1] = panel
    panel.rows, panel.columns = self.rows, self.columns
    local function newLabel(text, cell)
        local label = Label { text, margin=0, align='center', fit=true,
                                id=(self.id or '')..'CellLabel', 
                                delegate=self.delegate, index=self.cellIndex }
        label.cell = cell
        return label
    end
    if self.rowtitles then panel.columns = panel.columns + 1 end
    if self.coltitles then
        panel.rows = panel.rows + 1
        for col = 1,self.ncols do
            panel[#panel+1] = newLabel(self.coltitles[col])
        end
        if self.rowtitles then
            panel[#panel+1] = Spacer { }
        end
    end

    if self.cellFactory then
        local cell = self.cell or { }
        local idx = 1
        cell.fit = cell.fit or self.fit
        for row = 1,self.nrows do
            for col = 1,self.ncols do
                if self.ncells and idx > self.ncells then 
                    break 
                end
                local c = self:cellFactory(cell, row, col, idx)
                if c then panel[#panel+1] = c end
                idx = idx + 1
            end
        end
    elseif self.cellTitles then
        local cellTitles = self.cellTitles
        --if cellTitles and type(cellTitles) ~= 'function' then
            --cellTitles = function(row, col) return tostring((row-1)*self.ncols + col) end
        --end
        panel.rows = panel.rows * 2
        for row = 1,self.nrows do
            local cells = { }
            for col = 1,self.ncols do
                local cell = self:newCell(row, col)
                cells[#cells+1] = cell
                local title = type(cellTitles) == 'function' and cellTitles(self, row, col, (row-1)*self.columns + col) or cell:title()
                panel[#panel+1] = newLabel(title, cell)
            end
            for _,cell in ipairs(cells) do
                panel[#panel+1] = cell
            end
        end
    else
        for row = 1,self.nrows do
            for col = 1,self.ncols do
                panel[#panel+1] = self:newCell(row, col)
            end
            if self.rowtitles then
                panel[#panel+1] = newLabel(self.rowtitles[row])
            end
        end
    end
    Control.createChildren(self, parent)
end
--}}}
function Matrix:foreachTitle(fn)
    local row, col = 1, 1
    self:traverse(function(o)
            if isinstance(o, 'Label') then
                if col <= self.columns then  fn(o, nil, col); col = col + 1
                elseif row <= self.rows then fn(o, row, nil); row = row + 1
                end
            end
        end)
end

function Matrix:update()
    if self:isVisible() then
        self:traverse(function(o) if o ~= self and o.update then o:update() end end )
    end
end


function MatrixCell:index()
    return self:perform('onIndex') or {self.row, self.col}
end


