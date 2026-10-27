local config = require 'config'

local function string_increment(s)
    local str = ''
    local done = false
    local nc = '1'
    for ch in s:reverse():gmatch('.') do
        if not done then
            if     ch == 'z' then ch, nc = 'a','a'
            elseif ch == 'Z' then ch, nc = 'A','A'
            elseif ch == '9' then ch, nc = '0','1'
            else
                ch = string.char( string.byte(ch)+ 1 )
                done = true
            end
        end
        str = str .. ch
    end
    if not done then str = str .. nc end
    str = str:reverse()
    --print (s, '->', str)
    return str
end

local function string_split(s)
    local str = s:reverse()
    for _,re in ipairs{ '^(%s*)(%d+)(.*)',
                        '^(%s*)(%a+)(.*)' } do
        local x,y,z = str:match(re)
        x = x or ''; y = y or ''; z = z or ''
        if #y > 0 then
            return z:reverse(), y:reverse(), x:reverse()
        end
    end
    return s, '', ''
end

local function scenetake_increment(s)
    local x,y,z = string_split(s)
    local str = x .. string_increment(y) .. z
    print( string.format('scene_increment [%s] -> [%s][%s][%s] = [%s]', s, x,y,z, str) )
    return str
end
local scene_increment = scenetake_increment
local take_increment = scenetake_increment

local _M = Page {
    name = 'SceneTake',
    title = "Scene Take",
    delegate = true,
    depends = { 'metadata.continuity', 'metadata.history' },
    Panel {
        columns = 1,
        -- Continuity
        Panel {
            background = true,
            rows = 1,
            margin = 0.02,
            Panel {
                w = 0.75,
                columns = { 0.2, 0.4, 0.4 },
                Button { "Continuity", setting='metadata.continuity' },
                Text { fit={10}, setting='scene.continuity', id='SceneContinuity', readonly=true },
                Text { fit={10}, setting='take.continuity', id='TakeContinuity', readonly=true },
            },
            Button { 'Accept', id='Accept', font=0.7 },
        },
        Spacer { h=0.01 },
        -- Next Scene/Take
        Panel {
            background = true,
            rows = 1,
            margin = 0.02,
            Panel { -- TODO:
                w = 0.75,
                columns = { 0.2, 0.4, 0.4 },
                Label { "NEXT", font=0.6, align='center' },
                Text { id='Scene', setting='scene.next', fit={10}, tag=1 },
                Text { id='Take', setting='take.next', fit={10}, tag=1 },
            },
            Panel {
                rows = 1,
                Button { 'Inc|Scene', fit=5, id='IncScene' },
                Button { 'Inc|Take',  fit=5, id='IncTake' },
            },
        },
        Spacer { h=0.01 },
        -- History
        Panel {
            background = true,
            h = 0.6,
            rows = 1,
            margin = 0.02,
            List {
                w = 0.75,
                rows = 6,
                columns = { 0.2, 0.4, 0.4 },
                id = 'History',
                reversed = true,
                headerFactory = function(self)
                    return  Label { "SEG",   fit=5, align='center' },
                            Label { "SCENE", fit=5, align='center' },
                            Label { "TAKE",  fit=5, align='center' }
                end,
                rowFactory = function(self, row)
                    return  Text { id='IndexHistory', tag=row, fit=5, align='center', margin=1 },
                            Text { id='SceneHistory', tag=row, fit={10}, margin=1 },
                            Text { id='TakeHistory',  tag=row, fit={10}, margin=1 }
                end,
                onItemCount = function(self)
                    return SettingFind('metadata.history'):count()
                end,
            },
            Panel {
                columns = 1,
--[[  the DEL and CLR buttons do bad things!
                    Panel {
                    h = 0.1,
                    rows = 1,                    
                    conditional = config.debug,
                    Button { 'Add', id='AddTake' },
                    Button { 'Del', id='RemoveTake' },
                    Button { 'Clr', id='ClearTakes' },
                },
--]]
                Button { "Last|Segment", id='LastSegment' },
                Panel {
                    rows = 1,
                    Button { graphic='up', id='DecSegment' },
                    Button { graphic='down', id='IncSegment' },
                },
                Button { "Enter|Segment", id='EnterSegment'},
            },
        },
    },

    onCreatedPage = function(self, page)
        self.segment = 1
        self.list = self.ui.history
        SettingNotify('scene.continuity',    self._onChangedSceneContinuity, self)
        SettingNotify('take.continuity',     self._onChangedTakeContinuity, self)
        SettingNotify('metadata.continuity', self._onChangedContinuityAccept, self)
    end,
    onClickIncSegment = function(self)
        self.list:scrollBy(1)
    end,
    onClickDecSegment = function(self)
        self.list:scrollBy(-1)
    end,
    onClickLastSegment = function(self)
        self.list:scrollToTop()
    end,
    onClickEnterSegment = function(self, c)
        local editor = Page.Find('NumberEditor')
        editor.delegate:show{
            text='',
            callback=function(text) self.list:scrollTo( tonumber(text) ) end,
            prompt="Enter Segment",
            maxchars=4,
            control=c
        }
    end,

    onClickIncScene = function(self)
        settings.scene.next = scene_increment(settings.scene.next)
    end,
    onClickIncTake = function(self)
        settings.take.next = take_increment(settings.take.next)
    end,

    _onChangedSceneContinuity = function(self)
        local background = settings.metadata.continuity == 0 and 'green' or nil
        self.ui.scenecontinuity.background = background
        self.ui.accept.background = background
        self:update()
    end,
    _onChangedTakeContinuity = function(self)
        local background = settings.metadata.continuity == 0 and 'green' or nil
        self.ui.takecontinuity.background = background
        self.ui.accept.background = background
        self:update()
    end,
    _clearContinuityHighlights = function(self)
        self.ui.scenecontinuity.background = nil
        self.ui.takecontinuity.background = nil
        self.ui.accept.background = nil
        self:update()
    end,

    _onChangedContinuityAccept = function(self)
        self:_clearContinuityHighlights()
    end,

    onClickAccept = function(self, c)
        settings.scene.next = settings.scene.continuity
        settings.take.next = settings.take.continuity
        self:_clearContinuityHighlights()
    end,

    onUpdateAccept = function(self, c)
        c:setEnabled( settings.metadata.continuity == 0 )
    end,

    -- Data access
--  onIndexSceneHistory = function(self, c)
--      return self.list:indexOfControl(c)
--  end,
--  onIndexTakeHistory = function(self, c)
--      return self.list:indexOfControl(c)
--  end,
    onUpdateIndexHistory = function(self, c)
        local i = self.list:indexOfControl(c)
        if i >= 1 and i <= self.list:count() then c:setText(i)
        else c:setText('')
        end
    end,
    onUpdateSceneHistory = function(self, c)
        local metadata = SettingReference('metadata.history', self.list:indexOfControl(c))
        c:setText(metadata.scene or '')
    end,
    onUpdateTakeHistory = function(self, c)
        local metadata = SettingReference('metadata.history', self.list:indexOfControl(c))
        c:setText(metadata.take or '')
    end,
    onClickSceneHistory = function(self, c)
        local metadata = SettingReference('metadata.history', self.list:indexOfControl(c))
        Page.Find('TextEditor').delegate:show{
            text=metadata.scene,
            callback=function(text) metadata.scene = text end,
            prompt='Enter Scene',
            maxchars=12,
            control=c
        }
    end,
    onClickTakeHistory = function(self, c)
        local metadata = SettingReference('metadata.history', self.list:indexOfControl(c))
        Page.Find('TextEditor').delegate:show{
            text=metadata.take,
            callback=function(text) metadata.take = text end,
            prompt='Enter Take',
            maxchars=12,
            control=c
        }
    end,
    onClickAddTake = function(self, c)
        local setting = SettingFind('metadata.history')
        local metadata = {
            scene = settings.scene.next,
            take = settings.take.next,
            note = settings.note.next,
        }
        setting:setValue(metadata, setting:count()+1)

        settings.take.next = take_increment(settings.take.current)
        self.list:scrollToTop()

    end,
    onClickClearTakes = function(self, c)
        SettingFind('metadata.history'):clear()
    end,
    onClickRemoveTake = function(self, c)
        local setting = SettingFind('metadata.history')
        setting:setValue(nil, setting:count())
    end,

}

local _M = Page {
    name = 'Note',
    delegate = true,
    depends = { 'metadata.history' },
    Panel {
        Panel {
            margin = 0,
            h = 0.8,
        background = true,
        Panel {
            h = 0.25,
            columns = { 0.1, 0.8, 0.1 },
            Label { "SEG", align='center', font=0.8 }, Label{ "NOTE", align='center', font=0.8 }, Spacer { },
            Label { "NEXT", align='center', font=0.8 }, Text { setting='note.next' }, Button { "PRESET", id='PresetCurrent', page='PresetNotes'},
        },
        List {
            rows = 6,
            columns = { 0.1, 0.8, 0.1 },
            id = 'History',
            reversed = true,
            rowFactory = function(self, row)
                return  Text { id='Index', tag=row, fit=3, align='center', margin=1 },
                        Text { id='Note', tag=row, fit={10}, font=0.8, margin=1 },
                        Button { 'PRESET', id='Preset', tag=row, padding=0.2, page='PresetNotes', margin=1 }
            end,
            onItemCount = function(self)
                return SettingFind('metadata.history'):count()
            end,
        },
    },
        Panel {
            rows = 1,
            Button { "Enter|Segment", id='EnterSegment' },
            Button { graphic='up', id='Up' },
            Button { graphic='down', id='Down' },
            Button { "Last|Segment", id='LastSegment'},
        },
    },

    onCreatedPage = function(self)
        self.list = self.ui.history
    end,
    onClickUp = function(self, c)
        self.list:scrollBy(-1)
    end,
    onClickDown = function(self, c)
        self.list:scrollBy(1)
    end,
    onUpdateIndex = function(self, c)
        local i = self.list:indexOfControl(c)
        if i >= 1 and i <= self.list:count() then c:setText(i)
        else c:setText('')
        end
    end,
    onUpdateNote = function(self, c)
        local i = self.list:indexOfControl(c)
        local metadata = SettingReference('metadata.history', i)
        c:setText(metadata.note or '')
    end,
    onIndexNote = function(self, c)
        return self.list:indexOfControl(c)
    end,
    onIndexPreset = function(self, c)
        return self.list:indexOfControl(c)
    end,
    onClickNote = function(self, c)
        local metadata = SettingReference('metadata.history', c:index())
        Page.Find('TextEditor').delegate:show{
            text=metadata.note,
            callback=function(text) metadata.note = text end,
            prompt='Enter Note',
            maxchars=32,
            control=c
        }
    end,
    onClickPreset = function(self, button)
        Page.Enter(button.page, self, function(text)
                local metadata = SettingReference('metadata.history', button:index())
                metadata.note = text
            end
        )
    end,
    onClickPresetCurrent = function(self, button)
        Page.Enter(button.page, self, function(text)
                SettingFind('note.next'):setValue(text)
            end
        )
    end,
    onClickEnterSegment = function(self, c)
        local editor = Page.Find('NumberEditor')
        editor.delegate:show{
            callback=function(text) self.list:scrollTo( tonumber(text) ) end,
            prompt="Enter Segment",
            maxchars=4,
            control=c
        }
    end,
    onClickLastSegment = function(self, c)
        self.list:scrollToTop()
    end,
}


SettingNotify('scene.continuity', function()
        if settings.metadata.continuity > 0 then
            settings.scene.next = settings.scene.continuity
        end
    end)

SettingNotify('take.continuity', function()
        if settings.metadata.continuity > 0 then
            settings.take.next = settings.take.continuity
        end
    end)

--SettingNotify('metadata.history', function(setting)
--        local metadata = SettingReference(setting, setting:count())
--        settings.scene.current = metadata.scene or '---'
--        settings.take.current = metadata.take or '---'
--        settings.note.current = metadata.note or '---'
--    end)


