local despatch = require 'ui.runtime.despatch'
local perform = despatch.perform

function Object:perform(method, ...)
    return perform(self, method, ...)
end

function Object:create(w)
    self:createChildren(w)
end

