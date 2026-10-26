-------------------------------------------------------------------

local _M = Page {
    title = 'Dante Router',
    delegate = true,
    Panel {
        id = 'MainPanel',
        background = true,
        columns = 1,
        Panel {
            background = true,
            Matrix {
                margin = { 0.1, 0.2 },
                id = 'Source',
                columns = 16,
                rows = { 'Analog 1-16', 'Digital 1-16' },
                setting = 'input.source',
                drop = 'dante',
                fit = 2,
                cellClass = MatrixCellEx,
                cellIndex = function(cell) return (cell.row-1)*cell.matrix.columns + cell.col end,
            },
        },
        Panel  {
            background = true,
            Matrix {
                id = 'Dante',
                columns = 16,
                rows = { 'Dante 1-16', 'Dante 17-32', 'Dante 33-48', 'Dante 49-64' },
                --cellTitles = true,
                drag = 'dante',
                fit = 2,
                cellClass = MatrixCellEx,
                cellIndex = function(cell) return (cell.row-1)*cell.matrix.columns + cell.col end
            },
        },
    },
}

-------------------------------------------------------------------

function _M:onCreatedPage()
    self._setting = SettingFind('input.source')
end

function _M:onUpdateDanteCell(cell)
    cell:setGraphic('dante')
    cell:setText(tostring(cell:index()))
end

function _M:onDropSourceCell(cell, _srcWidget)
    local dante = _srcWidget:owner()
    local danteChan = dante:index()
    self._setting:setValue(danteChan, cell:index())
end

function _M:onClickSourceCell(cell)
    self._setting:setValue(nil, cell:index())
    return true
end

