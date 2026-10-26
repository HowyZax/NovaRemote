local colors = require 'colors'
-- TODO: Move to separate module

function Control:paintBackground(painter)
    -- Note: painting background via QStyle::drawControl doesn't mix well with stylesheets;
    -- to allow custom drawing on a stylesheet-styled widget (especially QPushButton) requires
    -- the standard widget paintEvent to be called followed by MSG_PAINT to overlay custom
    -- drawing & text in Lua */
    --self.widget:paintBackground(painter.canvas)
    local background = self.background
    local color = self.color
    local state = self.widget:state()
    local style = self.style

    if background and state == 'disabled' then
        background = 'darkgray'
    elseif background and not color then
        color = colors.textColor(background)
    end

    if state == 'disabled' then
        color = colors.translate('dimgray')
    end


    if not color then
        color = 'white'
    end

    if color then
        painter:setPen(colors.translate(color))
    end

    if background then
        background = colors.translate(background)
        --print(self.__classname, self.id, background, style, state)
        painter:gradient(background, style, state)
    end
end


function Control:setEnabled(state)
    self.enabled = state
    if self.widget then
        self.widget:setEnabled(state)
    else
        for _,c in ipairs(self) do
            c:setEnabled(state)
        end
    end
end

function Control:isEnabled()
    if self.widget then
        return self.widget:isEnabled()
    end
    return self.enabled
end

function Control:setTitle(s)
    if self.widget then self.widget:setTitle(s) end
end

