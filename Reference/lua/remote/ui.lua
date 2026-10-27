---------------------------------------------------------------------
-- Deva32 screen layouts
--
-- David Brenan <dbrenan@one-n.com>
---------------------------------------------------------------------
local message = require 'message'
local colors = require 'colors'
local stringx = require 'pl.stringx'
local config = require 'config'
local chanflags = require 'remote.chanflags'
local sources = require 'remote.sources'

colors.alias('trackmix', 'royalblue')
colors.alias('outputmix', 'limegreen')
colors.alias('faderassign', 'darksalmon')

function doNothing()
end

-------------------------------------------------------------------
-- For input names and track names
InputNameElement = LayoutElement:subclass('InputNameElement')
InputNameElement.columns = 1
InputNameElement.padding = { 0.05, 0.05 }

function InputNameElement:__init(items)
    super(InputNameElement).__init(self, items)
    table.insert(self, TextElement{ margin=0, font='usertext'} )
    table.insert(self, TextElement{ margin=0, font='usertext'} )
end

function InputNameElement:setContent(text)
    self.text = text    -- Keep original text for text editor
    local lst = stringx.split(text or '')
    lst = lst:slice(1,2):transform(function(s) return s:sub(1, 8) end)
    local smax = 'XXXXX'
    for _,s in ipairs(lst) do
        if #s > #smax then smax = s end
    end
    if #lst == 2 and #lst[1] + #lst[2] < 8 then
        lst = { lst[1] .. ' ' .. lst[2] }
    end
    for i,e in ipairs(self) do
        local s = lst[i] or ''
        e:setContent(s)
        if tonumber(s) and tonumber(s) < 99 then e.fit = 'MMM'
        elseif #s <= #smax - 1 then e.fit = smax
        else e.fit = s
        end
        if i > 1 then e:setVisible(#s > 0) end
    end
    self:layout()
end
InputNameElement.setText = InputNameElement.setContent

function InputNameElement:setPhase(state)
    self[1].overline = state
end

function InputNameElement:setColor(color)
    for _,e in ipairs(self) do
        e:setColor(color)
    end
end

function InputNameElement:setFont(font)
    for _,e in ipairs(self) do
        e:setFont(font)
    end
end

-------------------------------------------------------------------
SourceMatrixCell = MatrixCellEx:subclass('SourceMatrixCell')
SourceMatrixCell.fit = { 2, 8 }

function SourceMatrixCell:__init(items)
    if not items.elements then
        items.elements = LayoutElement {
            columns = 1,
            margin = 0,
            LayoutElement {
                rows = 1,
                id = 'body',
                margin = 0,
                GraphicElement{ w=0.3, constrain=1, id='graphic', visible=false },
                InputNameElement{ margin = 0, id='usertext' }, -- items.fit or self.fit
                TextElement{ margin = 0, id='text' }, -- items.fit or self.fit
                GraphicElement{ w=0.2, graphic='fader-white', margin={0,0.2}, id='fader' },
            }
        }
    end
    super(SourceMatrixCell).__init(self, items)
end

function SourceMatrixCell:onIndex()
    return (self.row-1)*self.matrix.columns + self.col
end

function SourceMatrixCell:setContent(options)
    local graphic, text, background
    assert(type(options) == 'table')
--  print('SourceMatrixCell:setContent', options.input)

    self.ui.body.h = options.small and 0.4 or 1.0
--  self.ui.usertext.align = options.small and 'left' or 'center'
    self.ui.text.align = options.small and 'left' or 'center'

    self.style = 'button.cell'

    -- If caller has specified an options.input channel then
    -- get the corresponding text and graphic.

    local which = options.input
    if which ~= nil and which > 0 then
        local chan
        local name
        local dante = SettingFind('input.source'):value(which) -- input.source[which]
        local warning = SettingFind('input.warning'):value(which) -- input.source[which]

        if warning > 0 then self.style = 'button.warning' end

        if not options.small then
            name, background = InputName(which)
        end
--      print ('SourceMatrixCell:onUpdate', which, dante, name, background)
        dante = tonumber(dante)
        if dante and dante > 0 then
            chan, graphic = dante, 'dante'
        else
            name, graphic = sources.inputDescriptionWithGraphic(which)
        end
--[[
        if not name or #name == 0 then
            name = tostring(chan)
        else
            graphic = nil
        end
--]]
        if options.prefix then
            name = options.prefix .. name
        end

        text = name
        if options.small then
            self.ui.usertext:setVisible(false)
            self.ui.text:setVisible(true)
            self.ui.text:setText(name)
        else
            self.ui.usertext:setVisible(true)
            self.ui.text:setVisible(false)

            self.ui.usertext:setText(name)
            self.ui.usertext:setPhase(options.phase or false)
        end
    elseif options.usertext then
        self.ui.usertext:setVisible(true)
        self.ui.text:setVisible(false)
        self.ui.usertext:setText(options.usertext)
    else
        self.ui.usertext:setVisible(false)
        self.ui.text:setVisible(true)
    end

    -- Adjust the elements of the control and layout

    graphic = graphic or options.graphic
    text = text or options.text or ''
    background = background or options.background

    local ge = self.ui.graphic
    ge:setVisible(graphic ~= nil)
    ge:setContent(graphic)

    local te = self.ui.text
    if te.visible then
        te.align = ((graphic or options.small) and options.input) and 'left' or 'center'
        te.margin = options.small and not graphic and { 0.1 } or nil
        if options.fit then te.fit = options.fit end
        if options.font then te:setFont(options.font) end
        if options.color then te:setColor(options.color) end
        te.overline = options.phase or false
        te:setContent(text)
    end

    self.ui.fader:setVisible(options.fader or false)
    if background then
        self.ui.fader.graphic = 'fader-' .. colors.textColor(background)
    end

    self:setBackground(background)

    self:layout()

end

function SourceMatrixCell:onUpdate()
    self:_onUpdate(self:index())
end

function SourceMatrixCell:_onUpdate(which, defaultBackground)
    self:setContent{
        input = which,
        background = defaultBackground
    }

end

function SourceMatrixCell:onClick()
    return true
end

-------------------------------------------------------------------
-- Get the user's input name & color assignment, if any
function InputName(chan)
    return Setting('input.name'):value(chan),
            Setting('input.color'):value(chan)
end

function OutputName(idx, prefix)
    local prefix = prefix == nil and 'OUT ' or prefix
    local s = prefix .. tostring(idx)
    if config.nova then
        -- pass
    elseif config.nomad then    -- TODO: remove
        if     idx == 7 then s = 'HPH2L'
        elseif idx == 8 then s = 'HPH2R'
        elseif idx == 9 then s = 'MONO'
        elseif idx == 10 then s = 'T/IFB'
        end
    end
    return s
end

function TrackName(idx)
    return 'TRK ' .. tostring(idx)
end

local pretty = require 'pl.pretty'

local function isIsoChan(chan)
    return (not config.nomad) and chan > 8
end

-------------------------------------------------------------------
function TrackDescription(chan)    -- config.nomad
    if not isIsoChan(chan) then
        local text = SettingFind( 'track.name'):text(chan)
        local color = SettingFind('track.color'):value(chan)
        local setting = SettingFind('trackmix.matrix')
        local sources = setting:value({settings.trackmix.preset, chan })
        local routed = false
        if type(sources) == 'table' then
            local source
            for k,v in pairs(sources) do
                assert(type(k) == 'number')
                if v ~= 0 then  -- TODO: use chanflags
                    routed = true
                    if source then
                        source = nil
                        break
                    end
                    source = k
                end
            end
--          print('TrackDescription', chan, routed, 'sources', pretty.write(sources,''))
            if source then
                local inputText, inputColor = InputName(source)
                if inputText and #inputText > 0 then
                    return inputText, inputColor, routed
                end
            end
            if not text or #text == 0 then
                text = ( config.nomad and 'TRACK ' or 'MIX ') .. tostring(chan)
            end
            return text, color, routed
        end
    else
        local iso = chan-8
        local setting = SettingFind('trackmix.isomatrix')
        local cf = chanflags.unpack( setting:value({ settings.trackmix.preset, iso }) )
        local color = nil
        if cf.assigned then
            local text
            text,color = InputName(cf.chan)
            if text and #text > 0 then return text, color, true end
--          return tostring(source) --InputDescription(source)
        end
        return 'ISO ' .. tostring(iso+8), color, false
    end
end

-------------------------------------------------------------------
if not config.nomad then
    function MatrixSources(items)
        return Matrix {
            --margin = {0.04,0.15,0.1,0},
            margin = {0.04,0.15,0.04,0},
            h = 0.3,
            id = 'Source',
            columns = 16,
            rows = 2,
            drag = 'source',
            setting = 'input.source',
            cellClass = SourceMatrixCell,
        }
    end
else
    function MatrixSources(items)
        return Matrix {
            --margin = {0.04,0.15,0.1,0},
            margin = {0.04,0.15,0.04,0},
            h = 0.3,
            id = 'Source',
            columns = 6 + 1 + 4,    -- nanalog + spacer + ndigital/2
            rows = 2,
            drag = 'source',
            setting = 'input.source',
            cellFactory = function(matrix, cell, row, col, idx)
                            return matrixAsciiArt('AAAAAA-DDDD|'..'RRRR---DDDD', idx, function(s, i)
                                    print('MatrixSources', row, col, idx, s, i, sources.inputIndex(s, i))
                                    return SourceMatrixCell{
                                            matrix=matrix, row=row, col=col,
                                            tag=sources.inputIndex(s, i),
                                            onIndex = function(self) return self.tag end,
                                        }
                                end ) or Spacer { }
                          end,
        }
    end
end


function enumerate(first, last, fn)
    local tbl = { }
    for i = first,last,1 do
        table.insert(tbl,  fn and fn(i) or i)
    end
    return tbl
end


message.subscribe('setting.changed', function(msg, setting, index)
    -- TODO: only process visible pages
    for _,page in pairs(pages) do
        page:traverse( function(o) if (o.setting == setting and o:isVisible()) then o:update() end end )
    end
end)

require( 'remote.pages' )
if config.player then 
    require 'player.ui' 
end

