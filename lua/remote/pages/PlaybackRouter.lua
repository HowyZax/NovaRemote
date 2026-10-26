local function sourceText(idx)
    if idx <= 8 then return 'MIX ' .. tostring(idx)
    elseif idx <=32 then  return 'ISO ' .. tostring(idx)
    else
        local t = { 'HPH L', 'HPH R', 'HPH L/R', 'MUTE' }
        return t[idx-32] or ''
    end
end

local function sourceInfo(idx)
    if idx <= 32 then
        local text, color = TrackDescription(idx)
        -- if color then color = 'input.color.' .. tostring(color) end
        if not text then text = sourceText(idx) end
        return text,color
    else
        return sourceText(idx)
    end
end

local function SourceCellFactory(matrix, cell, row, col, idx)
    local text = sourceText(idx)
    if #text == 0 then return Spacer { } end

    return CellPanel {
        columns = 1,
        padding = 0.02,
        Label {
            text,
            h = 0.3,
            margin = { 0.2, 0 },
            align = 'center',
            fit = {5, 8},
        },
        SourceMatrixCell {
            matrix=matrix,
            row=row,
            col=col,
            fit= matrix.fit, --{ 8, 12 }
        }
    }
end

local function OutputCellFactory(matrix, cell, row, col, idx)
    return CellPanel {
        columns = 1,
        margin = { 0.01, 0.2, 0.01, 0.01 },
        padding = 0.02,
        Label {
            'OUT' .. tostring(idx),
            h = 0.3,
            margin = { 0.2, 0 },
            align = 'center',
            fit = 4,
        },
        SourceMatrixCell {
            matrix=matrix,
            row=row,
            col=col,
            fit= matrix.fit, --{ 8, 12 }
        }
    }
end

local _M = Page {
    title = 'Playback Router',
    delegate = true,
    depends = {
        'input.name', 'input.color', 'trackmix.preset', 'trackmix.matrix', 'trackmix.isomatrix'
    },
    Panel {
        id = 'MainPanel',
        rows = 1,
        Panel {
            background = true,
            Matrix {
                id = 'Source',
                rows = 5,
                columns = 8,
                fit = { 7, 12},
                --cellTitles = function(self, row, col, idx) return sourceText(idx) end,
                cellFactory = SourceCellFactory,
                cellIndex = function(cell) return (cell.row-1) * cell.matrix.columns + cell.col end,
                drag = 'playback.source',
            },
        },
        Panel  {
            w = 0.3,
            Panel {
                background = true,
                Matrix {
                    margin = { 0.05, 0.1 },
                    id = 'Output',
                    columns = 4,
                    rows = 2,
                    fit = { 7, 12},
                    -- cellTitles = function(self, row, col, idx) return 'OUT' .. tostring(idx) end,
                    drop = 'playback.source',
                    style = 'output',
                    setting = 'output.source',
                    cellFactory = OutputCellFactory,
                    cellIndex = function(cell) return (cell.row-1)*cell.matrix.columns + cell.col end,
                },
            },
        },
    },
}

function _M:onUpdateSourceCell(cell)
    local text, color = sourceInfo(cell:index())
    cell:setContent{
        usertext = text,
        background = color,
    }
end

function _M:onUpdateOutputCell(cell)
    local value = cell:value()
    local text, color
    if value then text, color = sourceInfo(cell:value()) end
    cell:setContent{
        usertext = text,
        background = color,
    }
end

function _M:onDropOutputCell(cell, _srcWidget)
    local source = _srcWidget:owner()
    cell:setValue( source:index() )
end

function _M:onClickOutputCell(cell)
    cell:setValue(nil)
end
