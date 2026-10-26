
PageStack = Control:subclass('PageStack')
PageStack.ALL = { }
function PageStack:__init(items)
    super(PageStack).__init(self, items)
    table.insert(PageStack.ALL, self)
    self.visible = false        -- Start not visible
end

function PageStack:create()
    local w = super(PageStack).create(self, nil, false)
    return w
end

function PageStack:onPaint()
    return false
end

function PageStack:contains(page)
    for _,p in ipairs(self) do
        if p == page then return true end
    end
    return false
end

function PageStack.Find(page)
    for _,stack in ipairs(PageStack.ALL) do
        for _,p in ipairs(stack) do
            if p == page then return stack end
        end
    end
end

function PageStack:layoutPage(rect, page)
    -- Layout a single page inside the PageStack widget - used
    -- when pushing a new page on the stack
    local r = Rect(0,0,rect.w,rect.h)
    local layout = self.grid and GridLayout(r) or Layout(r)
    layout:layoutChildren(self, { page } )     -- Layout the children
end

function PageStack:layout(rect)
    -- Method to layout the pages in the stack. Each page is
    -- positioned relative to the PageStack.
    -- Overrides Control:layout()
    for _,page in ipairs(self) do
        self:layoutPage(rect, page)
    end
end

function PageStack:updateVisibility()
    local visible, enabled = true, true
    for i = #self,1,-1 do
        self[i].widget:setVisible(visible)
        self[i].widget:setEnabled(enabled)
        if self[i].exclusive then visible = false end
        enabled = false
    end
end

function PageStack:_push(page, ...)
    assert(not page.parent or page.parent == self)
    assert(not self:contains(page))

    page:setParent(self)
    table.insert(self, page)

    self:layoutPage( self:rect(), page )

    page:perform('onEnter', ...)
    page:update()
end

function PageStack:_pop()
    assert(#self > 1)
    local page = table.remove(self)
    page.widget:setVisible(false)
    if page.deleteOnExit then
        page:setParent(nil)
        page.widget:destroy()
        page.widget = nil
    end
    return page
end

function PageStack:pop()
    self:_pop()
    self:updateVisibility()
end

function PageStack:enter(page, ...)
    if not page.widget then page:create() end
    page:setParent(self)
    if #self == 0 then
        self.title = page.title or page.name
        self.widget:setTitle(self.title)
    end

    if not self:contains(page) then
        self:_push(page, ...)
    else
        while self[#self] ~= page do
            assert(#self > 1)
            self:_pop()
        end
    end

    assert(page.parent)
    assert(self[#self] == page)

    self:updateVisibility()
end

function PageStack:exit(page)
    if page == nil then
        self:_pop()
    elseif self:contains(page) then
        assert(page ~= self[1])
        while #self > 1 do
            local p = self:_pop()
            if p == page then break end
        end
    end
    self:updateVisibility()
end

function PageStack:wantFullscreen()
    return self[1] and self[1].fullscreen
end

