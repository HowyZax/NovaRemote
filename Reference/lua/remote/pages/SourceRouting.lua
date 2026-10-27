local config = require 'config'
local chanflags = require 'remote.chanflags'

local _M = Page {
    name = 'SourceRouting',
    title = 'Routing',
    delegate = true,
    depends = {
        'mixer.bank',
        'input.color',
        'input.name',
        'input.source',
        'input.channel',
        'trackmix.matrix',
        'trackmix.preset',
        'outputmix.matrix',
        'outputmix.preset',
    },
    constrain = { 1, 4 },
    Panel {
        rows = 1,
        Panel {
            w=0.2,
            columns = 1,
            id = 'ButtonPanel',
            IncDecButton { 'Input', fit='Analog 16', setting='input.channel', id='Channel' },
            Spacer { },
            Button { "FADER", setting='sourceroute.fader', id='Fader' },
            Button { "PHASE", setting='sourceroute.phase', id='Phase' },
        },
        Panel {
            margin = 0.02,
            style = 'panel.gradient',
            background = 'outputmix',
            Label { h=0.1  , 'Outputs', align='center' },
            w = 0.28,
            Matrix {
                rows = 6,
                columns = 1,
                id = 'OutputsMatrix',
                setting='outputmix.matrix',
                cellFactory = function(matrix, cell, row, col, idx)
                    if col == 2 and row > 4 then return Panel { } end
                    local tag = (col-1) * matrix.rows + row
                    return MixMatrixCell {
                        OutputName(idx),
                        fit=7,
                        matrix=matrix, row=row, col=col,
                        margin = 0.04,
                        tag=tag, setting=matrix.setting,
                        onIndex = function(self)
                            return { settings.outputmix.preset, self.tag, settings.input.channel }
                        end,
                    }
                end,
            }
        },
        Panel {
            margin = 0.02,
            style = 'panel.gradient',
            background = 'trackmix',
            Label { h=0.1, 'Tracks', align='center' },
            Matrix {
                rows = 6,
                columns = 2,
                id = 'TracksMatrix',
                setting = 'trackmix.matrix',
                cellFactory = function(matrix, cell, row, col, idx)
                    local tag = (col-1) * matrix.rows + row
                    return MixMatrixCell {
                        'TRK ' .. tostring(idx),
                        fit=7,
                        matrix=matrix, row=row, col=col,
                        margin = 0.04,
                        tag=tag, setting=matrix.setting,
                        onIndex = function(self)
                            return { settings.trackmix.preset, self.tag, settings.input.channel }
                        end,
                    }
                end,
            }
        }
    },
}

function _M:onUpdate_OutputsMatrixCell(cell)
    local text = OutputName(cell.tag)
    local cf = chanflags.unpack( cell:value() )
    cell:setContent{
        text = text,
        fader = cf.assigned and not cf.fader,
        phase = cf.assigned and cf.phase,
        background = cf.assigned and 'black' or 'outputmix',
        color = cf.assigned and 'white' or 'darkgreen',
        fit = 7,
--      font = flags and 1.0 or 0.7,
        small = not cf.assigned,
    }
end

function _M:onUpdate_TracksMatrixCell(cell)
    local text = TrackName(cell.tag)
    local cf = chanflags.unpack( cell:value() ) -- sources ~= nil and sources[input]
    cell:setContent{
        text = text,
        fader = cf.assigned and not cf.fader,
        phase = cf.assigned and cf.phase,
        background = cf.assigned and 'black' or 'trackmix',
        color = cf.assigned and 'white' or 'darkblue',
        fit = 7,
        --font = flags and 1.0 or 0.7,
        small = not cf.assigned,
    }
end

function _M:chanflags(setting)
    return {
        fader = (settings.sourceroute.fader ~= 0),
        phase = (settings.sourceroute.phase ~= 0),
        assigned = true
    }
end

function _M:_onClickMatrixCell(cell)
    local cf = chanflags.unpack( cell:value() )
    local flags = self:chanflags()
    if cf.assigned and (cf.phase ~= flags.phase or cf.fader ~= flags.fader) then
        cf.phase = flags.phase
        cf.fader = flags.fader
    elseif not cf.assigned then
        cf = flags
    else
        cf = nil
    end
    cell:setValue( chanflags.pack(cf) )
end
_M.onClick_TracksMatrixCell = _M._onClickMatrixCell
_M.onClick_OutputsMatrixCell = _M._onClickMatrixCell

