MsgBox = Page:subclass('MsgBox')
MsgBox.modal = true
MsgBox.deleteOnExit = true
MsgBox.constrain = { 5/3, 8/3 }

function MsgBox:__init(items)
    local panel = Panel {
        background = true,
        style = 'msgbox.panel',
        columns = 1,
        margin = 0.15,
        Spacer { h = 0.2 },
        Panel {
            rows = 1,
            Spacer { w=0.1 },
            Label { "Text message goes here!", fit=30, align='center', color='gold' },
            Spacer { w=0.1 },
        },
        Panel {
            h = 0.2,
            Panel {
                rows = 1,
                Spacer { w=0.1 },
                Button { "OK", style='button.highlight', background='darkgreen' }, --0x217F21 },
                Spacer { w = 0.05 },
                Button { "Cancel", style='button.highlight', background='red' }, --0xEA0E0E },
                Spacer { w=0.1 },
            },
        },
        Spacer { h = 0.1 },
    }
    super(MsgBox).__init(self, { panel } )
end

function MsgBox:show(...)
    local items = {...}
    local parent

    if isinstance(items[1], Page) then parent = table.remove(items, 1) end
    if parent then self.modal, self.popup = false, true end

    self:findByClass(Label)[1]:setText(items[1])
    print('MsgBox:show', items[1])
    local i = 2
    local nbuttons = #items - 1
    for _,button in ipairs(self:findByClass(Button)) do
        if i > #items then
            button:setVisible(false)
        else
            local text, fn = items[i][1], items[i][2]
            button:setVisible()
            button:setText(text)
            button.onClick = function()
                if fn == nil or fn() == nil then self:done(); return true; end end
            i = i + 1
        end
    end
    self:enter(parent)
--  Page.Enter(self)
end

function MsgBox.Show(...)
    local page = MsgBox()
    page:show(...)
end

