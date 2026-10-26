local core = require 'qt.core'

--[[
assert(core.debugString)
function dprint(...)
    s = ''
    for i,v in ipairs{...} do
        s = s .. tostring(v) .. ' '
    end
    s = s .. '\n'
    core.debugString(s)
end
local oldprint = print
function print(...)
    dprint(...)
    oldprint(...)
end
--]]


require 'ui.widgets.qt.painter'
------------------------------------------------------------------
assert(core.QObject)
assert(core.Tracker)
assert(core.QWidget)
assert(core.QDialog)
assert(core.PushButton)
--assert(core.TabWidget)
--assert(core.QLineEdit)
assert(core.QPainter)
assert(core.Image)
assert(core.Desktop)
assert(core.ElapsedTimer)
assert(core.Timer)
-------------------------------------------------------------------

QPainter = core.QPainter
Image = core.Image
Desktop = core.Desktop
ElapsedTimer = core.ElapsedTimer
Timer = core.Timer

TraceTimer =  ElapsedTimer.create()

assert(core.chooseFile)
-------------------------------------------------------------------

function core.create(control, kind, parent, ...)
    local function create(kind, parent, ...)
        --print('core.create ' .. tostring(kind))
        if type(kind) == 'table'    then return kind.create(parent, ...)
        elseif kind == 'widget'     then return core.QWidget.create(parent, ...)
        elseif kind == 'button'     then return core.PushButton.create(parent, ...)
        elseif kind == 'page'       then return core.QWidget.create(parent, ...)
        elseif kind == 'textedit'   then return core.QLineEdit.create(parent, ...)
        elseif kind == 'textbox'    then return core.QPlainTextEdit.create(parent, ...)
        elseif kind == 'dialog'     then return core.QDialog.create(parent, ...)
        elseif kind == 'tabbed'     then return core.TabWidget.create(parent, ...)
        else
            error('Unknown widget type: ' .. kind)
        end
    end
    local w = create(kind, parent, ...)
    if control then
        control.widget = w
        w:setOwner(control)    -- .. allows this control to receive messages sent to the widget
    end
    return w
end

-------------------------------------------------------------------
local owner = setmetatable( { },  { __mode = 'k' } )

function core.QObject:owner()
    return owner[self]
end
function core.QObject:setOwner(o)
    owner[self] = o
    self:setProperty('class', o.__classname)
    if o.name then self:setName(o.name) end
    if o.tag then self:setProperty('tag', o.tag) end
    if o.id then self:setProperty('id', o.id) end
end

function core.QObject:__deleted()
    print('core.QObject:__deleted', self)
    owner[self] = nil   -- C++ object has been deleted
end

-------------------------------------------------------------------
function core.QWidget:perform(method, ...)
    local owner = self:owner()
    if owner then
        if method == 'onPaint' or method == 'onRender' then
            local painter = ...
            print(method, self.__classname, self.owner.name or '??')
            painter:initFrom(owner)
        end
        owner:perform(method, ...)
    end
end

-- Cleanup known references to Lua objects when QWidget is deleted.
-- This allows Lua garbage collection to collect the associated Lua objects.
function core.QWidget:__deleted()
    print('core.QWidget:__deleted', self, self.__classname)
    local owner = self:owner()
    if owner and owner.widget then
        assert(owner.widget == self)
        print 'clearing owner.widget'
        owner.widget = nil
    end
    super(core.QWidget).__deleted(self)
end

function core.QWidget:setOwner(o)
    core.QObject.setOwner(self, o)
    if o.drop then self:setAcceptDrops(true) end
    self:repolish()
end

function core.QWidget:isAnimating()
    return mainWindow and mainWindow:isAnimating(self)
end

function core.QWidget:animate(...)
    if mainWindow then
        mainWindow:animate(self, ...)
    end
end

-------------------------------------------------------------------
-- Following allows timer to be created using Timer() instead of Timer.create()
assert(not getmetatable(ElapsedTimer))
setmetatable(ElapsedTimer, { __call = ElapsedTimer.create  } )

assert(not getmetatable(Timer).__call)
getmetatable(Timer).__call = function(cls, opts)
    local o = cls.create()
    if opts and opts[1] then
        o:setTimeout(opts[1])
    end
    return o
end


return core
