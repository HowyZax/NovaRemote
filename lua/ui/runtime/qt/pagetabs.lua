-- PageTabs is a top-level control that acts as a container for a
-- tab-bar/buttons and multiple PageStack controls, one of which
-- is active. The PageStack controls containing Pages that can be
-- nested. Typically a PageStack has one page which has the main
-- content displayed to the user, but nested pages allow for overlays
-- and popups.

local history = require 'ui.runtime.qt.pagehistory'
local config = require 'config'

PageTabs = Panel:subclass('PageTabs')
PageTabs.ALL = { }
--PageTabs:attributes{ '[]' }
PageTabs.visual = true
PageTabs.margin = 0
PageTabs.style = 'default'

function PageTabs:__init(items)
    super(PageTabs).__init(self, items)
    table.insert(PageTabs.ALL, self)
    self.stacks = { }
    self.activeStack = nil
    self.visible = false
end

function PageTabs:create(parentWidget)
    local w = super(PageTabs).create(self, parentWidget)
    --w:setMsgFilter('draw', false)
    return w
end

function PageTabs:hasStack(stack)
    assert(isinstance(stack, PageStack))
    for _,v in ipairs(self.stacks) do
        if v == stack then return true end
    end
    return false
end

function PageTabs:insertStack(stack)
    assert(isinstance(stack, PageStack))
    if not self:hasStack(stack) then
        table.insert(self.stacks, stack)
        -- TODO: remove from other stack
        assert(not isinstance(stack.parent, PageTabs), 'Need to remove stack from other tab!')
    end

    if stack ~= self.activeStack then
        if self.activeStack then
            assert(self[#self] == self.activeStack)
            self.activeStack.widget:hide()
            local removed = table.remove(self)
            assert(removed == self.activeStack)
            self.activeStack = nil
        end
        table.insert(self, stack)

        self:showHeader( not stack:wantFullscreen() )
        self:setTitle(self.stacks[1].title or '') --stack.title or '')
        stack:setParent(self)
        self:layout( self:rect() )
        stack:setVisible(true)
        self.activeStack = stack
    end
end

function PageTabs:insertPage(page)
    assert( isinstance(page, Page) )
    for _,stack in ipairs(self.stacks) do
        if stack:contains(page) then
            stack:enter(page)
            return
        end
    end
    local stack = PageStack{ }
    stack:create()
    self:insertStack(stack)
    stack:enter(page)
    self:setTitle(stack.title or '')
end

function PageTabs:insert(o)
    if isinstance(o, PageStack) then self:insertStack(o)
    elseif isinstance(o, Page) then self:insertPage(o)
    else
        error('PageTabs: Cannot insert object')
    end
end

function PageTabs:showHeader(state)
    if self.header then
        self.header:setVisible(state)
    end
end

function PageTabs.CreateDefault(parent)
    assert(not PageTabs.Default)            -- Only do this once
    local panel
    if config.standalone then
        panel = Panel {
            h = 0, --0.1,
            rows = 1,
            background = 'darkgray',
            style = 'default',
            margin = 0, --0.02,
            id = 'Header',
            --Button { graphic='home', w=0.12, background=0x555555 },
        }
    else
        panel = Panel {
            h = 0.1,
            rows = 1,
            background = 'darkgray',
            style = 'default',
            margin = 0, --0.02,
            id = 'Header',
            Button { graphic='home',  w=0.12, background=0x555555, page='Home' },
            Button { graphic='reply', w=0.12, background=0x555555, onClick = function() history.gotoPrevious() end },
            Button { "Output|Mix",            background=0x555555, color='lightgray', page='OutputMix' },
            Button { "Track|Mix",             background=0x555555, color='lightgray', page='TrackMix' },
            Button { "Record|Enables",        background=0x555555, color='lightgray', page='RecordChannels' },
            Button { "Input|Config",          background=0x555555, color='lightgray', page='InputConfigure' },
            Button { "Headphone|Mix",         background=0x555555, color='lightgray', page='HeadphoneMix' }, 
            Button { "Main|Menu",             background=0x555555, color='lightgray', page='MainMenu' },
        }
    end
    PageTabs.Default = PageTabs {           -- Create container for stacks
        columns = 1,
        panel
    }

    PageTabs.Default.header = PageTabs.Default:findById('Header')
    assert(PageTabs.Default.header)
    PageTabs.Default:create(parent)     -- Create underlying widget as child of MainWindow.rootWidget
    assert(PageTabs.Default.widget)
    PageTabs.Default.widget:setTitle('PageTabs')
    if parent and isinstance(parent, 'TabWidget') then
        parent:insertWidget(PageTabs.Default.widget)
    end
    --PageTabs.Default.widget:show()                 -- Show the container
    return PageTabs.Default
end

