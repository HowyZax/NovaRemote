local config = require 'config'
local sources = require 'remote.sources'

local _M

if not config.nomad then _M = Page {
    name = 'FaderAssign',
--  titleAlign = 'left',
    delegate = true,
--  Spacer { h = 0.1 },
    depends = {
        'mixer.bank',
        'input.color',
        'input.name',
        'input.source',
        'input.warning',
    },
    Panel {
        rows = 1,
        Panel {
            columns = 1,
            background = true,
            Panel {
                rows = 1,
                --margin = 0,
                Panel {
                    columns = 1,
                    background = 'salmon',
                    style = 'panel.gradient',
                    id = 'MatrixPanel',
                    margin = { 0,0.03,0,0.03 },
                    Matrix {
                        columns = enumerate(1, 16, tostring ), --,{ '1', '2', '3', '4', '5', '6', '7', '8', '9', '10' },
                        rows = 5, --enumerate(1, 5, tostring),
                        margin = 0,
                        setting = 'mixer.source',
                        id = 'Matrix',
                        drop = 'sources',
                        cellIndex = function(cell) return {cell.row, cell.col} end,
                        cellFactory = function(matrix, cell, row, col, idx)
                                return CellPanel {
                                    row = row, col = col,
                                    margin = 0.01, --{ 0.01, 0.1, 0.01, 0.01 },
                                    padding = 0.05, --{ 0.05, 0.2, 0.05, 0.05 },
                                    SourceMatrixCell{ matrix=matrix, row=row, col=col }
                                }
                            end,
                    },
                },
                Panel {
                    margin = 0,
                    w = 0.1,
                    columns = 1,
                    Spacer { h=0.17, },
                    CheckedGroup {
                        setting = 'mixer.bank',
                        fit = true,
                        columns = { 0.3, 0.7 },
                        --color = 'salmon',
                    },
                }
            },
            MatrixSources { },
        },
    },
}
else

local nfaders = 8
local nbanks = 3

_M = Page {
    name = 'FaderAssign',
--  titleAlign = 'left',
    delegate = true,
--  Spacer { h = 0.1 },
    depends = {
        'mixer.bank',
        'input.color',
        'input.name',
        'input.source',
        'input.warning',
    },
    Panel {
        rows = 1,
        Panel {
            columns = 1,
            background = true,
            Panel {
                rows = 1,
                --margin = 0,
                Panel {
                    columns = 1,
                    background = 'salmon',
                    style = 'panel.gradient',
                    id = 'MatrixPanel',
                    margin = { 0,0.03,0,0.03 },
                    Matrix {
                        columns = enumerate(1, nfaders, tostring ), --,{ '1', '2', '3', '4', '5', '6', '7', '8', '9', '10' },
                        rows = nbanks, --enumerate(1, 5, tostring),
                        margin = 0,
                        setting = 'mixer.source',
                        id = 'Matrix',
                        drop = 'sources',
                        cellIndex = function(cell) return {cell.row, cell.col} end,
                        cellFactory = function(matrix, cell, row, col, idx)
                                return CellPanel {
                                    row = row, col = col,
                                    margin = 0.01, --{ 0.01, 0.1, 0.01, 0.01 },
                                    padding = 0.05, --{ 0.05, 0.2, 0.05, 0.05 },
                                    SourceMatrixCell{ matrix=matrix, row=row, col=col }
                                }
                            end,
                    },
                },
                Panel {
                    margin = 0,
                    w = 0.1,
                    columns = 1,
                    Spacer { h=0.17, },
                    CheckedGroup {
                        setting = 'mixer.bank',
                        fit = true,
                        columns = { 0.3, 0.7 },
                        --color = 'salmon',
                    },
                }
            },
            MatrixSources { },
        },
    },
}
end
-------------------------------------------------------------------

function _M:onCreatedPage()
    self._settings = settings['mixer']
    self._setting = SettingFind('mixer.source')
    for _,c in ipairs(self.ui.matrixpanel:findByClass(Label)) do
        c:setFont(0.5)
    end
end

function _M:onEnterPage()
    self:onChangedMixerBank()
end

function _M:onChangedMixerBank()
    local bank = settings.mixer.bank+1
    local matrix = self.ui.matrix
    for _,cell in pairs(matrix:findByClass(CellPanel)) do
        local bank = settings.mixer.bank+1
        cell.background = cell.row == bank and 'white' or nil
        cell:update()
    end
end

function _M:onUpdateMatrixCell(cell)
    local idx = cell:index()

    local options = {
        background=(cell.row == (settings.mixer.bank+1)) and  'faderassign' or nil,
    }

    -- Get the fader mode (0=default, 1=trim, 2=zaxnet)
    local mode = SettingFind('faders.mode'):value(idx)
    if mode == 2 then       -- Zaxnet control
        local znet = SettingFind('faders.zaxnet.id'):value(idx)
        options.usertext = znet == 0 and 'Z--' or ('Z' .. znet)
        options.background = nil
    else
        if mode == 1 then options.prefix = 'T' end -- Trim control
        options.input = SettingFind('mixer.source'):value(idx)
    end

    cell:setContent(options)
end

function _M:onClickMatrixCell(cell)
    if cell:value() ~= 0 then
        cell:setValue(0)
    end
end

function _M:onClickClear()
    MsgBox.Show(
        self,
        "Clear bank " .. settings.mixer.bank + 1 .. "?",
        { "Ok", function() self._setting:clear( settings.mixer.bank+1) end } ,
        { "Cancel" } )
end

function _M:onDropMatrixCell(cell, _sourceWidget)
    local source = _sourceWidget:owner()
    local which = source:index()
--  print ('_M:onDragDropMatrixCell', source, source.__classname, source, source.__classname, which)
    local index = cell:index()
    for _,c in ipairs(cell.matrix:findByClass(MatrixCell)) do
        if c.row == cell.row and c:value() == which then
            c:setValue(0)
        end
    end
    cell:setValue(which)
end


