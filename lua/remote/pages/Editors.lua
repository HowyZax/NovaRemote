-------------------------------------------------------------------
-- Common delegate class used for text, number and timecode edit
-- popup pages

local stringx = require 'pl.stringx'

require 'remote.pages.KeyboardEditor'

EditPageDelegate = Object:subclass('EditPageDelegate')
EditPageDelegate:attributes{ 'kind' }

local function EditorTitle(items)
    return Panel {
        rows = 1,
        h = 0.1,
        margin = 0,
        Label { items[1], id='Title', align='center', color='gold' },
        Button { w=0.15, background='darkred', graphic='times', id='Close' },
    }
end

Page {
    name = 'TextEditor',
    delegate = EditPageDelegate{ kind='string' },
    --modal = true,
    floating = true,
    columns = 1,
    --Title { 'Enter Text', id='Title', align='center' },
    EditorTitle{ 'Enter Text' },
    Panel {
--      y=0.2,
--      columns = 1,
        Panel {
            rows = 1,
            --Text { w=0.2, 'Enter text:', font='small', align='right', id='Prompt' },
            TextEdit { '<text here>', align='center', id='Control' },
--          Text { w=0.1, '32/32', align='right', id='Status', font='small' },
        },
        Keyboard{ h=0.8 },
    }
}
Page {
    name = 'NumberEditor',
    delegate = EditPageDelegate{ kind='number' },
    floating = true,
--  Title { 'Enter Number', id='Title', align='center', w=0.75 },
    columns = 1,
    EditorTitle{ 'Enter Number' },
    Panel {
        Panel {
            rows = 1,
            --Text { w=1/3, 'Enter number:', font='small', align='right', id='Prompt' },
            TextEdit { '<number here>', align='left', id='Control' },
        },
        Keypad{ h=0.8 },
    },
}
Page {
    name = 'TimecodeEditor',
    delegate = EditPageDelegate{ kind='timecode' },
    floating = true,
    columns = 1,
--  Title { 'Enter Timecode', id='Title', align='center', w=0.75 },
    EditorTitle{ 'Enter Timecode' },
    Panel {
--      y=0.2,
--      columns = 1,
        constrain = {1, 1.5},
        Panel {
            rows = 1,
            --Text { w=1/3, 'Enter timecode:', font='small', align='right', id='Prompt' },
            TimecodeEdit { '<text here>', constrain=6, align='left', id='Control', font='timecodeedit' },
        },
        KeypadTimecode{ h=0.8 },
    },
}
Page {
    name = 'UserbitsEditor',
    delegate = EditPageDelegate{ kind='userbits' },
    floating = true,
    columns = 1,
--  Title { 'Enter Userbits', id='Title', align='center', w=0.75 },
    EditorTitle { 'Enter Userbits' },
    Panel {
--      y=0.2,
--      columns = 1,
        constrain = {1, 1.5},
        Panel {
            rows = 1,
            --Text { w=1/3, 'Enter userbits:', font='small', align='right', id='Prompt' },
            UserbitsEdit { '<text here>', constrain=6, align='left', id='Control', font='timecodeedit' },
        },
        KeypadUserbits{ h=0.8 },
    },
}

function EditPageDelegate:onCreatedPage(page)
    self.page = page
    self.title = page.ui.title
    self.control = page.ui.control
    self.prompt = page.ui.prompt
    self.keyboard = page.ui.keyboard
    self.status = nil --page:findById('Status', false)
    self.editor = KeyboardEditor{
        keyboard = self.keyboard,   -- use the keyboard control on this page
        notify = self,              -- send KeyboardEditor notifications to this delegate
        buffer = self.control       -- use the control as a buffer, instead of KeyboardEditorBuffer
    }
end

function EditPageDelegate:onEnterPage(page)
    print(self.control.cursor)
    --assert(self.control.cursor == nil, 'Control has an unexpected cursor')
    self.control.cursor = self.editor.cursor
    if (self.kind == 'timecode' or self.kind == 'userbits') then
        self.control.widget:setCursor(0)
    end
    --!!self.editor:reset(self.target:text(), self.maxchars)
end

function EditPageDelegate:onExitPage(page)
    self.control.cursor = nil   -- Clear the cursor
end

function EditPageDelegate:onClickPage(page)
    return true     -- Don't close the dialog/target
end

function EditPageDelegate:textChanged(text, cursor)                     -- Required by KeyboardDelegate
    --self.control.cursor = cursor
    --self.control:setText(text)
    if self.status then
        self.status:setText(cursor .. '/' .. self.buffer.maxchars)
        self.status:update()
    end
    self.control:update()
end

function EditPageDelegate:completed(text)                       -- Required by KeyboardDelegate
    if text and self.callback then
        self.callback(text)
    end
    self.page:done()
end

function EditPageDelegate:onClickClose()
    self:completed(nil)
end

function EditPageDelegate:show(options) -- (text, callback, prompt, maxchars, control)
    local text = options.text or ''
    local callback = options.callback
    local prompt = options.prompt
    local setting = options.setting
    local control = options.control
    local maxchars = options.maxchars or 10

    if not prompt then
        if setting then
            prompt = 'Enter ' .. (setting.description or stringx.title( setting.name:gsub('%.', ' ')) )
        else
            prompt = ''
        end
    end

    assert(callback); assert(prompt); assert(maxchars)
    if self.prompt then self.prompt:setText(prompt .. ': ') end
    if self.title then self.title:setText(prompt or '') end
    self.callback = callback
    self.maxchars = maxchars
    local options = { }
    if control then
        local x,y = control.widget:globalPos()
        local r = control:geometry();
        r.x, r.y = x, y
        options.animateFrom = r
        options.animateTo = self.page.savedGeometry
    end

    if self.kind == 'string' or self.kind == 'number' then
        local mask = ''
        local align = 'center'
        if setting then
            print('setting.template', setting.template)
            if setting.template then mask = setting.template end
            if isinstance(setting, SettingNumber) or isinstance(setting, SettingText) then
                align = 'center'
            end
            print('setting.text', setting:value(),  setting:text(),  setting:rawtext(),  setting)
        end
        self.control.widget:setAlignment(align)
        self.control.widget:setInputMask(mask)
    end

    self.editor:reset(text or '', self.maxchars)

    self.page:enter(options)
--  self:onEnterPage(self.page)
    self.control.widget:setFocus()
    self.control.cursor = self.editor.cursor

    --self.page:update()
end

function EditPageDelegate:onKeyPressControl(control, ...)
    self.editor:onKeyPress(...)
end


