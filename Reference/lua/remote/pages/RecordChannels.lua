local config = require 'config'

local function CellFactory(matrix, cell, row, col, idx)
    local c = MatrixCell {
            matrix=matrix,
            row=row,
            col=col,
            fit= matrix.fit, --{ 8, 12 }
            -- margin = { 0.2, 0.1 }
        }
    local l = Label {
            --tostring(idx),
            -- margin = { 0.05, 0.2, 0.05, 0.1 },
            align = 'center',
            id = 'MatrixCellLabel',
            elements = InputNameElement{ id='.text' },
        }
    l.cell = c
    l.delegate = matrix.delegate
    return CellPanel {
        columns = 1,
        padding = 0.05,
        l,
        c,
    }
end

local RecordChannels = Page {
    name = 'RecordChannels',
    title = 'Record Enables',
    columns = 1,
    delegate = true,
    depends = {
        'trackmix.preset', 'trackmix.matrix', 'trackmix.isomatrix',
        'input.name', 'input.color',
        'track.name', 'track.color'
    },
    constrain = { 1, 4 },
    Panel {
        rows=1,
        id='panel',
        h=0.15,
        Button { 'Two Track', id='TwoTrack', fit=15 },
        Button { 'Four Track', id='FourTrack', fit=15 },
        Button { 'Tracks Mixed To', id='TracksMixedTo', fit=15 },
        Button { 'All Tracks', id='AllTracks', fit=15 },
    },
    Panel {
        background = true,
        Matrix {
            id = 'Matrix',
            columns = config.ntracks > 12 and 8 or 4,
            ncells = config.ntracks,
            fit = 8,
            -- cellTitles = true,
            cellFactory = CellFactory,
            cellIndex = function(cell) return (cell.row-1) * cell.matrix.columns + cell.col end,
            setting = 'record.tracks',
        },
    },
        ------------------------------------------------
    _setTracks = function(self, n)
            local setting = self.ui.matrix.setting
            for i=1,32 do setting:setValue(i <=n and 1 or 0, { i }) end
        end,
    onClick_TwoTrack = function(self) self:_setTracks(2) end,
    onClick_FourTrack = function(self) self:_setTracks(4) end,
    onClick_AllTracks = function(self) self:_setTracks(32) end,
    onClick_TracksMixedTo = function(self) self:_setTracks(32) end,
}

function RecordChannels:onEnter_Page()
    if config.nomad then
        self.ui.panel:setVisible(false)
    end
end

function RecordChannels:onUpdate_MatrixCell(cell)
    local _, _, routed = TrackDescription(cell:index())
    local text, color, style --= 'Not routed', 'gray','button.flat'
    if true then -- or routed then
        style = 'button'
        if cell:value() == 1 then
            text, color = 'ARMED', 'red'
            if not routed then  text = 'ARMED n/r' end
        else
            text,color = 'DISARMED', 0x606060 --'darkgray'
            if not routed then text = 'Not routed' end
        end
    end
    cell.color = color
    cell.style = style
    cell:setText(text)
    --cell:setState(state)
    --cell:setEnabled(routed)

end

function RecordChannels:onUpdate_MatrixCellLabel(label)
    local idx = label.cell:index()
    local text, background = TrackDescription(idx)
    label:setText(text)
    label:setBackground(background)
    label:layout()
end

