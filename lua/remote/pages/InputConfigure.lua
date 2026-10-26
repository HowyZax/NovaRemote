local config = require 'config'
local wantZaxnet = not config.tablet
local sources = require 'remote.sources'

local function MainPanel()
    local panel = Panel {
        rows = 1,
        id = 'mainpanel',
    }

    panel:insert(
        Panel {
            columns = 1,
            id = 'inputpanel',
--          background = 'blue',
            CellPanel {
                h = 0.23,
                IncDecButton { "INPUT 1", fit='Analog 16', align='center', id='Channel' },
            },
            Spacer { h=0.01 },
            Panel {
                margin = 0, columns = 1,
                Button { "LEVEL", setting='input.level', index='input.configure', id='Level' },
                EditButton { "TRIM LEVEL",  setting='input.trim.level', index='input.configure', id='Trim' }
            },
            Panel {
                margin = 0, columns = 1,
                Button { "48V", setting='input.power48V', index='input.configure', id='Power48v' },
            },
            Panel {
                margin = 0, columns = 1,
                Button { "PHASE", setting='input.phase', index='input.configure', id='Phase' },
            },
            not wantZaxnet and EditButton { "TRIM",  setting='input.trim.level', index='input.configure' } or
                             Button { "TRIM",  setting='input.trim.mode', index='input.configure' },
--          config.nomad and Spacer { } or
            EditButton { "ZAXNET#", setting='input.zaxnet.id', index='input.configure', conditional=wantZaxnet },
--          Button { "DELAY", setting='input.delay.time', index='input.configure' },
--          Button { "HPF", setting='input.hpf', index='input.configure' },

        })

    if config.nomad then
        local function subIndex(n)
            assert(n)
            return function(self) return { settings.input.configure, n } end
        end
        panel:insert(
            Panel {
                columns = 1,
--              background = 'green',
                CellPanel {
                    columns = 1, margin = 0.03, padding=0.03,
                    Label { "NOTCH 1", h=0.15, align='center' },
                    Button { h=0.3, setting='input.notch.enable', index=subIndex(1) },
                    EditButton { "FREQ",    setting='input.notch.freq', index=subIndex(1) },
                    EditButton { "Q",       setting='input.notch.q', index=subIndex(1), conditional=not config.nomad },
                },
                Spacer { h=0.08 },
                CellPanel {
                    columns = 1, margin = 0.03, padding=0.03,
                    Label { "DELAY", h=0.15, align='center' },
                    Button { h=0.3, setting='input.delay.enable', index='input.configure' },
                    EditButton { "DELAY TIME", setting='input.delay.time', index='input.configure' },
                },
            })

        panel:insert(
            Panel {
                columns = 1,
--              background = 'red',
                CellPanel {
                    columns = 1, margin = 0.03, padding=0.03,
                    Label { "NOTCH 2", h=0.15, align='center' },
                    Button { h=0.3, setting='input.notch.enable', onIndex=subIndex(2) },
                    EditButton { "FREQ", setting='input.notch.freq', onIndex=subIndex(2) },
--                  config.nomad and Spacer { } or
                    EditButton { "Q", setting='input.notch.q', onIndex=subIndex(2), conditional=not config.nomad }
                },
                Spacer { h=0.08 },
                CellPanel {
                    columns = 1, margin = 0.03, padding=0.03,
                    Label { "HPF", h=0.15, align='center' },
                    Button { h=0.3, setting='input.hpf.enable', index='input.configure' },
                    EditButton { "HPF FREQ", setting='input.hpf.freq', index='input.configure' },
                },
            })
    else
        panel:insert(
            Panel {
                columns = 1,
                config.nomad and Spacer{ }  or Button { "EQ", page='InputEq' },
                Button { "COPY|COMPRESS" },
                config.nomad and Spacer{ } or Button { "COPY|EQ" },
                Spacer{ }, --Button { "INC|CHANNEL", id='IncChannel' },
                Spacer{ }, --Button { "DEC|CHANNEL", id='DecChannel' },
            })
    end

    panel:insert(
        Panel {     -- Meter
            w = 0.2,
            columns = 1,
--          background = 'grey',
            id = 'meterpanel',
--          Label { h=0.15, "INPUT 1", fit='Analog 16', align='center', id='Channel' },
--          IncDecButton { h=0.15, "INPUT 1", fit='Analog 16', align='center', id='Channel' },
            Panel {
                margin = {0.2, 0, 0.2, 0.05},
                rows = { 0.1, 0.9 },
                Label { "LEVEL", align='center', font=0.8 },
                -- Following meter shows the single input meter sent from Nomad
                Meter { margin={0.2,0,0.2,0}, vertical=true, id='Meter', source='meters.input', tag=1 },
            },
        })
    local compressIndex = config.nomad and 1 or 'input.configure' -- Nomad has 3 compressor settings: input, output, tracks/card ... not per channel
    panel:insert(
        Panel {     -- Compressor
            columns = 1,
            id = 'compresspanel',
--          background = 'yellow',
            Button { "COMPRESS", setting='input.compress.enable', index='input.configure' },  -- per channel
            Button { "LINK", setting='input.compress.link', index='input.configure' },        -- per channel
            Panel {
                h = 5/7,
                columns = 1,
                background = true,
                EditButton { "ATTACK", setting='input.compress.attack', index=compressIndex },
                EditButton { "DECAY",  setting='input.compress.decay', index=compressIndex },
                EditButton { "THRESH", setting='input.compress.thresh', index=compressIndex },
                EditButton { "RATIO", setting='input.compress.ratio', index=compressIndex },
--              EditButton { "GAIN", setting='input.compress.gain', index=compressIndex },
            }
        })
    return panel
end

local _M = Page {
    name = 'InputConfigure',
    delegate = true,
    depends = {
        'input.configure'
    },
    constrain = { 1, 4 },
    MainPanel { },
}
function _M:onEnter_Page(c)
    SettingNotify('input.configure', self.onChanged_InputConfigure, self)
    self:onChanged_InputConfigure()
end

function _M:onChanged_InputConfigure()
    local setting = SettingFind('input.configure')
    local src = setting:value()
    local digital = sources.isDigitalInput(src)
    local ret = sources.isReturnInput(src)
    local analog = sources.isAnalogInput(src) and not ret
    self.ui.power48v:setVisible(analog)
    self.ui.level:setVisible(analog)
    self.ui.phase:setVisible(not digital)
    self.ui.trim:setVisible(digital)
    for _,o in ipairs(self:findById('mainpanel')) do
        if not o.id or (o.id ~= 'inputpanel' and o.id ~= 'meterpanel') then
            for _,c in ipairs(o) do
                c:setVisible(not digital)
            end
        end
    end
    self:layout()
end

function _M:onUpdate_Channel(c)
    c:setText( sources.inputNameRaw(settings.input.configure) )
end
function _M:onClickInc_Channel()
    settings.input.configure = settings.input.configure + 1
end

function _M:onClickDec_Channel()
    settings.input.configure = settings.input.configure - 1
end

SettingNotify('input.channel',  function(setting)
        local other = Setting('input.configure')
        local value = setting:value()
        if value >= other.range[1] and value <= other.range[2] then
            other:setValue(value)
        end
    end)
