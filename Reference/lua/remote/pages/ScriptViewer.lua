require 'pl'
require 'fdviewer'
local message = require 'message'

-------------------------------------------------------------------
local PDFViewer = Control:subclass('PDFViewer')
PDFViewer.widgetclass = require 'pdf2.core'

function PDFViewer:open(path)
    return self.widget:open(path)
end
function PDFViewer:scroll(...)
    if self:isVisible() then
        self.widget:scroll(...)
    end
end
-------------------------------------------------------------------
local Scroller = class('Scroller')

function Scroller:__init(viewer)
    self.timer = ElapsedTimer()
    self.events = { }
    self.viewer = assert(viewer)
    self.stopped = true
    message.subscribe('timer.tick', self.onTimerTick, self)
end

function Scroller:advance()
    if self.events.playindex then
        local idx = self.events.playindex
        if idx <= #self.events then
            local when = self.events[idx][1]
            if self.timer:elapsed() >= when then
                local pos = self.events[idx][2]
                if idx == 1 then
                    self.viewer:setPosition(pos)
                else
                    self.viewer:scrollTo(pos)
                end
                self.events.playindex = idx + 1
            end
        else
            self.events.playindex = nil
        end
    end
end

function Scroller:onTimerTick()
    self:advance()
end

function Scroller:scrollBy(step)
    local when = self.timer:elapsed()
    local pos = self.viewer:scrollStep( step )
    if #self.events > 0 and not self.stopped then
        if self.events.playindex ~= nil then
            assert(self.events.playindex >= 2)
            while #self.events >= self.events.playindex do
                table.remove(self.events)
            end
            self.events.playindex = nil -- cancel playback of events
        end
        print 'ADD EVENT'
        table.insert(self.events, { when,  pos } )
    end
    print ('Scrolling to',  pos, 'currently at',  self.viewer:position(), when, self.timer:elapsed() )
end

function Scroller:setStart()
    self.events = {
        { 0, self.viewer:position() }
    }
    self.timer:restart()
end

function Scroller:show()
    print('--- EVENTS ---', self.events.playindex)
    for i,v in ipairs(self.events) do
        print (i, v[1], v[2])
    end
    print '--- END EVENTS ---'
end

function Scroller:start()
    self.stopped = false
    self.events.playindex = #self.events > 0 and 1 or nil
    self.timer:restart()
    print('start',  self.events.playindex,  #self.events)
    self:show()
    self:advance()
end

function Scroller:stop()
    self.stopped = true
    self.events.playindex = nil
    self:show()
end

function Scroller:reset()
    self.events = { }
end

function Scroller:isStopped()
    return self.stopped
end

function Scroller:gotoStart()
    if #self.events > 0 then
        local pos = self.events[1][2]
        self.viewer:setPosition(pos)
    end
end

-------------------------------------------------------------------
local P = Page {
    title = "Script",
    delegate = true,
    Panel {
        Panel {
            h = 60, -- 0.05, -- 60
            rows = 1,
            --Button{ graphic='plus-square', constrain=1,  align='left', id='Load' },
            --Button{ graphic='file-text', constrain=1,  align='left', id='Load' },
            Button{ 'Load|Script', w=0.2, id='Load' },
            Panel {
                rows = 1,
                margin = 0,
                id = 'fdbuttons',
                Button{ 'Actions', setting='script.show.actions', id='Actions' },
                Button{ 'Edit', setting='script.edit.enable', id='Edit' },
                Button{ 'Scroll By', setting='script.scroll.step' },
                Button{ 'Font', setting='script.font' },
                Panel {
                    rows = 1,
                    w = 0.4,
                    Button{ 'Set|Start', id='SetStart' },
                    Button{ graphic='up', id='ScrollUp'},
    --              Button{ id='GotoStart' },
                    Button{ graphic='down', id='ScrollDown'}
                },
            },
        },

        Panel {
            --h = 1.0,
            id = 'MyPanel',
            columns = 1,
            FDViewer  { x=0, id='fdview' },
            PDFViewer { x=0, id='pdfview' },
        },

        Panel {
            id = 'CharacterPanel',
            h = 300, --0.3,
            Label { 'Import character names & colors?', h=0.15, align='center', font=0.8, fit=24 },
            Matrix {
                rows = 3,
                columns = 5,
                id = 'Character',
                fit = { 10, 16 },
            },
            Panel {
                h = 50, --0.15,
                rows = 1,
--              Spacer{ w = 0.05},
--              Label { w=0.5, 'Do you want to import character names & colors?' },
                Button{ 'Accept', id='Accept', background='green' }, --, background='darkgreen'},
                Button{ 'Dismiss', id='Dismiss', background='red' },
            }
        },

    },

    onCreatedPage = function(self)
        self.characters = { }
        self.scroller = Scroller(self.ui.fdview)
        self.ui.fdview:setVisible(false)
        self.ui.pdfview:setVisible(false)
        self.ui.fdbuttons:setVisible(false)
        --self.view:open("Test - Farmland - Shooting Script.fdx")
        self.ui.characterpanel:setVisible(false)
        self.ui.fdview:setFontScale(settings.script.font)
        self:loadFile(settings.script.file, false)
        SettingNotify('names.character.name', self._onChangedCharacter, self)
        SettingNotify('names.character.color', self._onChangedCharacter, self)
        SettingNotify('script.show.actions', self._onChangedShowActions, self)
        SettingNotify('script.edit.enable', self._onChangedEditEnable, self)
        SettingNotify('script.font', self._onChangedFont, self)
        message.subscribe('footpedal.press', self._onFootPedal, self)
--      message.subscribe('footpedal.release', self._onFootPedal, self)
        self.ui.fdview.onResize = function()
            print('fdviewer:onResize')
            self.scroller:reset()
        end
        SettingNotify('scene.current',  function()
            print 'Scene changed! Resetting scroller'
            self.scroller:reset()
        end)
    end,

    onEnterPage = function(self)
        message.subscribe('transport.status', self.onTransportStatus, self)
    end,

    _allowChangeCharacter = function(self, setting, idx)
        local name = Setting('names.character.name'):value(idx)
        local color = Setting('names.character.color'):value(idx)
        self.ui.fdview:setColor(name, color)
    end,

    _onChangedCharacter = function(self, ...)
        if self._handleChangeCharacter then
            self:_handleChangeCharacter(...)
        end
    end,

    _onChangedFont = function(self, setting)
        self.ui.fdview:setFontScale( setting:value() )
        self:reloadFile()
    end,

    onClickLoad = function(self, c)
        --local filepath = widgets.chooseFile(self.widget, "Choose a script", "", "PDF files (*.pdf);;Final Draft files (*.fdx)")
        local filepath = widgets.chooseFile(self.widget, "Choose a script", "", "Final Draft/PDF files (*.fdx *.pdf)")
        print(filepath)
        if filepath then
            self:loadFile(filepath, true)
        end
    end,

    _onChangedShowActions = function(self, setting)
        self.ui.fdview:setVisibleForTag(self.ui.fdview.TAG_ACTION, settings.script.show.actions)
        self.scroller:reset()
    end,
    _onChangedEditEnable = function(self, setting)
        self.ui.fdview:setEdit(settings.script.edit.enable)
        self.ui.fdview:update()
        self.scroller:reset()
    end,
    onIndexCharacterCell = function(self, cell)
        return (cell.row-1)*cell.matrix.columns + cell.col
    end,

    onUpdateCharacterCell = function(self, c)
        local character = self.characters[c:index()]
        c:setText( character and character.name or '')
        c:setBackground( character and character.color)
    end,

    onClickAccept = function(self)
        self._handleChangeCharacter = nil

        local nameSetting = Setting('names.character.name')
        local colorSetting = Setting('names.character.color')
        nameSetting:clear()
        colorSetting:clear()
        for idx,character in ipairs(self.characters) do
            nameSetting:setValue(character.name, idx)
            colorSetting:setValue(character.color, idx)
        end
        self.ui.characterpanel:setVisible(false)
        self:layout()

        self._handleChangeCharacter = self._allowChangeCharacter
    end,

    onClickDismiss = function(self)
        self.ui.characterpanel:setVisible(false)
        self:layout()
    end,

    _onFootPedal = function(self, msg, which)
        print('Script._onFootPedal', msg, which)
        if which == 0 then
            self:scriptScrollUp()
        elseif which == 1 then
            if self.scroller:isStopped() then
                self.scroller:gotoStart()
            end
        elseif which == 2 then
            self:scriptScrollDown();
        end
    end,

    onClickScrollUp = function(self)
        self:scriptScrollUp()
    end,
    onClickScrollDown = function(self)
        self:scriptScrollDown()
    end,
}

function P:onClickSetStart()
    self.scroller:setStart()
end

function P:onTransportStatus(msg, status)
    self.status = status
    if status == 'record' then
        self.scroller:start()
    else
        self.scroller:stop()
    end
    print('onTransportStatus',  self,  msg,  status)
end

function P:scriptScroll(down)
    local step = settings.script.scroll.step
    if not down then step = -step end

    self.scroller:scrollBy(step)
end

function P:scriptScrollUp()     self:scriptScroll(false)    end
function P:scriptScrollDown()   self:scriptScroll(true)     end

function P:loadFile(filepath, wantCharacters)
    if not filepath then
        return
    end

    self._handleChangeCharacter = nil
    self.filepath = filepath

    local fd, pdf = false, false
    if filepath:lower():find('.pdf$') then
        self.ui.pdfview:open(filepath)
        pdf = true
    elseif filepath:lower():find('.fdx$') then
        self.ui.fdview:open(filepath)
        fd = true
    end

    --  self.showActions:setVisible(fd)
    self.ui.fdview:setVisible(fd)
    self.ui.pdfview:setVisible(pdf)

    if fd and wantCharacters and #self.ui.fdview.characters > 0 then
        self.characters = self.ui.fdview.characters
        self.ui.characterpanel:setVisible(true)
    else
        self.characters = { }
        self.ui.characterpanel:setVisible(false)
        self._handleChangeCharacter = self._allowChangeCharacter
    end

    if fd then
        self.ui.fdview:setVisibleForTag(self.ui.fdview.TAG_ACTION, settings.script.show.actions)
    end

    self.ui.fdbuttons:setVisible(fd)
    self:layout()
    self:update()
    settings.script.file = filepath
    self.scroller:reset()
end

function P:reloadFile()
    if self.filepath then
        self:loadFile(self.filepath, false)
    end
end

