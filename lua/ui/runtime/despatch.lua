--require 'message'

local function delegate(target, method, sender, ...)
    if not sender.id then return end
    local m = target[method .. sender.id]   -- e.g. onClickStop (obsolete)
    if m then
        local result = m(target, sender, ...)
        return result == nil and true or result
    end
    m = target[method .. '_' .. sender.id]  -- e.g. onClick_Stop (preferred)
    if m then
        local result = m(target, sender, ...)
        return result == nil and true or result
    end
end

local function perform(target, method, ...)
    local result
    if target.delegate then
        result = delegate(target.delegate, method, target, ...)
--      print('Tried delegate...', result)
    end
    if not result or result == 0 then
        if target[method] then
            result = target[method](target, ...)
--          print('Tried target...', result)
        end
    end
    return result
end

function despatch(target, method, ...)
    local result = 0
    if method == "onPaint" then
        -- TODO: check if object responds first?
        local painter = Painter.create(target)
        result = perform(target, 'onPaintBackground', painter)
        result = perform(target, method, painter)
        painter:update()
        return result
    end
    return perform(target, method, ...)
end

return {
    perform = perform,
    despatch = despatch,
}
