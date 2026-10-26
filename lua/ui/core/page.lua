
--==================================================================
-- Page class
--
-- Represents a single user interface page (or screen)
--
-- A page has a message handler and a list of controls
--==================================================================

Page = Control:subclass('Page')      -- Page class
Page.margin = 0
Page:attributes{ '[]', 'name', 'title', 'rows', 'columns', 'onEnter', 'onExit', 'modal', 'floating', 'popup', 'exclusive', 'depends', 'constrain' }
Page.wantFrames = true

pages = {}          -- table of all defined pages

-- Find a page by name
function Page.Find(name)
    return pages[name]
end

-- Constructor
function Page:__init(items)
--  print('**** Page:__init ****')
    --super(self, '__init',  items)
    Object.__init(self, items)
    if items == nil then return end -- Required for derived classes
    -- Copy attributes from initializer table
    --for k,v in pairs(items) do self[k] = v end
    -- Automatically synthesize a name from title
    -- e.g. title="Main Menu" gives page name "MainMenu"
    if not self.name and self.title then
        self.name = self.title:gsub(' ','')
    end

    self.id = self.id or 'Page'

    if not self.name then
        print('Creating un-named/temporary page')
    else
        assert(self.name)
        assert(not pages[self.name], 'Duplicate page name ' .. self.name)
        pages[self.name] = self
    end


    -- Add Title control if 'title' specified. Center the title
    -- if this looks like a simple page with single panel (e.g Main Menu)
    if self.title and self.popup then
        local align = 'center'
--      if #self == 1 and isinstance(self[1], Panel) then align = 'center' end
        table.insert(self, 1, Title { self.title, align=align } )
    end
    -- Add Text control if 'footer' specified
    if self.footer then
        table.insert(self, Text{ self.footer, x=Lcd.right-1,
                y=Lcd.bottom-Fonts.medium.height+1, align='right' } )
    end

    if self.modal then assert(not self.name, 'Unexpected name for modal page: ' .. (self.name or '')) end
end

function Page:onPaint(painter)
    if self.widget and self.widget:state() == 'disabled' then
        painter:gradient('gray', '', 'disabled')
--      painter:gradient('darkgray')
    end
end

