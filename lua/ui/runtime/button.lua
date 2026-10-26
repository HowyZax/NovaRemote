local message = require 'message'

function Button:create(parent)
    --print('','', 'Button.create', parent.__classname)
    local w = widgets.create(self, 'button', parent)

    w:setCanFocus(self.canFocus)

    if w.setText then w:setText(self:text()) end

    -- local p = LayoutElement{ columns = 1 }
    -- for _,line in ipairs(self:textSplit(self:text() or '')) do
    --     table.insert(p, TextElement{ text=line, background='blue' } )
    -- end
    -- if self.setting then
    --     table.insert(p, TextElement{ color='gold', id='.value' } )
    -- end
    -- table.insert(self, p)
    w:setVisible(true)
    return w
end

-- function Button:textSplit(s)
--     local lines = { }
--     for line in s:gmatch("[^|\n]+") do
--         table.insert(lines, line)
--     end
--     return lines
-- end

function Button:onUpdate()
    if self.setting then
        if self.elements then
            local c = self.elements:findById('.value', false)
            if c then
                c:setContent(self:valueAsText())
            end
        end
    end
end

function GraphicElement:onPaint(painter)
--  print('GraphicElement:onPaint', painter, self.rect)
    if self.background then painter:fillRect(self.rect, self.background) end

    if not self.graphic then return end

    --painter:fillRect(self.rect, 'green')
    painter:graphic(self.graphic, self.rect)
end

function TextElement:onPaint(painter)
    painter:text{ self.text, rect=self.rect, align=self.align,
                    color=self.color, background=self.background,
                    fit=self.fit, font=self.font, overline=self.overline }
end

-- function ButtonEx:layout(r)
--     -- print('ButtonEx:layout', r)
--     local layout = Layout( Rect(0, 0, r.w, r.h) )
--     layout:layoutChildren(self)
-- end

-- function ButtonEx:onPaint(painter)
--     self:paintBackground(painter)

--     for _,c in ipairs(self) do
--         if c.visible == nil or c.visible then
--             c:onPaint(painter)
--         end
--     end

--     return true
-- end

function Button:onKeyPress(key)
    print('Button:MSG_KEY_PRESS', self, key)
end

function Button:onClick()
    print('Button:onClick', self.__classname, self.id, self.tag, self:text(), 'page', self.page, self:findParentByClass(Page))
    if self.page then
        Page.Enter(self.page, self:findParentByClass(Page))
    elseif self.action then
        message.send('action', self.action)
    else
        self:incValue()
        self:update()
    end
    return true
end

local ActiveControl = class('ActiveControl')     -- singletone
ActiveControl.instance = nil

function ActiveControl:__init()
    assert(not ActiveControl.instance)   -- enforced singleton
    ActiveControl.instance = self
    self.control = nil                  -- the active control
    self.timer = ElapsedTimer()         -- for timer-based operation
    self.state = 'idle'
    message.subscribe('timer.tick', function()
            self:onTick();
        end)
end

function ActiveControl:onTick()
    if self.prevstate ~= self.state then
        print('ActiveControl:onTick',  self.state,  self.control)
        self.prevstate = self.state
    end

    if self.state == 'idle' then return end
    assert(self.control)

    if self.state == 'new' then
        if self.timer:elapsed() > 300 then
            if self.control.setting then
                self.control.setting:edit(self.control, self.control)
            end
            self.state = 'edit'
        end
    end
end

function ActiveControl:register(c)
    print('ActiveControl.register',  c, self.control)
    if self.control ~= c then
        self:unregister(self.control)
--      if self.control == c then return end
--      self.control.selected = false
--      self.control:update()
    end
    self.control = c
    self.state = 'new'
    self.timer:restart()
end

function ActiveControl:unregister(c)
    print('ActiveControl.unregister',  c, self.control)
    if c and self.control == c then
        self.state = 'idle'
        self.control.selected = false
        self.control:update()
        self.control = nil
    end
end

function ActiveControl:onTouchPress(c)
    print('ActiveControl:onTouchPress',  self.state, c, self.control)
    if self.control ~= c then
        self:register(c)
        return true
    else
        --[[
        if self.state ~= 'new' then
            self:unregister(c)
        end
        --]]
        self.timer:restart()
--      return true
    end
end

function ActiveControl:onTouchRelease(c)
    print('ActiveControl:onTouchRelease',  self.state, c, self.control)
    if self.control == c then
        if self.state == 'new' then
            self.state = 'active'
            self.control.selected = true
            self.control:update()
            return true
        elseif not self.control.selected then
            self:unregister(c)
            return true
        end
    end
end

local activeControl = ActiveControl{ }

--[[
function IncDecButton:onTouchPress(x, y)
    self:update()
end
--]]
function IncDecButton:onTouchRelease(x, y)
    local r = self:rect()
    if (not self.setting or y > r.h/2) and x > r.w/4 and x < 3*r.w/4 then
        self:perform('onClickCenter')
    elseif x > r.w/2 then
        self:perform('onClickInc')
    else
        self:perform('onClickDec')
    end

    self:update()
end

function IncDecButton:onClick()
    -- do nothing .. this comes from the underlying widget
end

function IncDecButton:onClickCenter()
    print 'IncDecButton:onClickCenter'
    if self.names then
        self.names:edit(self, self)
    end
end

function IncDecButton:onClickInc()
    print 'IncDecButton:onClickInc'
    self:incValue(1)
end

function IncDecButton:onClickDec()
    print 'IncDecButton:onClickDec'
    self:incValue(-1)
end

function IncDecButton:onUpdate()
    if self.setting then
        local v = self:value()
        local s = self.names and self.names:text(v) or nil
        if s == nil or s == '' then s = self:valueAsText() end
        self.elements:findById('.value'):setContent(s)
    end
end


function IncDecButton:setValue(v)
    if type(v) == 'string' then
        if self.names then
            self.names:setValue(v, self:value())
        else
            super(IncDecButton).setValue(self,  v)
        end
        self:update()
    else
        super(IncDecButton).setValue(v)
    end
end

function EditButton:onUpdate()
    self.ui.inc:setVisible(self.selected)
    self.ui.dec:setVisible(self.selected)
    self:layout()

    --self:setBackground(self.selected and 0x002000 or nil)
    self:setBackground(self.selected and 0x004000 or nil)
    --self:setStyle(self.selected and 'button.highlight' or 'button')
    self.style = self.selected and 'button.selected' or 'button'

    super(EditButton).onUpdate(self)    -- IncDecBuffer:onUpdate()
end

function EditButton:onTouchPress(x, y)
    print 'EditButton:onTouchPress'
    if not activeControl:onTouchPress(self) then
        -- pass
    end
end

function EditButton:onTouchRelease(x, y)
    print('EditButton:onTouchRelease', self)
    if not activeControl:onTouchRelease(self) then
        local r = self:rect()
        if x > 3*r.w/4 then
            self:perform('onClickInc')
        elseif x < r.w/4 then
            self:perform('onClickDec')
        else
            self:perform('onClickCenter')
        end
    end
    self:update()
end

function EditButton:onClickCenter()
    print 'EditButton:onClickCenter'
    if self.selected then
        activeControl:unregister(self)
    end
end

function EditButton:onRightClick(x, y)
    print 'EditButton.onRightClick'
    if self.selected then
        activeControl:unregister(self)
    end
    if self.setting then
        self.setting:edit(self, self)
    end
end

function EditButton:cancelEditor()
    if self.selected then
        activeControl:unregister(self)
    end
end

