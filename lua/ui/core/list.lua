List = Control:subclass('List')
List:attributes{ 'rows', 'columns', 'headerFactory', 'rowFactory', 'reversed' }
function List:__init(items)

    super(List).__init(self, items)

    self.nrows = self.rows or 5

    self.rows = nil
    self.columns = nil

    local panel = Panel { columns = 1 }

    if items.headerFactory then
        local rowPanel = Panel { rows = 1, columns = items.columns, margin=0, items.headerFactory(self) }
        table.insert(panel, rowPanel)
    end

    self.listRows = { }
    if items.rowFactory then
        for i = 1,self.nrows do
            local rowPanel = Panel { rows = 1, columns = items.columns, margin=0, items.rowFactory(self, i) }
            table.insert(panel, rowPanel)
            table.insert(self.listRows, rowPanel)
        end
    end

    table.insert(self, panel);
    self.itemCount = 0
    self.itemOffset = 0
end

function List:onUpdate()
    self.itemCount = self:onItemCount()
    if self.itemOffset+1 > self.itemCount then
        self:scrollToBottom(false)
    end
    for i,c in ipairs(self.listRows) do
        local enable = self.itemOffset + i <= self.itemCount
--      for _,child in ipairs(c) do
--          child:setVisible(enable)
--      end
        c:setEnabled(enable)
    end
end

function List:onItemCount(n)
    print 'Warnign: List:onItemCount not implemented for this list'
    return 0
end

function List:count()
    return self.itemCount;
end

function List:indexOfControl(c)
    if self.reversed then
        return self.itemCount - (self.itemOffset + c.tag-1)
    else
        return self.itemOffset + c.tag
    end
end

function List:_scrollTo(i, wantUpdate)
    print('List:scrollTo', i)
    if wantUpdate == nil then wantUpdate = true end
    if self.itemCount <= 0 then
        self.itemOffset = 0
    elseif i <= 0 then
        self.itemOffset = 0
    elseif i > self.itemCount then
        self.itemOffset = self.itemCount-1
    else
        self.itemOffset = i-1
    end
    if wantUpdate then self:update() end
end

function List:scrollTo(i)
    if i == nil then return end
    if self.reversed then
        i = self.itemCount - (i-1)
    end
    self:_scrollTo(i)
end

function List:scrollBy(delta)
    -- if self.reversed then delta = -delta end
    self:_scrollTo(self.itemOffset+1+delta)
end

function List:scrollToTop(wantUpdate)
    self:_scrollTo(1, wantUpdate)
end

function List:scrollToBottom(wantUpdate)
    self:_scrollTo(self.itemCount, wantUpdate)
end

