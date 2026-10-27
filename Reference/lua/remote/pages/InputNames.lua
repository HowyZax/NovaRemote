-- Input Names and Track Names (work in progress)

local config = require 'config'
local sources = require 'remote.sources'

local function PresetColor(idx)
    local presets = {
        0xffff00,  -- bright yellow
        0x33ff00,  -- bright green
        0x33ffff,  -- bright blue
        0xff33ff,  -- bright pink
        0xff9900,  -- bright orange
        0xfbc1c1,  -- red
        0xffd8bf,  -- orange
        0xfff9b8,  -- yellow
        0xc0efbd,  -- green
        0xb7dffd,  -- blue
        0xd0bcec,  -- purple
        0xd3d3d3,  -- gray
    }
    return presets[idx] or 0xfffffff
end

local NameMatrixCell = MatrixCellEx:subclass('NameMatrixCell')

function NameMatrixCell:__init(items)
    if not items.elements then
        items.elements = LayoutElement {
                rows = 1,
                --GraphicElement{ w=0.3, constrain=1, id='.graphic' },
                InputNameElement{ id='.text' }, -- items.fit or self.fit
            }
    end
    super(SourceMatrixCell).__init(self, items)
end


local function NameCellEx(matrix, cell, row, col, idx)

    local function paintInputName(cell, painter, idx)
        local chan, graphic = sources.inputDescriptionWithGraphic(idx)
        if graphic then
            painter:graphic{ graphic, w=0.3 }
            painter:text{ tostring(chan), x=0.3, align='left', fit=2 }
        else
            painter:text{ tostring(chan), align='left', fit=4 } -- eg. RET1
        end
    end

    local function paintTrackName(cell, painter, idx)
        local chan = 'TRACK ' .. idx
        painter:text{ tostring(chan), align='center', fit='TRACK 12', font=0.8 } -- eg. RET1
    end

    return CellPanel {
        columns = 1,
        padding = 0.02,
        Label {
            h = 0.3,
            onPaint = function(self, painter)
                if matrix.setting.name:match('input%..*') then
                    paintInputName(self, painter, idx)
                else
                    paintTrackName(self, painter, idx)
                end
                return true
            end
        },
        NameMatrixCell {
            matrix=matrix,
            row=row,
            col=col,
            fit=matrix.fit, --{ 8, 10 }
        }
    }
end

local function NameCell(matrix, cell, row, col, idx)
    return NameMatrixCell {
        matrix=matrix,
        row=row,
        col=col,
        fit=matrix.fit, --{ 8, 10 }
    }
end

local _M
if not config.nomad then
    _M = Page {
        title = 'Input Names',
        delegate = true,
        depends = { 'input.color', 'input.source', 'names.character.name', 'names.character.color', 'input.warning' },
        Panel {
            id = 'MainPanel',
            rows = 1,
            Panel {
                background = true,
                Matrix {
                    -- margin = 0.05,
                    rows = 4,
                    columns = 8,
                    id = 'Names',
                    setting = 'input.name',
                    drag = '@name',
                    drop = '@name',
                    cellFactory = NameCellEx,
                    fit = { 4, 10 },
                },
            },
            Panel {
                columns = 1,
                w = 0.3,
                Panel {
                    background = true,
                    Label { 'PRESET TRACK NAMES', h=0.1, align='center', font=0.8, fit=24 },
                    Matrix {
                        rows = 3,
                        columns = 5,
                        id = 'Presets',
                        drag = '@name',
                        drop = '@name',
                        setting = 'names.preset.name',
                        cellFactory = NameCell,
                        fit = { 4, 10 },
                    },
                    Label { 'SCRIPT CHARACTER NAMES', h=0.1, align='center', font=0.8, fit=24 },
                    Matrix {
                        rows = 3,
                        columns = 5,
                        id = 'Characters',
                        drag = '@name',
                        drop = '@name',
                        setting = 'names.character.name',
                        cellFactory = NameCell,
                        fit = { 4, 10 },
                    },
                },

                Panel {
                    rows = 1,
                    h = 0.25,
                    background = true,
                    Label { 'COLORS', align='center', font=0.8, fit=true },
                    Matrix {
                        w=0.75,
                        rows = 4,
                        columns = 3,
                        id = 'Colors',
                        drag = '@color',
                        cellIndex = function(cell) return (cell.row-1) * cell.matrix.columns + cell.col end,
        --              setting = 'names.characters',
        --              fit = { 8, 10 },
                    },
                },
            },
        },
    }
else    -- config.nomad or config.nova
    _M = Page {
--      title = 'Input Names',
        title = 'Track Names',
        delegate = true,
        depends = {
            'track.name', 'track.color',
            'input.color', 'input.source',
            'names.character.name', 'names.character.color' },
        Panel {
            id = 'MainPanel',
            columns = 1,
            Panel {
                background = true,
                Matrix {
                    columns = config.ntracks > 12 and 8 or 4,
                    ncells = config.ntracks,
                    id = 'Names',
--                  setting = 'input.name',
                    setting = 'track.name',
                    drag = '@name',
                    drop = '@name',
                    cellFactory = NameCellEx,
                    fit = { 4, 10 },
                },
            },
            Panel {
                rows = 1,
                h = 0.3,
                Panel {
                    columns = 1,
                    background = true,
                    Label { 'PRESET TRACK NAMES', h=0.25, align='center', font=0.8, fit=24, margin={0.02, 0} },
                    Matrix {
                        rows = 4, -- was 3,
                        columns = 6,
                        id = 'Presets',
                        drag = '@name',
                        drop = '@name',
                        setting = 'names.preset.name',
                        cellFactory = NameCell,
                        fit = { 4, 10 },
                    },
                },
                Panel {
                    columns = 1,
                    w = 0.4,
                    background = true,
                    Label { 'COLORS', h=0.25, align='center', font=0.8, fit=true },
                    Matrix {
                        rows = 3,
                        columns = 4,
                        id = 'Colors',
                        drag = '@color',
                        cellIndex = function(cell) return (cell.row-1) * cell.matrix.columns + cell.col end,
                    },
                    Button {
                        h = 0.2,
                        drag = '@color',
                        onDropValue = function() return 0 end,
                    }
                },
            },
        },
    }
end

-------------------------------------------------------------------
function _M:_onIndexCell(cell)
    return (cell.row-1)*cell.matrix.columns + cell.col
end
_M.onIndexCharactersCell    = _M._onIndexCell
_M.onIndexPresetsCell       = _M._onIndexCell
_M.onIndexNamesCell         = _M._onIndexCell

-------------------------------------------------------------------

-- Get color setting associated with cell
-- e.g. if cell setting is 'input.name' then associated color settings
-- us 'input.color'
function _M:_colorSetting(cell)
    return SettingFind(cell.setting.name:gsub('.name', '.color'))
end

-- Get text, color & style for cell
function _M:_cellInfo(cell)
    local idx = cell:index()
    local color = self:_colorSetting(cell):value(idx)
    return cell.setting:value(idx), color, color
end

-------------------------------------------------------------------
function _M:_onUpdateCell(cell)
    local text, color = self:_cellInfo(cell)
--  print('_onUpdateCell', cell.__classname, text, color)
    cell:setText(text)
    cell:setBackground(color)
    cell:layout()
end

_M.onUpdateCharactersCell    = _M._onUpdateCell
_M.onUpdatePresetsCell       = _M._onUpdateCell
_M.onUpdateNamesCell = function(self, cell)
    if cell.setting.name:match('^input%..*') then
        cell.style = SettingFind('input.warning'):value(cell:index()) > 0 and 'button.warning' or 'button.cell'
    end
    self:_onUpdateCell(cell)
end


-------------------------------------------------------------------
function _M:_onClickCell(cell)
    cell.setting:edit(cell, cell)
end

_M.onClickCharactersCell    = _M._onClickCell
_M.onClickPresetsCell       = _M._onClickCell
_M.onClickNamesCell         = _M._onClickCell

-------------------------------------------------------------------
function _M:_onDropCell(cell, _srcWidget)
    local source = _srcWidget:owner()
    assert(source.drag ~= nil)
    if source.drag == '@color' then
        self:_colorSetting(cell):setValue( source:dropValue(), cell:index())
    elseif source.drag == '@name' then
        local text, color = self:_cellInfo(source)
        cell:setValue( source:value() )
        self:_colorSetting(cell):setValue(color, cell:index())
    end
    cell:update()
end

_M.onDropNamesCell          = _M._onDropCell
_M.onDropCharactersCell     = _M._onDropCell
_M.onDropPresetsCell        = _M._onDropCell

-------------------------------------------------------------------
function _M:onDropValueColorsCell(cell)
    return PresetColor(cell:index())
end

function _M:onUpdateColorsCell(cell)
    cell:setBackground(PresetColor(cell:index()))
end


