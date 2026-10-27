local colors = require 'colors'
-------------------------------------------------------------------
CheckedGroup = Panel:subclass('CheckedGroup')
CheckedGroup:attributes{ 'setting', 'fit', 'color',  }
CheckedGroup.color = 'black'

function CheckedGroup:__init(items)

    -- Add attributes and fields (check buttons/labels) to caller's
    -- table of attributes, according to the setting's menu items

--  items.margin = items.margin or 0
    if not items.rows then
        items.columns = items.columns or { 0.2, 0.8 }
        items.columns = 1
    end
    if items.setting then
        local setting = SettingFind(items.setting)
        assert(isinstance(setting,  SettingMenu))
        local fit = items.fit
        if fit == nil then
            fit = 0
            for _,s in ipairs(setting:textList()) do
                if #s > fit then fit = #s end
            end
        end
        items.background = 'transparent'
        for _,s in ipairs(setting:textList()) do
            if true or items.rows then
                table.insert(items, Button{
                    background = 0xD0000000, --'transparent', --'red', --items.background,
                    style = items.style,
--                  style = 'button.flat',
--                  style = items.style,
--                  background = items.background,
--                  style = 'default',
                    elements = LayoutElement {
                        rows = 1,
                        margin = { 0.2 },
                        GraphicElement {
                            w = 0.25,
                            margin = 0.2,
                            id = 'graphic',
                            constrain = 1,
                            graphic = 'circle-thin'
                        },
                        TextElement {
                            id = 'text',
                            text = s,
                            fit = fit,
                            align='left',
                        }
                    }
                } )
            else
                table.insert(items, Button{
                    elements = LayoutElement {
                        GraphicElement {
                            margin = 0.25,
                            id = 'graphic',
                            constrain = 1,
                            graphic = 'circle-thin'
                        },
                    }
                } )
                table.insert(items, Label { s, fit=fit, margin = { 0.02, },  } )
            end

        end
    end

    -- Perform default initialization

    super(CheckedGroup).__init(self, items)

    -- Fixup check-buttons to make this control the delegate of the
    -- check-buttons and to add identifying information to each check-button
    for i,c in ipairs(self:buttons()) do
        assert(c.id == nil and c.tag == nil and c.delegate == nil)
        c.id, c.tag, c.delegate = 'Button', i, self
    end
    --self.color = colors.translate('black') + 0x101000
--  self.color = 0x383844 --colors.translate('black') + 0x101000

end

function CheckedGroup:buttons()
    return self:findByClass(Button)
end
--[[
function CheckedGroup:onPaintButton(button, painter)
    error 'X'
    if button.checked then
        local r = painter:rect():shrink(0.25)
        local len = r.w > r.h and r.h or r.w
        local r = Rect(0, 0, len, len)
        r:setCenter(painter:rect():center())
--      button:paintBackground(painter)
        --painter:fillRect(r, 'lightblue')
        painter:graphic('circle', r)
    end
end
--]]

function CheckedGroup:onClickButton(button)
    self:setValue(button.tag-1)
end

function CheckedGroup:onUpdate()
    local value = self:value()
    for _,c in ipairs(self:buttons()) do
        c.checked = (value+1 == c.tag)
        assert(c.ui.graphic)
        if c.checked then
            c.ui.graphic:setContent('check-square')
        else
            c.ui.graphic:setContent('square-o')
        end
--      c.background = c.checked and self.color or 'black'
    end
end


