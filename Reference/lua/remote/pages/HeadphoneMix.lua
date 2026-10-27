local HeadphoneMix = Page {
    title = "Headphone Mix",
    delegate = true,
    depends = { 'hpmix.preset' },
    Panel {
        id = 'MainPanel',
        rows = 1,
        Panel {
            w=0.1,
            columns = 1,
            id = 'ButtonPanel',
            IncDecButton { "PRESET", setting='hpmix.preset', names='hpmix.names' },
            Spacer { },
            Spacer { },
            Button { "PHASE", setting='hpmix.phase' },
            Button { "CLEAR|ALL", id='Clear' },
        },
        Panel {
            rows = 1,
            background = true,
            Matrix {
                id = 'Matrix',
                columns = 8,
                rows = 4,
                setting = 'hpmix.matrix',
                cellIndex = function(cell) return { settings.hpmix.preset, (cell.row-1)*cell.matrix.columns + cell.col } end,
                cellFactory = function (matrix, cell, row, col, idx)
                    return CellPanel {
                        columns = 1, margin = 0.03,
                        padding = 0.02,
                        Label {
                            (idx <= 8 and 'MIX ' or 'ISO ') .. idx,
                            h = 0.4,
                            margin = { 0.2, 0 },
                            -- margin = { 0.05, 0.2, 0.05, 0.1 },
                            align = 'center',
                            fit = 'ISO 32',
                        },
                        MatrixCell {
                            matrix=matrix, row=row, col=col,
                            fit = 2,
                            background = 'plum', --0xEB86EB, -- 'plum',
                            color = 'white',
                            -- id = matrix.id .. 'Cell',
                            -- setting=matrix.setting,
                            -- delegate = matrix.delegate,
                            -- margin = 0,
                        }
                    }
                end,
            },
            Matrix {
                id = 'Output',
                w = 0.3,
                columns = 4,
                rows = 4,
                setting = 'hpmix.matrix',
                cellIndex = function(cell) return { settings.hpmix.preset, 32 + (cell.row-1)*cell.matrix.columns + cell.col } end,
                -- cellTitles = function(self, row, col, idx)
                --         if idx <= 8 then return 'OUT' .. tostring(idx)
                --         else return 'RET' .. tostring(idx-8) end
                --     end,
                cellFactory = function (matrix, cell, row, col, idx)
                    if idx > 12 then return Panel { } end
                    return CellPanel {
                        columns = 1, margin = 0.03,
                        padding = 0.02,
                        Label {
                            idx <= 8 and ('OUT ' .. idx) or ('RET ' .. idx-8),
                            h = 0.4,
                            margin = { 0.2, 0 },
                            -- margin = { 0.05, 0.2, 0.05, 0.1 },
                            align = 'center',
                            fit = 'OUT 8',
                        },
                        MatrixCell {
                            matrix=matrix, row=row, col=col,
                            background='limegreen',
                            -- id = matrix.id .. 'Cell',
                            -- setting=matrix.setting,
                            -- delegate = matrix.delegate,
                            -- margin = 0,
                        }
                    }
                end,

            },
        },
    },
}

function HeadphoneMix:onClickMatrixCell(cell)
    local tbl
    if settings.hpmix.phase == 0 then
        tbl = { 1, 2, 3, 0, 1, 2, 3 }
    else
        tbl = { 4, 4, 5, 6, 5, 6, 0 }
    end
    cell:setValue( tbl[ cell:value()+1 ] )
end
HeadphoneMix.onClickOutputCell = HeadphoneMix.onClickMatrixCell

function HeadphoneMix:onPaintMatrixCell(cell, painter)
    local text = cell:text()
    local r = painter:rect():adjusted(0, 0.25, 0, -0.05)
    if #text > 0 then
        --painter:fillRect(r, 'red')
        local phase = text:sub(1,1) == '/'
        if phase then text = text:sub(2) end
        painter:fitFont(2, r.w, r.h)
        painter:text{text, rect=r, align='center'}
        if phase then
            local n = #text
            local _,hbar = painter:textBounds(string.rep('X', n)) --text)
            r:translate(0, -hbar)
            painter:text{ string.rep('_',  n), rect=r,  align='center' }
        end
    end
end
HeadphoneMix.onPaintOutputCell = HeadphoneMix.onPaintMatrixCell

function HeadphoneMix:_clear()
    local setting = SettingFind('hpmix.matrix')
    setting:clear(settings.hpmix.preset)
end

function HeadphoneMix:onClickClear()
    MsgBox.Show(
        self,
        "Clear the current preset?",
        { "Ok", function() self:_clear() end } ,
        { "Cancel" } )
end

