--------------------------------------------------------------------
-- Matrix cell
--------------------------------------------------------------------

MatrixCell = ButtonEx:subclass('MatrixCell')
MatrixCell:attributes{ 'row', 'col', 'matrix', 'fit' }
MatrixCell.canFocus = true         -- Cursor can focus on this control type
MatrixCell.margin = 0
MatrixCell.constrain = { 0.5, 4}
MatrixCell.style = 'button.cell'

function MatrixCell:__init(items)
    Control.__init(self, items)

    -- self.rows = 1
    if not self.elements then
        self.elements = LayoutElement {
            TextElement{ text=items[1], align='center', fit=items.fit,id='.text' }
        }
    end
    assert(self.matrix and isinstance(self.matrix, Matrix), 'MatrixCell requires associated Matrix')
    self.matrix:link(self)
end

function MatrixCell:title()
    return tostring( self.matrix.ncols * (self.row-1) + self.col)
end

function MatrixCell:onUpdate()
    if self.setting then
        local idx = self:index()
        self:setText(self.setting:text(idx))
        self:setGraphic(self.setting:graphic(idx))
    end
end

-------------------------------------------------------------------
MatrixCellEx = MatrixCell:subclass('MatrixCellEx')
function MatrixCellEx:__init(items)
    if not items.elements then
        items.elements = LayoutElement {
            rows = 1,
            GraphicElement{ w=0.45, constrain=1, id='.graphic' }, --, margin=0.2
            TextElement{ text='X', align='left', fit=items.fit, id='.text' },
        }
    end
    super(MatrixCellEx).__init(self, items)
end

--------------------------------------------------------------------
-- Matrix
--------------------------------------------------------------------
Matrix = Control:subclass('Matrix')
Matrix:attributes{ 'title', 'rows', 'columns', 'ncells', 'cellTitles', 'cellClass', 'cellIndex', 'cellFactory', 'cell', 'drag', 'drop', 'style', 'fit', 'settings' }
Matrix.canFocus = true         -- This type of control can receive focus (cursor)
Matrix.cellClass = MatrixCell

-- Constructor. Build Matrix from 'items' table
function Matrix:__init(items)
    Control.__init(self, items)
    self.menu = Menu(items.menu or { ' ', 'X' } )
    -- Derive 'nrows', rowtitles[] and colsPerRow[]
    if type(items.rows) == 'table' then
        self.nrows = #items.rows
        self.rowtitles = { }
        if type(items.rows[1]) == 'table' then
            self.colsPerRow = { }
            for _,row in ipairs(items.rows) do
                table.insert(self.rowtitles, row[1])
                table.insert(self.colsPerRow, row[2])
            end
        else
            self.rowtitles = items.rows
        end
    elseif type(items.rows) == 'number' then
        self.nrows = items.rows
        --self.rowtitles = items.rows
    end

    -- Derive 'ncols', coltitles
    if type(items.columns) == 'table' then
        self.ncols = #items.columns
        self.coltitles = items.columns
        self.coltitlesYOffset = items.columns.yoffset
    elseif type(items.columns) == 'number' then
        self.ncols = items.columns
    else
        assert(items.columns == nil, 'Unexpected column type')
    end

    if items.ncells then
        if self.ncols and self.nrows then
            assert(items.ncells <= self.ncols * self.nrows, "Unexpected (or redundant) ncells")
        elseif self.nrows then
            self.ncols = (items.ncells + self.nrows-1) // self.nrows
        elseif self.ncols then
            self.nrows = (items.ncells + self.ncols-1) // self.ncols
        else
            error("Unknown number of rows or columns")
        end
    end

    if not self.nrows and self.ncols then
        self.nrows = 1
    elseif not self.ncols and self.nrows then
        self.ncols = 1
    else
        assert(self.nrows, 'Unknown number of rows')
        assert(self.ncols, 'Unknown number of columns')
    end

    -- For development, make sure that the Matrix has a dummy Setting object
    -- from which it can get/set cell values

    if self.settings and type(self.settings) == 'string' then
        self.setting, self.settings = self.settings, nil
    end
    self.setting = SettingFind(items.setting)

--[[
    if not self.setting then
        self.setting = Setting{ matrix=true }
    end
    --]]

    -- Check for consistency
    assert(self.ncols > 0)
    assert(self.nrows > 0)
    assert((self.rowtitles == nil)  or (#self.rowtitles  == self.nrows))
    assert((self.colsPerRow == nil) or (#self.colsPerRow == self.nrows))
    assert((self.coltitles == nil)  or (#self.coltitles  == self.ncols))

    -- TODO: Improve following
    -- Store dimensions for layout
    self.columns, self.rows = nil, nil
    if self.ncols then self.columns = self.ncols end
    if self.nrows then self.rows = self.nrows end
end

-- Associate a control with this matrix
-- Typically control is a MatrixCell or similar
function Matrix:link(c)
    assert(c)
    assert(c.matrix == nil or c.matrix == self)
    c.matrix = self
    c.id = c.id or ((self.id or '') .. 'Cell')
    c.setting = c.setting or self.setting
    if self.cellIndex then c.index = self.cellIndex end
    c.delegate = c.delegate or self.delegate

    c.drag=c.drag or self.drag
    c.drop = c.drop or self.drop
    c.style=c.style or self.style
end

--------------------------------------------------------------------
CellPanel = Panel:subclass('CellPanel')
CellPanel:attributes{ 'row', 'col' } --, 'matrix', 'fit' }
CellPanel.style = 'cell.panel'
CellPanel.background = 0x707070
CellPanel.visual = true


--------------------------------------------------------------------
function matrixAsciiArt(s, idx, fn)
    local t = { }
    local i = 1
    for c in s:gmatch('.') do
        if not c:match('%p') then
            t[c] = t[c] and t[c] + 1 or 1
            if idx == i then return fn(c, t[c]) end
            i = i + 1
        elseif c == '-' then
            i = i + 1
        end
    end
end

--------------------------------------------------------------------

MatrixAsciiArt = Object:subclass('MatrixAsciiArt')

function MatrixAsciiArt:__init(s)
    self.s = s
end

function MatrixAsciiArt:artwork()
    return self.s
end

-- Call fn for the cell at specified index into the ascii art
function MatrixAsciiArt:cellAtIndex(idx, fn)
    return matrixAsciiArt(self.s, idx, fn)
end

function MatrixAsciiArt:dimensions()
    local rows = 1
    local columns = 0
    local col = 0
    for c in self.s:gmatch('.') do
        if c == '|' then
            rows = rows + 1
            col = 0
        else
            col = col + 1
            if col > columns then
                columns = col
            end
        end
    end
    return rows, columns
end

function MatrixAsciiArt:rows()
    local rows, _ = self:dimensions()
    return rows
end

function MatrixAsciiArt:columns()
    local _, cols = self:dimensions()
    return cols
end

--------------------------------------------------------------------

