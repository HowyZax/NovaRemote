assert(_TARGET == 'qt')

--local widgets = require 'widget'

require 'ui.runtime.qt.control'
require 'ui.runtime.qt.text'
require 'ui.runtime.qt.textedit'
require 'ui.runtime.qt.button'
require 'ui.runtime.qt.matrix'
require 'ui.runtime.qt.page'
require 'ui.runtime.qt.pagestack'
local history = require 'ui.runtime.qt.pagehistory'
local serialize = require 'serialize'
local message = require 'message'
local config = require 'config'
local usePageTabs = config.tablet or config.standalone
if usePageTabs then
    require 'ui.runtime.qt.pagetabs'
else
    assert(not PageTabs)
end
--require 'pl'
local file = require 'pl.file'

-------------------------------------------------------------------

_UI_CFG = _UI_CFG or config.utils.filename('ui')

-------------------------------------------------------------------
--require 'settings.client'

local updateCounter = 0
local serialRxCounter = 0
local serialTxCounter = 0
local serialRxErrorCounter = 0
local serialRxPacketCounter = 0
local timer = ElapsedTimer.create()
--local client = require 'client'

local status = {
    fps = 0,
    memory = 0,
    update = false,
}

local profiler = nil --require 'ShinyLua'
-- local profiler = require 'ShinyLua'

message.subscribe('serial.receive', function(msg, rx)
    serialRxCounter = serialRxCounter + #rx
end )
message.subscribe('serial.receive.errors', function(msg, n)
    serialRxErrorCounter = serialRxErrorCounter + n
end )
message.subscribe('serial.transmit', function(msg, tx)
    serialTxCounter = serialTxCounter + #tx
end )
message.subscribe('serial.receive.packets', function(msg, n)
    serialRxPacketCounter = serialRxPacketCounter + n
end )

local system = { }

local pulse = Timer.create()
pulse:setTimeout(20)
function pulse:onTimeout()
    system.onPulse()
end

function system:onStartup()

    message.send('system.startup')

    if not config.nomad then
        if mainWindow.startFootPedal then
            mainWindow:startFootPedal()
        end
    end

    local setting = SettingFind('ui.update.period', false)
    pulse:setTimeout(setting and setting:value() or 20)
    pulse:start()
    if setting then
        SettingNotify('ui.update.period', function(setting)
            pulse:setTimeout(setting:value())
        end)
    end
    if profiler then profiler.start()  end
--  profiler.damping(0.01)
--  if client then client:send('sendAll') end

end

function system:onShutdown()
    print('Received MSG_SHUTDOWN')
--  saveWindowPositions()
    if wantSettings or config.autosave then
        SettingsSave()
    end
    if profiler then
        profiler.stop()
        profiler.update()
        --profiler.output('profile.txt')
        print 'Start writing profile'
        file.write('profile.txt', profiler.flat_string())
        print 'Finish writing profile'
    end
    message.send('system.shutdown')
end

function system:onPulse()
--  client:update()
    local standalone = config.standalone

    --MetersUpdate()
    message.send('timer.tick')
    if not standalone then
        message.send('meters.update')
        message.send('timecode.update')
    end
    --collectgarbage()
    updateCounter = updateCounter + 1
    local elapsed = timer:elapsed()
    if elapsed >= 1000 then
        message.send('timer.second')
        status.memory = math.floor(collectgarbage('count'))
        status.fps = (updateCounter * 1000) / elapsed
        updateCounter = 0
        timer:start()
        collectgarbage()
        --status.toggle = status.toggle == 0 and 1 or 0
        status.update = true
    end

    if not standalone and status.update then
        mainWindow:setText( 'V' .. config.version .. ' Memory: ' .. status.memory .. 'KB, Update rate: ' .. math.floor(status.fps) .. ' fps, Serial Rx: ' ..
                                serialRxCounter .. ' bytes, ' ..
                                serialRxErrorCounter .. ' errors, ' ..
                                serialRxPacketCounter .. ' packets, Serial Tx: '  ..
                                serialTxCounter .. ' bytes')
        status.update = false
    end

    io.flush()
end

function system:onFootPress(which)
    message.send('footpedal.press', which)
end

function system:onFootRelease(which, which)
    message.send('footpedal.release', which)
end

-------------------------------------------------------------------
local function _MsgHandler(target, method, ...)
    if target then
        local owner = target:owner()
        if owner then
            return despatch(owner, method, ...)
        else
            --print ('SEND TO TARGET: ', target.__classname,  method, ...)
            return despatch(target, method, ...)
        end
    else
        target = system
        if target[method] then
            return target[method](target, ...)
        end
    end
end

if profiler then
    function MsgHandler(target,  method, ...)
        if profiler then profiler.start()end
        local result = _MsgHandler(target, method, ...)
        if profiler then profiler.stop() end
        return result
    end
else
    MsgHandler = _MsgHandler
end

assert(rootWidget or _STANDALONE)
if usePageTabs then
    if rootWidget.showTabBar then
        rootWidget:showTabBar(false)
    end
    PageTabs.CreateDefault(rootWidget)
    if _STANDALONE then
        assert(PageTabs.Default)
        mainWindow:setCentralWidget(PageTabs.Default.widget)
    end
end
-------------------------------------------------------------------
local TabWidgetFind

if not usePageTabs then function TabWidgetFind(page)
    if page.parent then
        assert(page.widget)
        assert(page.parent.widget)
        for _,tabWidget in ipairs(mainWindow:tabWidgets()) do
            if tabWidget:hasWidget(page.parent.widget) then
                return tabWidget
            end
        end
    end
  end
else function TabWidgetFind(page)
    if page.parent then
        assert(page.widget)
        assert(page.parent.widget)
        assert(isinstance(page.parent, PageStack))
        for _,tabWidget in ipairs(PageTabs.ALL) do
            if tabWidget:hasStack(page.parent) then
                return tabWidget
            end
        end
    end
  end
end

-------------------------------------------------------------------
-- Function called to activate a regular top-level page and, if necessary,
-- create its contained PageStack and assign it to a tab in a TabWidget.
--
-- rootWidget is the TabWidget for the MainWindow (ie. not detached windows).
-- By default pages are added to this TabWidget as a new Tab. The page added
-- inside a PageStack object to allow popups etc. to be nested.
--
-- When loading the UI configuration, RootTabWidget is set to the currently
-- loading TabWidget and pages are added to RootTabWidget instead of RootWidget.
--
-- TabWidget is a contained for widgets. Each widget is assigned to a tab.
-- In this case, each tab contains a PageStack[QWidget] which in turn contains
-- a Page[QWidget].

local RootTabWidget  -- Temporary 'current widget' while loading from ui.cfg

function RootSetPage(page)
    print ('RootSetPage', page, page.name)
    assert(not page.modal)
    assert(not page.floating)
    assert(not page.popup)
    assert(page.widget)
--  assert(not page.parent)
    local tabWidget = TabWidgetFind(page)
    if not tabWidget then
        if usePageTabs then
            tabWidget = PageTabs.Default
        else
            tabWidget = RootTabWidget or rootWidget
        end
    end

    local stack = page.parent
    if not stack then
        stack = PageStack{ }
        stack:create()
        stack:enter(page)
    end
    if usePageTabs then
        tabWidget:insert(stack)
    else
        tabWidget:insertWidget(stack.widget)
    end
    if not RootTabWidget then
        history.add(page)
    end

end

-------------------------------------------------------------------
function setFrames(frames)
    Page.wantFrames = frames
    if config.tablet then return end -- For tablet, don't touch the main window
    if usePageTabs then return end -- Not yet supported for PageTabs-based programs

    for _,tabWidget in ipairs(mainWindow:tabWidgets()) do
        if tabWidget ~= rootWidget then
            tabWidget:setFrame(frames)
        end
    end
    mainWindow:setFrame(frames)
end

function ToggleWindowFrames()
    Page.wantFrames = not Page.wantFrames
--  saveWindowPositions()
    setFrames(Page.wantFrames)
end

-------------------------------------------------------------------
local function floatingPositions()
    local tbl = { }
    for _,page in pairs(pages) do
        if page.floating and page.savedGeometry then
            local t = { }
            t.x, t.y, t.w, t.h = page.savedGeometry:unpack()
            t.name = page.name
            table.insert(tbl, t)
        end
    end
    return tbl
end

function saveWindowPositions()
    print('saveWindowPositions')
    local tbl = { }

    local t = { }
    if mainWindow:isMaximized() then t.maximized = true
    else t.x, t.y, t.w, t.h = mainWindow:geometry()
    end
    --table.insert(tbl,  t)

    tbl.window = t
    tbl.floating = floatingPositions()
    tbl.frames = Page.wantFrames

    if usePageTabs then
        local function saveTab(t)
            local tab = { }
            tab.x, tab.y, tab.w, tab.h = t:geometry():unpack()
            if not t:isVisible() then tab.visible = false end
            for _,stack in ipairs(t) do
                local page = stack and stack[1]
                if page and not page.modal and not page.floating and not page.popup then
                    local _t = { name=page.name }
                    if #t > 1 and stack==t.activeStack then _t.active = true end
                    table.insert(tab, _t);
                end
            end
            if #tab > 0 then
                tbl.tabs = tbl.tabs or { }
                table.insert(tbl.tabs, tab)
            end
        end
        if not config.standalone then
            saveTab(PageTabs.Default)
        else
            assert(PageTabs.ALL[1] == PageTabs.Default)
            for _,tab in ipairs(PageTabs.ALL) do
                saveTab(tab)
            end
        end

    else
        for i, tabWidget in ipairs(mainWindow:tabWidgets()) do
            local tab = { }
            tab.x, tab.y, tab.w, tab.h = tabWidget:geometry()

            local wpcurrent = tabWidget:currentWidget()
    --      if wpcurrent then tab.active = wpcurrent.name end

            for _,wp in ipairs(tabWidget:widgets()) do
                local stack = wp:owner()
                local page = stack and stack[1]
                if page and not page.modal and not page.floating and not page.popup then
                    local t = { }
                    t.name = page.name
                    if wpcurrent == wp then t.active = true end
                    table.insert(tab,  t);
                end
            end
            if #tab > 0 then
                tbl.tabs = tbl.tabs or { }
                table.insert(tbl.tabs, tab)
            end
        end
    end

    print ('------------ SAVE ------------')
    tprint(tbl)
    print ('------------ END SAVE --------')
    --[[
    Preferences.write('ui/layout', serialize.pack(tbl))
    --]]
    file.write(_UI_CFG, serialize.packEx(tbl))

end

local framesPending
local framesPendingCountdown = 0
message.subscribe('timer.second', function()
    if framesPending ~= nil then
        if framesPendingCountdown > 0 then
            framesPendingCountdown = framesPendingCountdown - 1
            if framesPendingCountdown == 0 then
                setFrames(framesPending)
                framesPending = nil
            end
        end
    end
end)

if config.standalone then function restoreWindowPositions(activePage, factory)
    local s = file.read(_UI_CFG) or ''
    local newPageTabs = { }
    print(type(s))
    print(s)
    assert(not factory or type(factory) == 'function')
    local status, tbl = serialize.unpack(s)
    if status and type(tbl) == 'table' then
        if not config.fullscreen then
            if tbl.window.maximized then mainWindow:setMaximized()
            else mainWindow:setGeometry(tbl.window.x, tbl.window.y, tbl.window.w, tbl.window.h) end
        end
        for i,tab in ipairs(tbl.tabs or { }) do
            local current
            local r
            if tab.x and tab.y and tab.w and tab.h then
                r = Rect(tab.x, tab.y, tab.w, tab.h)
            end

            local pagetabs

            if i == 1 then
                pagetabs = PageTabs.Default
            else
                pagetabs = PageTabs{ }
                pagetabs:create()
--              pagetabs:setVisible( tab.visible == nil or tab.visible )
                newPageTabs[pagetabs] = tab.visible == nil or tab.visible
            end

--          table.insert(newPageTabs, pagetabs)

--          newpage:setParent(stack)
--          stack:enter(newpage)
            if i ~= 1 and r then
                pagetabs:setGeometry(r)
            end

            for _,t in ipairs(tab) do
                local page = pages[t.name]
                if not page and factory then
                    page = factory(t.name)
                end
                if page then pagetabs:insert(page) end
--              if page and not page.modal and not page.floating and not page.popup then
--                  Page.Enter(t.name)
--                  if not usePageTabs then
--                      if t.active then activePage = page; current = page; end
--                  end
--                  assert(page.widget)
--                  assert(page.parent)
--                  assert(page.parent.widget)
--              end
            end
--          pagetabs:setVisible()
        end
        for p,visible in pairs(newPageTabs) do
            p:setVisible(visible)
        end
        for _,t in ipairs(tbl.floating or { }) do
            local page = pages[t.name]
            if page and page.floating then
                page.savedGeometry = Rect(t.x, t.y, t.w, t.h)
                page:setGeometry(page.savedGeometry)
            end
        end

        framesPending = tbl.frames
        framesPendingCountdown = 3

    else
        return false
    end

    -- Activate any pages that have not yet been assigned to a TabWidget tab.
    -- These will be pages that are defined in the application but are not
    -- stored in the configuration file (e.g. due to newly defined page(s),
    -- renamed pages or an edited ui.cfg file).
    for _,page in pairs(pages) do
        if not page.modal and not page.floating and not page.popup and not TabWidgetFind(page) then
            Page.Enter(page.name)
        end
    end

    -- Finally make the designated inital page active
    if activePage then activePage:enter() end

    return true

end else function restoreWindowPositions(activePage)
    local s = file.read(_UI_CFG) or ''
--  local s = Preferences.read('ui/layout')
    print(s)
    print(type(s))
    local status, tbl = serialize.unpack(s)
    if status and type(tbl) == 'table' then
        if not config.fullscreen then
            if tbl.window.maximized then mainWindow:setMaximized()
            else mainWindow:setGeometry(tbl.window.x, tbl.window.y, tbl.window.w, tbl.window.h) end
        end
        if tbl.tabs then
            for i,tab in ipairs(tbl.tabs) do
                local current
                local r
                if tab.x and tab.y and tab.w and tab.h then r = Rect(tab.x, tab.y, tab.w, tab.h) end
                local tabWidget = (i == 1 or usePageTabs) and rootWidget or widgets.create(nil, 'tabbed', mainWindow)
                RootTabWidget = tabWidget   -- Fudge to load to correct tabWidget
                if not config.tablet and r then
                    tabWidget:setGeometry(r:unpack())
                end
                for _,t in ipairs(tab) do
                    local page = pages[t.name]
                    if page and not page.modal and not page.floating and not page.popup then
                        Page.Enter(t.name)
                        if not config.tablet then
                            if t.active then activePage = page; current = page; end
                        end
                        assert(page.widget)
                        assert(page.parent)
                        assert(page.parent.widget)
                    end
                end
                if current then tabWidget:setCurrentWidget(current.parent.widget) end
                tabWidget:show()
            end
        end
        if tbl.floating then
            for _,t in ipairs(tbl.floating) do
                local page = pages[t.name]
                if page and page.floating then
                    page.savedGeometry = Rect(t.x, t.y, t.w, t.h)
                    page:setGeometry(page.savedGeometry)
                end
            end
        end

        framesPending = tbl.frames
        framesPendingCountdown = 3

    end

    -- Activate any pages that have not yet been assigned to a TabWidget tab.
    -- These will be pages that are defined in the application but are not
    -- stored in the configuration file (e.g. due to newly defined page(s),
    -- renamed pages or an edited ui.cfg file).
    for _,page in pairs(pages) do
        if not page.modal and not page.floating and not page.popup and not TabWidgetFind(page) then
            Page.Enter(page.name)
        end
    end
    RootTabWidget = nil

    -- Finally make the designated inital page active
    if activePage then activePage:enter() end
--  for _,tabWidget in ipairs(mainWindow:tabWidgets()) do
--    tabWidget:show()
--  end
end end

-------------------------------------------------------------------
-- Resize/Move handlers
--
-- Install hooks for resize/move events depending on the configuration
-- Method onResize() is only used to respond to resize events for top-level
-- widgets that are resized by the O/S.
-- [Alternative approach is to handle the event for all widgets and use
-- the event handler to trigger a layout of child controls. However this
-- would not work for Lua controls that don't have an associated widget,
-- e.g. non-visual Panel].
--
-- PageStack is a top-level widget except for config.tablet when PageTabs
-- is the top-level widget.
--
-- Page is a top-level widget only when floating or modal, otherwise it
-- is contained within a PageStack
--
if not usePageTabs then
    function PageStack:onResize(w, h)
        self:layout( Rect(0,0,w,h) )
    end
else
    function PageTabs:onResize(w, h)
        self:layout( Rect(0, 0, w, h ) )
    end
end

function Page:saveGeometry()
    if not self.widget:isAnimating() then
        self.savedGeometry = self:geometry()
    end
end

function Page:onResize(w, h)
    -- Method called when underlying widget is resized by the
    -- O/S window manager. Calls layout manager to recursively
    -- position child objects relative to its parent.
    --
    -- This should only do anything when the Page is a top-level
    -- window (floating or modal). Regular pages are contained within
    -- other controls that receive onResize and treat a Page as
    -- a regular child control for layout.

    print('Page:onResize', self.name, w, h, self:geometry())
    if self.floating or self.modal then
        self:saveGeometry()
        self:layout( Rect(0, 0, w, h) )
    end
end

function Page:onMove(x, y)
    -- Method called when underlying widget is moved by
    -- the O/S window manager. Only relevant when Page is
    -- a top-level window (floating or modal).
--  print('Page:move',  self.name, x, y, self:geometry(), self.widget:isAnimating())
    if self.floating or self.modal then
        self:saveGeometry()
    end
end

