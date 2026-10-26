local config = require 'config'

assert(config.nomad or config.nova, 'Only for config.nomad or nova')

local asciiArt = { }

if config.nova then
    -- Nova only supports 6 of the available 8 outputs
    asciiArt.TapeMix = MatrixAsciiArt('TTTTTTTT|TTTTTTTT|TTTT--RR|OOOOOO--')
    asciiArt.HphMix  = MatrixAsciiArt('TTTTTTTT|TTTTTTTT|OOOO--RR')     --|ZZZZZZZZ' was  TTTT--RR|OOOOOO--')
    asciiArt.Hph2Mix = MatrixAsciiArt('TTTTTTTT|TTTTTTTT|TTTT--RR|OOOOOO--')
    asciiArt.MonoMix = MatrixAsciiArt('TTTTTTTT|TTTTTTTT|TTTT--RR|OOOOOO--')
else
    asciiArt.TapeMix = MatrixAsciiArt('TTTTTT|TTTTTT|OOOOOO|RRRR--')
    asciiArt.HphMix  = MatrixAsciiArt('TTTTTT|TTTTTT|OOOOOO|RRRR--|ZZZZZZ')
    asciiArt.Hph2Mix = MatrixAsciiArt('TTTTTT|TTTTTT|OOOOOO|RRRR--')
    asciiArt.MonoMix = MatrixAsciiArt('TTTTTT|TTTTTT|OOOOOO|RRRR--')
end

local function mixCellFactory(settingbase, s, matrix, cell, row, col, idx)
    return matrixAsciiArt(s, idx, function(s, i)
        local text = ({T='TRACK',O='OUT',R='RET',Z='ZNET'})[s]
        local color = ({T='plum',O='outputmix',R=nil,Z=nil})[s]
        local setting = settingbase .. '.' .. ({T='tracks',O='outputs',R='returns',Z='znet.select'})[s]
        return CellPanel {
            columns = 1, margin = 0.03,
            padding = 0.02,
            tag = i,
            Label {
                text .. ' ' .. i, -- idx <= 8 and ('OUT ' .. idx) or ('RET ' .. idx-8),
                h = 0.4,
                margin = { 0.2, 0 },
                -- margin = { 0.05, 0.2, 0.05, 0.1 },
                align = 'center',
                fit = 'TRACK 12',
            },
            MatrixCell {
                matrix=matrix, row=row, col=col,
                background=color,
                color='white',
                tag = i,
                setting=setting,
                id = (s == 'Z') and 'ZaxnetCell' or nil,
            }
        }
    end) or Panel{}
end

local function clickPhaseCell(cell, phase)
    local value = cell:value()
    local origValue = value
    local menu = cell.setting:menu(cell:index())
    local text = menu:textForValue(value)

    if #text > 0 and phase ~= (text:find('/') ~= nil) then
        if phase then text = text:gsub('(%w)', '/%1')
        else text = text:gsub('/', '')
        end
        if menu:valueForText(text) ~= value then
            cell:setValue( menu:valueForText(text) )
        end
        return
    end

    while true do
        value = menu:incValue(value)
        text = menu:textForValue(value)
        if value == 0 or #text == 0 or phase == (text:find('/') ~= nil) then
            if value == origValue then cell:incValue() -- ... for parameter without phase options do normal increment!
            else cell:setValue(value)
            end
            return
        end
    end
end

local function paintPhaseCell(cell, painter)
    local s = cell:text()
    local r = painter:rect():adjusted(0.15, 0.25, -0.15, -0.05)
    if #s > 0 then
        -- Extract fields e.g 'L/R' becomes { 'L', '/R' }
        local t = { }
        local phase = false
        for c in s:gmatch('.') do
            if c == '/' then phase = true
            elseif c:match('%w+') then
                table.insert(t, (phase and '/' or '') .. c)
                phase = false
            end
        end
        -- Draw fields
        r:adjust(0, 0, -(#t-1)/#t, 0)
        for i,text in ipairs(t) do
            --painter:fillRect(r, 'red')
            local phase = text:sub(1,1) == '/'
            if phase then text = text:sub(2) end
--          painter:fill{ rect=r, color='red' }
            painter:fitFont(2, r.w, r.h)
            painter:text{text, rect=r, align='center'}
            if phase then
                local n = #text
                local _,hbar = painter:textBounds(string.rep('X', n)) --text)
                painter:text{ string.rep('_',  n), rect=r:translated(0, -hbar), align='center' }
            end
            r.x = r.x + r.w
        end
    end
end

-------------------------------------------------------------------

local HeadphoneMix = Page {
    title = "Headphone Mix",
    delegate = true,
    depends = { 'hpmix.preset' },
    Panel {
        id = 'MainPanel',
        rows = 1,
        Panel {
            w=0.2,
            columns = 1,
            id = 'ButtonPanel',
            IncDecButton { "PRESET", setting='hpmix.preset', names='hpmix.names' },
            Button { "ENABLE", setting='hpmix.enable', index='hpmix.preset' },
            Spacer { },
            Button { "PHASE", setting='hpmix.phase' },
--          Button { "CLEAR|ALL", id='Clear' },
        },
        Panel {
            rows = 1,
            background = true,
            Matrix {
                id = 'Matrix',
                columns = asciiArt.HphMix:columns(),
                rows = asciiArt.HphMix:rows(),
                cellFactory = function (matrix, cell, row, col, idx)
                    return mixCellFactory('hpmix', asciiArt.HphMix:artwork(), matrix, cell, row, col, idx)
                end,
            },
        },
    },
}

function HeadphoneMix:onEnter_Page(cell)
    SettingNotify('hpmix.preset', self.onChangedPreset, self)
    self:onChangedPreset()
end

function HeadphoneMix:onChangedPreset()
    -- Nomad HP Preset 11,12 only have tracks
    local preset = settings.hpmix.preset
    local tracksOnly = (preset == 11) or (preset == 12)
    for i,cell in ipairs(self.ui.matrix:findByClass(CellPanel)) do
        cell:setVisible(not tracksOnly or i <= 10)
    end
    self:layout()
    self:update()
end

function HeadphoneMix:onClick_MatrixCell(cell)
    clickPhaseCell(cell, (settings.hpmix.phase ~= 0))
end

function HeadphoneMix:onPaint_MatrixCell(cell, painter)
    paintPhaseCell(cell, painter)
end

HeadphoneMix.onPaint_ZaxnetCell = HeadphoneMix.onPaint_MatrixCell

function HeadphoneMix:onIndex_MatrixCell(cell)
    return { settings.hpmix.preset, cell.tag }
end

function HeadphoneMix:onIndex_ZaxnetCell(cell)
    return { settings.hpmix.preset }
end

function HeadphoneMix:onUpdate_ZaxnetCell(cell)
    cell:setText( cell:value()+1 == cell.tag and 'LR' or '')
end

function HeadphoneMix:onClick_ZaxnetCell(cell)
    cell:setValue( cell:value() == cell.tag-1 and -1 or cell.tag-1)
end

-------------------------------------------------------------------

local Hph2Mix = Page {
    title = "Hph2 Mix",
    delegate = true,
    Panel {
        id = 'MainPanel',
        rows = 1,
        Panel {
            w=0.2,
            columns = 1,
            id = 'ButtonPanel',
            Panel {
                columns = 1,
                Label { 'Hph 2', align='center', font=0.8 },
                Label { 'Mix',  align='center', font=0.8 },
            },
            Spacer { },
            Spacer { },
            Button { "PHASE", setting='hpmix2.phase' },
        },
        Panel {
            rows = 1,
            background = true,
            Matrix {
                id = 'Matrix',
                columns = asciiArt.Hph2Mix:columns(),
                rows = asciiArt.Hph2Mix:rows(),
                cellFactory = function (matrix, cell, row, col, idx)
                    return mixCellFactory('hpmix2', asciiArt.Hph2Mix:artwork(), matrix, cell, row, col, idx)
                end,
            },
        },
    },

    onClick_MatrixCell = function(self, cell)
        clickPhaseCell(cell, (settings.hpmix2.phase ~= 0))
    end,

    onPaint_MatrixCell = function(self, cell, painter)
        paintPhaseCell(cell, painter)
    end,

    onIndex_MatrixCell = function(self, cell)
        return { cell.tag }
    end

}


-------------------------------------------------------------------

local TapeMix = Page {
    title = "Tape Mix",
    delegate = true,
    Panel {
        id = 'MainPanel',
        rows = 1,
        Panel {
            w=0.2,
            columns = 1,
            id = 'ButtonPanel',
            Panel {
                columns = 1,
                Label { 'Tape/IFB', align='center', font=0.8 },
                Label { 'Mix',  align='center', font=0.8 },
            },
            Spacer { },
            Spacer { },
            Button { "PHASE", setting='tapemix.phase', id='phase'},
        },
        Panel {
            margin = 0,
            columns = 1,
            Panel {
                rows = 1,
                background = true,
                Matrix {
                    id = 'Matrix',
                    columns = asciiArt.TapeMix:columns(),
                    rows = asciiArt.TapeMix:rows(),
                    cellFactory = function (matrix, cell, row, col, idx)
                        return mixCellFactory('tapemix', asciiArt.TapeMix:artwork(), matrix, cell, row, col, idx)
                    end,
                },
            },
        },
    },

    onIndex_MatrixCell = function(self, cell)
        return { cell.tag }
    end,

    onClick_MatrixCell = function(self, cell)
        clickPhaseCell(cell, self.ui.phase:value() ~= 0)
    end,

    onPaint_MatrixCell = function(self, cell, painter)
        paintPhaseCell(cell, painter)
    end,
}

-------------------------------------------------------------------

local MonoMix = Page {
    title = "Mono Mix",
    delegate = true,
    Panel {
        id = 'MainPanel',
        rows = 1,
        Panel {
            w=0.2,
            columns = 1,
            id = 'ButtonPanel',
            Panel {
                columns = 1,
                Label { 'Mono', align='center', font=0.8 },
                Label { 'Mix',  align='center', font=0.8 },
            },
            Spacer { },
            Spacer { },
            Button { "PHASE", setting='monomix.phase', id='phase'},
        },
        Panel {
            margin = 0,
            columns = 1,
            Panel {
                rows = 1,
                background = true,
                Matrix {
                    id = 'Matrix',
                    columns = asciiArt.MonoMix:columns(),
                    rows = asciiArt.MonoMix:rows(),
                    cellFactory = function (matrix, cell, row, col, idx)
                        return mixCellFactory('monomix', asciiArt.MonoMix:artwork(), matrix, cell, row, col, idx)
                    end,
                },
            },
        },
    },

    onIndex_MatrixCell = function(self, cell)
        return { cell.tag }
    end,

    onClick_MatrixCell = function(self, cell)
        clickPhaseCell(cell, self.ui.phase:value() ~= 0)
    end,

    onPaint_MatrixCell = function(self, cell, painter)
        paintPhaseCell(cell, painter)
    end,
}

