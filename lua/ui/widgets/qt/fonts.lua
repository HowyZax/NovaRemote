local core = require 'qt.core'
local Font = assert(core.QFont)

function createfont(items)
    local font = Font.create()
    font:setFont(table.unpack(items))
    return font
end

local fontTbl = {
    default         = createfont { 'Arial', 'bold' },
    timecode        = createfont { 'Digital-7', 'normal' },
--  timecode        = createfont { 'Digital-7', 'italic' },
    timecodeedit    = createfont { 'Digital-7 Mono', 'normal' },
    usertext        = createfont { 'Courier New', 'normal' },
    edittext        = createfont { 'Consolas', 'normal' },
    status          = createfont { 'Consolas', 'normal' },
    ["timecode.small"] = createfont { 'Consolas', 'normal' },
}

-- TODO: consider weak keys/value and allow garbage collector to maintain size

local fontCache = { }

-- Return base font and scaling from caller's parameters
local function fontUnpack(fnt)
    local name, scale, maxpx = 'default', 1.0, 200
    if fnt == nil then
        -- do nothing; use defaults
    elseif type(fnt) == 'number' then
        scale = fnt
    elseif type(fnt) == 'string' then
        name = fnt
    elseif type(fnt) == 'table' then
        name, scale = table.unpack(fnt)
    else
        error '!'
    end
    if name == 'timecode' then maxpx = 300 end
    return fontTbl[name], scale, maxpx
end


local function fitFont(s, w, h, fnt)
    if type(s) == 'number' then s = string.rep('M', s) end
    local basefont, scale, maxpx = fontUnpack(fnt)
    local k = string.format('%.3f:%s:%.1f:%.1f', scale, s, w, h)
    local cache = fontCache[basefont]
    if not cache then cache = { }; fontCache[basefont] = cache; end
    local f = cache[k]
    if f == nil then
        local low, high = 2, maxpx
        f = basefont:copy()
        repeat
            local mid = (low + high)/2
            f:setFont(mid)
            local wtext, htext = f:textBounds(s)
            if wtext > w or htext > h then high = mid
            else low = mid end
        until math.abs(high-low) <= 0.5

        f:setFont(low*scale)

        local count = cache[1] or 0
        if count >= 1024 then
            print('Discarding FitFontCache', count)
            --brute force!
            fontCache[basefont] = { }
            cache = fontCache[basefont]
            count = 0
        end
        cache[k] = f;
        cache[1] = count + 1
        --print('FitFontCache', cache[1], k, f)
    end
    return f --:copy()
end

return {
    fitFont = fitFont,
}

