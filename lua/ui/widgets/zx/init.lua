local core = require 'zx.core'

assert(core.UI_PAGE)
assert(core.UI_CONTROL)
assert(core.keys)
assert(core.Timer)

require 'ui.widgets.zx.canvas'
require 'ui.widgets.zx.painter'

function core.create(kind, parent)
    if kind == 'page' or kind == 'dialog' then return core.UI_PAGE.create(parent)
    else return core.UI_CONTROL.create(parent)
    end
end

function core.UI_PAGE:onResize(size)
    self.owner():resize(size.w,  size.h)
end

local UI_PAGE = core.UI_PAGE
Timer = core.Timer
assert(not getmetatable(Timer))
setmetatable(Timer,  { __call = Timer.create } )

-- Aliases
core.PageExit    = core.UI_PAGE.Exit
core.PageCurrent = core.UI_PAGE.Current
core.PageRefresh = core.UI_PAGE.refresh

-- Stub functions, for compatiblity
function UI_PAGE:hide() end
function UI_PAGE:show() end

return core
