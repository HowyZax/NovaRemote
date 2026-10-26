--{{{ Page:create
--{{{ Page:create
local keys = widgets.keys

-- TODO: use parent in Page:create()
-- TODO: make Page a subclass of control?
function Page:create(parent)

    self:fixupPreCreate()

    if _TARGET=='qt' then
        parent = parent or rootWidget
    end

    local w
    if      self.modal      then w = widgets.create(self, 'dialog', parent)
    elseif  self.floating   then w = widgets.create(self, 'widget', nil)
    elseif  self.popup      then w = widgets.create(self, 'widget', parent)
    else                         w = widgets.create(self, 'page', parent) end

    assert(w)
    w:hide()

    if self.floating then w:setFloating() end
    if w.setTitle and (self.title or self.name) then w:setTitle(self.title or self.name) end
    print('Page:create', w.__classname, self.name)
    w:setName(self.name or '')
    super(Page).createChildren(self, self.widget)

    for _,s in ipairs(self.depends or {}) do
        local method = 'onChanged'
        for word in s:gmatch('%w+') do
            method = method .. word:gsub('^%l', string.upper)
        end
        SettingNotify(s, self[method] or self.update, self)
    end

    self:perform('onCreated')

    self:fixupPostCreate()
    return self.widget
end
--}}}

--}}}

function Page.Enter(page, ...)
    if type(page) == 'string' then page = pages[page] end
    assert(page,  'Page.Enter ' .. tostring(page) .. ' requires a valid page')
    assert(isinstance(page, Page))
    print('GOTO PAGE', page.name, page.widget)
    page:enter(...)
end

function Page.Exists(page)
    if type(page) == 'string' then page = pages[page] end
    return isinstance(page, Page)
end

function Page:fixupPreCreate()
    local page = self
    self:assignParent() -- ensure controls have an assigned parent field
    self:traverse(
        function(o)
            if o.delegate then
                -- o.delegate = DelegateFind(o.delegate)
            elseif page.delegate then
                o.delegate = page.delegate
            end
        end
        )
    -- TODO: use Control.buildUITable
    if not self.ui then
        self.ui = { }
        local function buildui(o, recurse)
            if o.id then
                local k = o.id:lower()
                self.ui[k] = self.ui[k] or o
            end
            if recurse then
                for _,c in ipairs(o) do
                    buildui(c, not c.compound)
                end
             end
        end
        buildui(self, true)
--[[        
        self:traverse(
            function(o)
                if o.id and not o.compound then
                    local k = o.id:lower()
                    self.ui[k] = self.ui[k] or o
                end
            end
        )
--]]
    end
end

function Page:fixupPostCreate()
    -- do nothing
end


function Page:onClick()
    self:done()
    --Page.Exit()
end

function Page:onKeyPress(...)
    --print ('Page:onKeyPress', ...)
    --[[
    if key == keys.KEY_ESCAPE then
        self:done()
    end
    --]]
end


