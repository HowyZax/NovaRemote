local config = require 'config'
local colors = require 'colors'
local chanflags = require 'remote.chanflags'
local sources = require 'remote.sources'

MixPageDelegate = Object:subclass('MixPageDelegate')
MixPageDelegate:attributes{ 'prefix' }

local asciiArt = { }

if config.nova then
    asciiArt.MixA    = MatrixAsciiArt('AA|AA|RR')
    asciiArt.MixB    = MatrixAsciiArt('DDDD|DDDD|XXXX|XXXX') 
    asciiArt.MixBtrk = MatrixAsciiArt('DDDD|DDDD|XXXX|XXXX|OOOO') 
    -- MixC is for Output 4-8 Mix
    asciiArt.MixC = MatrixAsciiArt('TTTTTTTT|TTTTTTTT|------RR|OOOO') -- Nova has 16 tracks for now
elseif config.nomad then
    asciiArt.MixA = MatrixAsciiArt('AA|AA|AA')
    asciiArt.MixB = MatrixAsciiArt('RRRR|DDDD|DDDD')
else
    asciiArt.MixA = MatrixAsciiArt('AAAAAAAA|AAAAAAAA|DDDDDDDD|DDDDDDDD')
end

--{{{ class MixMatrixCell(SourceMatrixCell)
-------------------------------------------------------------------
MixMatrixCell = SourceMatrixCell:subclass('MixMatrixCell')
--}}}

--{{{ AuxPanel
local function AuxButton(items)
    local panel = CellPanel {
        w = items.w,
        background=0xD0000000, --'transparent',
        Label {
            items[1] or '',
            h = 0.4,
--            margin = { 0.05, 0.2, 0.05, 0.1 },
            align = 'center',
            fit = items.fit,
        },
    }
    panel:insert(
        Button {
            style = 'button.cell',
            fit = 3, -- items.fit or { 4, 10 },
            margin = 0,
            setting = items.setting,
            index = items.onIndex,
            id = items.id,
            elements = LayoutElement {
                rows = 1,
                TextElement {
                    id = '.value',
                    color = 'gold',
                    fit = 5,
                },
--                GraphicElement {
--                    graphic = 'circle',
--                    margin = 0.25,
--                    constrain = 1,
--                },
            },
        } )
    return panel
end

local function AuxPanelTrackMix(idx)
    local factor = not config.nomad and 2 or 1
    local function onIndex(c)
        print('AuxPanelTrackMix.onIndex',  settings.trackmix.which, idx, c.tag)
        return { settings.trackmix.preset, settings.trackmix.which*factor + idx }
    end
    local fit = 'SLATE' -- 'NEVERCLIP'
    local panel = Panel {
        rows = 1,
        margin = { 0, 0.2 },
        AuxButton { "ISO ATT", setting='trackmix.neverclip', fit=true, onIndex=onIndex, id='IsoAtten'  },
        AuxButton { "COMP", setting='trackmix.comp', fit=fit, onIndex=onIndex },
        AuxButton { "TONE", setting='trackmix.tone',  fit=fit, onIndex=onIndex },
        AuxButton { "SLATE", setting='trackmix.slate', fit=fit, onIndex=onIndex },
    }
    if not config.nomad then
        panel.margin = { 0, 0.1 }
        panel:insert( 1, Panel{ } )
        panel:insert( 1, Panel{ } )
        panel:insert( 1, Panel{ } )
        panel:insert( 1, Panel{ } )
    end

    return panel
end

local function AuxPanelOutputMix(idx)
    local factor = not config.nomad and 2 or 1
    local function onIndex(c)
        return { settings.outputmix.preset, settings.outputmix.which*factor+idx }
    end

    local fit = 'SLATE' -- 'NEVERCLIP'
    local panel = Panel {
        rows = 1,
        margin = { 0, 0.2 },
        AuxButton { "COMP", setting='outputmix.comp', fit=fit, onIndex=onIndex },
        AuxButton { "TONE", setting='outputmix.tone',  fit=fit, onIndex=onIndex },
        AuxButton { "SLATE", setting='outputmix.slate', fit=fit, onIndex=onIndex },
    }


    if not config.nomad then
        panel.margin = { 0, 0.1 }
        panel:insert(1,  AuxButton { "ISO ATT", setting='outputmix.neverclip', fit=true, onIndex=onIndex, id='IsoAtten' } )
        panel:insert( AuxButton { "TALK1", setting='outputmix.com1', fit=fit, onIndex=onIndex } )
        panel:insert( AuxButton { "TALK2", setting='outputmix.com2', fit=fit, onIndex=onIndex } )
        panel:insert( AuxButton { "TRK1", setting='outputmix.trk1', fit=fit, onIndex=onIndex } )
        panel:insert( AuxButton { "TRK2", setting='outputmix.trk2', fit=fit, onIndex=onIndex } )
    else        
        panel:insert( AuxButton { "TALK1", setting='outputmix.com1', fit=fit, onIndex=onIndex } )
    end

    return panel
end

--}}}
--{{{ MixMatrix
local MixMatrix = Matrix:subclass('MixMatrix')
MixMatrix:attributes{ 'asciiArt' }

function MixMatrix:__init(items)
    items.rows = items.asciiArt:rows()
    items.columns = items.asciiArt:columns()
    items.cellClass = MixMatrixCell    
    items.cellFactory = function(matrix, cell, row, col, idx)
        return matrixAsciiArt(items.asciiArt:artwork(), idx, function(s, i)
                return MixMatrixCell {
                        matrix=matrix, row=row, col=col, id=matrix.id..'Cell',
                        tag=sources.inputIndex(s, i),
                        setting=matrix.setting
                    }
            end ) or Spacer { }
        end

    super(MixMatrix).__init(self, items)
end

--}}}
--{{{ MixPanel
-------------------------------------------------------------------
local function MixPanel(items)
    assert(items.setting)
    assert(items.background)
    assert(type(items.auxpanel) == 'function')
    local idx = items[1]

    local panel = Panel {
        columns = 1,
        background = items.background,
        style = 'panel.gradient',
        margin = 0.03,
        id = 'MixPanel',
        tag = idx,
        drop = 'source',
        Panel {
            h = 0.3,
            rows = 1,
            --HSpacer { 0.05 },
            Label { 'TRACK ' .. idx, w=0.3, margin={ 0.05,0,0,0}, id='MixPanelLabel' .. idx,
                                fit=config.nomad and "TRACK 12" or "TRACK 8",
                                color='white'},
            HSpacer { config.nomad and 0.1 or 0.02 },
            items.auxpanel(idx),
        },
    }
    if not config.nomad then
        panel:insert(
            MixMatrix {
                id = 'MatrixA',
                tag = idx,
                setting = items.setting,
                asciiArt = asciiArt.MixA,
            } )
        panel:findById('MatrixA').sourceOffset = 0
    else
				local ASCIIIartB = asciiArt.MixB
				if items.setting:find('trackmix') then ASCIIIartB = asciiArt.MixBtrk end
        local p1 = Panel {
            rows = 1,
            margin = 0,
            id = 'MixMatrixPanelAB',
            background = false, -- ensure backed by a window
            MixMatrix {
                w = 0.33,
                id = 'MatrixA',
                tag = idx,
                setting = items.setting,
                asciiArt = asciiArt.MixA,
           },
           MixMatrix {
                id = 'MatrixB',
                tag = idx,
                setting = items.setting,
                asciiArt = ASCIIIartB, -- asciiArt.MixB,
            }
        }
        panel:insert(p1)

        if items.setting:find('outputmix') and asciiArt.MixC then
            local p2 = Panel {
                rows = 1,
                margin = 0,
                id = 'MixMatrixPanelC',
                background = false, -- ensure backed by a window
                MixMatrix {
                    id = 'MatrixC',
                    tag = idx,
                    setting = items.setting,
                    asciiArt = asciiArt.MixC,
                }
            }
            panel:insert(p2)
        end
    end
    return panel
end
--}}}
--{{{ MatrixPanel
-- Function for creating a panel with matrices for mix screens (output mix, disk mix etc)
-- Matrices are compatible with MixPageDelegate
local function MatrixPanel(items)
    assert(items.setting)
    local panel = Panel {
        rows = 1,
        margin = 0, -- { 0, 0.02, 0, 0 },
        id = 'MatrixPanel',
        grid = false,
        MixPanel{ 1, setting=items.setting, auxpanel=items.auxpanel, background=items.background },
    }
    if not config.nomad then
        panel:insert(
            MixPanel{ 2, setting=items.setting, auxpanel=items.auxpanel, background=items.background }
        )
    end
    return panel
end
--}}}

--{{{ Page: TrackMix
-------------------------------------------------------------------
if not config.nomad then Page {
    title = "Track Mix",
    delegate = MixPageDelegate { prefix = 'trackmix' },
    depends = {
        'input.color',
        'input.name',
        'input.source',
        'input.warning',
        'neverclip.attenuation',
    },
--  constrain = { 2, 4 },
    Panel {
        x = 0,
        id = 'MainPanel',
        rows = 1,
        Panel {
            w=0.1,
            columns = 1,
            id = 'ButtonPanel',
            IncDecButton { "PRESET", setting='trackmix.preset', names='trackmix.names' },
            --Button { "INPUT", setting='trackmix.ad', id='AD' },
            Spacer { },
            Button { "FADER", setting='trackmix.fader', id='Fader' },
            Button { "PHASE", setting='trackmix.phase', id='Phase' },
            Button { "CLEAR|ALL", id='Clear' },
        },
        Panel {
            columns = 1,
            background = true,
            Panel {
                columns = 1,
                margin = 0,
                background = true,
                MatrixPanel {
                    setting = 'trackmix.matrix',
                    auxpanel = AuxPanelTrackMix,
                    background = 'trackmix',
                },
                Panel {
                    background = 'trackmix',
                    style = 'panel.gradient',
                    id = 'IsoMixPanel',
                    columns = 1,
                    --margin = 0.03, --{ 0.03, 0.05, 0.02, 0.08 },
                    margin = { 0.08, 0.15, 0.08, 0.03 },
                    VSpacer { 0.1 },
                    Label { h = 0.15, 'ISO TRACKS', align='center', color='white' },
                    -- HSpacer { 0.1 },
                    Matrix {
                        columns = 12,
                        rows = 2,
                        margin = 0.05, --, 0.1 }, --{ 0.08, 0.05, 0.08, 0.08 },
                        id = 'IsoMatrix',
                        -- cellTitles = function(self, row,col) return 'ISO'.. tostring(self.columns*(row-1)+9+col-1) end,
                        drop = 'source',
                        setting = 'trackmix.isomatrix',
                        -- cellClass = MixMatrixCell,
                        -- cellClass = SourceMatrixCell,
                        cellFactory = function (matrix, cell, row, col, idx)
                                        return CellPanel {
                                            columns = 1, margin = 0.03,
                                            --background = 'navy',
                                            Label {
                                                -- 'ISO'.. tostring(matrix.columns*(row-1)+9+col-1),
                                                'ISO '.. (idx+8),
                                                h = 0.4,
                                                margin = { 0.2, 0 },
                                                -- margin = { 0.05, 0.2, 0.05, 0.1 },
                                                align = 'center',
                                                fit = 'ISO 32',
                                                color = 'white',
                                            },
                                            MixMatrixCell {
                                                matrix=matrix, row=row, col=col,
                                                id = matrix.id .. 'Cell',
                                                setting=matrix.setting,
                                                delegate = matrix.delegate,
                                                margin = 0,
                                            }
                                        }
                                    end,

                    },
                },
            },
            MatrixSources{ },
        },
        Panel {
            w=0.1,
            columns = 1,
            CheckedGroup {
                setting = 'trackmix.which',
                fit = true,
                columns = { 0.3, 0.7 },
            },
        },
    },
}
else Page {             -- config.nomad or config.nova
    title = "Track Mix",
    delegate = MixPageDelegate { prefix = 'trackmix' },
    depends = {
        'input.color',
        'input.name',
        'input.source',
        'input.warning',
        'neverclip.attenuation',
    },
    columns = 1,
    Panel {
        x = 0,
        id = 'MainPanel',
        rows = 1,
        Panel {
            w=0.2,
            columns = 1,
            id = 'ButtonPanel',
            Spacer { }, --IncDecButton { "PRESET", setting='trackmix.preset', names='trackmix.names' },
            Button { "FADER", setting='trackmix.fader', id='Fader' },
            Button { "PHASE", setting='trackmix.phase', id='Phase' },
        },
        Panel {
            columns = 1,
            background = true,
            Panel {
                columns = 1,
                margin = 0,
                background = true,
                MatrixPanel {
                    setting = 'trackmix.matrix',
                    auxpanel = AuxPanelTrackMix,
                    background = 'trackmix',
                },
            },
        },
    },
    Panel {
        h = 0.25,
        background = true,
        rows = 1,
        Panel {
            w = 0.2,
            columns = 1,
            margin = { 0.03, 0.1 },
            Label{ 'Track',  align='center', fit=6, },
            Label{ 'Select', align='center', fit=6, },
        },
        Matrix {
            margin = { 0.03, 0.05 },
            ncells = config.ntracks,
            rows = 2,
            id = 'TrackSelect',
            cellIndex = function(cell) return (cell.row-1) * cell.matrix.columns + cell.col end,
            fit = tostring(config.ntracks),
        },
    },
}

end
--}}}
--{{{ Page: OutputMix
if not config.nomad then Page {
    title = "Output Mix",
    delegate = MixPageDelegate { prefix = 'outputmix' },
    depends = {
        'input.color',
        'input.name',
        'input.source',
        'input.warning',
        'neverclip.attenuation',
    },
    Panel {
        x = 0,
        id = 'MainPanel',
        rows = 1,
        Panel {
            w=0.1,
            columns = 1,
            id = 'ButtonPanel',
            IncDecButton { "PRESET", setting='outputmix.preset', names='outputmix.names' },
            --Button { "INPUT", setting='outputmix.ad', id='AD' },
            Spacer { },
            Button { "FADER", setting='outputmix.fader', id='Fader' },
            Button { "PHASE", setting='outputmix.phase', id='Phase' },
            Button { "CLEAR|ALL", id='Clear' },
        },
        Panel {
            columns = 1,
            background = true,
            MatrixPanel{
                setting = 'outputmix.matrix',
                auxpanel = AuxPanelOutputMix,
                background = 'outputmix',
            },
            MatrixSources{ },
        },
        CheckedGroup {
            w = 0.1,
            setting = 'outputmix.which',
            fit = true,
            columns = { 0.3, 0.7 },
        },
    },
}
else Page { -- For config.nomad
    title = "Output Mix",
    delegate = MixPageDelegate { prefix = 'outputmix' },
    depends = {
        'input.color',
        'input.name',
        'input.source',
        'input.warning',
        'neverclip.attenuation',
    },
    columns = 1,
    Panel {
        x = 0,
        id = 'MainPanel',
        rows = 1,
        Panel {
            w=0.2,
            columns = 1,
            id = 'ButtonPanel',
            Spacer { }, --IncDecButton { "PRESET", setting='outputmix.preset', names='outputmix.names' },
            Button { "FADER", setting='outputmix.fader', id='Fader' },
            Button { "PHASE", setting='outputmix.phase', id='Phase' },
        },
        Panel {
            columns = 1,
            background = true,
            MatrixPanel {
                background = 'outputmix',
                setting = 'outputmix.matrix',
                auxpanel = AuxPanelOutputMix,
            },
        },
    },
    Panel {
        h = 0.25,
        background = true,
        rows = 1,
        Panel {
            w = 0.2,
            columns = 1,
            margin = { 0.03, 0.1 },
            Label{ 'Output', align='center', fit=6, },
            Label{ 'Select', align='center', fit=6, },
        },
--[[
        CheckedGroup {
            setting = 'outputmix.which',
            fit = true,
            rows = 1,
            margin = { 0.02, 0.2 },
        },
--]]
---[[
        Matrix {
            margin = { 0.03, 0.05 },
            rows = 1,
            ncells = config.noutputs,
            id = 'OutputSelect',
            cellIndex = function(cell) return (cell.row-1) * cell.matrix.columns + cell.col end,
            cellFactory = function(matrix, cell, row, col, idx)
                --if idx == 12 then return Button { 'PAN FOLLOW', setting='outputmix.pan.follow' } end
--              if idx > 10 then return Panel{ } end
                return MatrixCell{
                    matrix=matrix, row=row, col=col,
                    id = matrix.id .. 'Cell',
                    fit = 'OUT 6', --idx > 6 and 5 or { 4, 10 },
                }
            end,
        },
--]]
    },

}
end
--}}}

--{{{ MixPageDelegate
local _M = MixPageDelegate

function MixPageDelegate:onCreatedPage(page)
    --print('MixPageDelegate.onCreatedPage', page, page.name)
    assert(self.prefix)
    self.page = page
    self.trackOffset = 0
    self.settings = settings[self.prefix]
    self.setting = SettingFind(self.prefix .. '.matrix')
    --assert(self.setting.__classname == 'SettingMatrix')
    for _,c in ipairs{self.matrix8, self.matrix16, self.matrixAux } do
        if c then SettingNotify(self.prefix .. '.preset', c.update, c) end
    end
    self.settingWhich = SettingFind(self.prefix .. '.which')
    SettingNotify(self.prefix .. '.which', self.onChangeWhich, self)
    SettingNotify(self.prefix .. '.preset', self.onChangePreset, self)
end

function MixPageDelegate:onEnterPage()
    self:updateLayout()
end

function MixPageDelegate:clear()
    self.setting:clear( { self.settings.preset } )
end

function MixPageDelegate:onClickClear()
    MsgBox.Show(
        self.page,
        "Clear the current preset?",
        { "Ok", function() self:clear() end } ,
        { "Cancel" } )
end

function MixPageDelegate:onIndexAuxMatrixCell(cell)
--  print ('onIndexAuxMatrixCell', cell, cell.row, cell.col, cell.matrix.id, cell.matrix.tag, self.trackOffset )
    return { self.settings.preset, cell.matrix.tag + self.trackOffset }
end

--------------------------------------------------------------------------------

function MixPageDelegate:onIndexMatrixCell(cell)
    return { self.settings.preset,                          -- preset
             cell.matrix.tag + self.trackOffset,            -- track number
             cell.tag                                       -- source number
    }
end
MixPageDelegate.onIndexMatrixACell = MixPageDelegate.onIndexMatrixCell
MixPageDelegate.onIndexMatrixBCell = MixPageDelegate.onIndexMatrixCell
MixPageDelegate.onIndexMatrixCCell = MixPageDelegate.onIndexMatrixCell

function MixPageDelegate:onIndexIsoMatrixCell(cell)
    return { self.settings.preset, (cell.row-1)*cell.matrix.columns + cell.col }
end

function MixPageDelegate:chanflags(chan)
    return {
        chan = chan,
        fader = (self.settings.fader ~= 0),
        phase = (self.settings.phase ~= 0),
        assigned = true
    }
end

--------------------------------------------------------------------------------

function MixPageDelegate:_onUpdateMixMatrixCell(cell, cf, allowFader)
    cell:setContent {
        fader = cf.assigned and not cf.fader and allowFader,   -- Draw a fader on right if pre-fade (ie. no fader on signal!)
        phase = cf.assigned and cf.phase,
        input = cf.chan,
        small = not cf.assigned,
        color = not cf.assigned and 'white' or nil,
        background = self.prefix
    }
end

function MixPageDelegate:onUpdateMatrixCell(cell)
    local chan = cell.tag
    local cf = chanflags.unpack( cell:value() )
    cf.chan = chan
    local allowFader = not sources.isOutput(chan)
    self:_onUpdateMixMatrixCell(cell, cf, allowFader)
end
MixPageDelegate.onUpdateMatrixACell = MixPageDelegate.onUpdateMatrixCell
MixPageDelegate.onUpdateMatrixBCell = MixPageDelegate.onUpdateMatrixCell
MixPageDelegate.onUpdateMatrixCCell = MixPageDelegate.onUpdateMatrixCell

function MixPageDelegate:onUpdateIsoMatrixCell(cell)
    local cf = chanflags.unpack( cell:value() )
    if not cf.assigned then cf.chan = nil end
    self:_onUpdateMixMatrixCell(cell, cf)
end

--------------------------------------------------------------------------------

function MixPageDelegate:onClickMatrixCell(cell)
    local chan = cell.tag
    local cf = chanflags.unpack( cell:value() )
    local flags = self:chanflags()  -- ignore chan in saved setting
    local isOutput = sources.isOutput(chan)
    if isOutput then flags.phase = nil; flags.fader = nil end    -- No phase/fader for Outputs
    if cf.assigned and not isOutput and (cf.phase ~= flags.phase or cf.fader ~= flags.fader) then
        cf.phase = flags.phase
        cf.fader = flags.fader
    elseif not cf.assigned then
        cf = flags
    else
        cf = nil
    end
    cell:setValue( chanflags.pack(cf) )
end
MixPageDelegate.onClickMatrixACell = MixPageDelegate.onClickMatrixCell
MixPageDelegate.onClickMatrixBCell = MixPageDelegate.onClickMatrixCell
MixPageDelegate.onClickMatrixCCell = MixPageDelegate.onClickMatrixCell

--------------------------------------------------------------------------------

if not config.nomad then function MixPageDelegate:onDropMixPanel(panel, _srcWidget)
    local source = _srcWidget:owner()
    local chan = panel.tag + self.trackOffset
    --local i = source:dropValue()
    print ('MixPageDelegate:onDropMixPanel', panel.tag, source.__classname, source.dropValue, source:dropValue(), source:index())
    local index = { self.settings.preset, chan, source:index() }
    self.setting:setValue( chanflags.pack(self:chanflags()), index)
    panel:update()
end end

function MixPageDelegate:updateLayout()
    local idx = self.settingWhich:value()
    local m = self.page.ui.isomixpanel
    local iso = m and (idx == 4)
    if m then
        m:setVisible(iso)
        m = self.page.ui.matrixpanel
        m:setVisible(not iso)
    end

    if config.nova then
        local panel1 = self.page:findById('MixMatrixPanelAB')
        local panel2 = self.page:findById('MixMatrixPanelC', false)
        if panel2 then
            local alt = (idx >= 4)
            print ('setVisible', alt)
            panel1:setVisible(not alt)
            panel2:setVisible(alt)
        end
    end

    if not iso then
        -- Extract labels for current selection
        -- e.g 'TRACK', '3', '5'
        local which = self.settingWhich:value()
        local s, n = self.settingWhich:text():match('(%S+)%s*(%d*)')
        if not config.nomad then which = which * 2 end
        local s1, s2 = s, s
        print('onChangePreset', s, n, #n)
        if n and #n > 0 then
            s1 = s .. ' ' .. which+1
            s2 = s .. ' ' .. which+2
        end
        m = self.page.ui.mixpanellabel1
        if m then m:setText(s1) end
        m = self.page.ui.mixpanellabel2
        if m then m:setText(s2) end

        self.trackOffset = which -- - 1
        assert(self.trackOffset >= 0)

    end
    self.page:layout()
end

function MixPageDelegate:onChangeWhich(setting)
    print (self.__classname, 'onChangeWhich', setting:value())
    self:updateLayout()
    self.page:update()
end

function MixPageDelegate:onChangePreset()
    self.page:update()
end

function MixPageDelegate:onClickIsoMatrixCell(cell)
    local cf = chanflags.unpack( cell:value() )
    local flags = self:chanflags()
    if cf.assigned and (cf.phase ~= flags.phase or cf.fader ~= flags.fader) then
        cf.phase = flags.phase
        cf.fader = flags.fader
    else
        cf = nil
    end
    cell:setValue( chanflags.pack(cf) )
end

function MixPageDelegate:onDropIsoMatrixCell(cell, _srcWidget)
    local source = _srcWidget:owner()
    local i = source:index()
    for _,c in ipairs(cell.matrix:findByClass('MatrixCell')) do
        local cf = chanflags.unpack( c:value() )
        if cf.chan == i then
            c:setValue(nil)
        end
    end
    cell:setValue( chanflags.pack(self:chanflags(i)) )
    self.page:update()
end

function MixPageDelegate:onUpdateTrackSelectCell(cell)
    cell:setText( tostring(cell:index()) )
end

function MixPageDelegate:onClickTrackSelectCell(cell)
    self.settingWhich:setValue( cell:index()-1 )
end

function MixPageDelegate:onUpdateOutputSelectCell(cell)
    cell:setText( OutputName( cell:index(), '') )
end

function MixPageDelegate:onClickOutputSelectCell(cell)
    self.settingWhich:setValue( cell:index()-1 )
end

function MixPageDelegate:onUpdateIsoAtten(cell)
    local value = cell:value()
    local s = value > 0 and Setting('neverclip.attenuation'):text() or ''
    cell.ui['.value']:setText(s)
end

--}}}

-- vim: set foldmethod=markers
