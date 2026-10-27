local config = require 'config'
local message = require 'message'

function AboutButton()
    local product, text
    if     config.nova    then product, text = "Touch",  "Nova Touch GUI version NT" .. config.version
    elseif config.tablet  then product, text = "Touch",  "Nomad Touch GUI version NT" .. config.version
    elseif config.surface then product, text = "Oasis",  "Oasis GUI version NO" .. config.version
    else                       product, text = "Deva32", "Deva32 GUI version " .. config.version
    end
    return Button { "About|" .. product, id='About',
            onClick = function(self)
                message.send('popup.msg',  { text, timeout=5000 } )
            end,
        }
end

Page {
    --name = 'SetupMenu',
    title = "Setup Menu",
    delegate = true,
    Panel {
        columns = 5,
        --IncDecButton { "Sample|Rate", setting='sample.rate' },
        --Button { "Tone|Level", setting='tone.level'},
        Button { 'Pre Record', setting='record.preroll' },
        config.camera   and Button { "Camera", setting='camera.name' } or Spacer{},
        config.recorder and Button { 'MP3 Rec', setting = 'mp3record.mode' } or Spacer{ },
        --Button { "Pre|Record", setting='record.preroll' },
        --Button { "Operating|Mode", page='OperatingMode' },
        Button { "Home|Meters", setting='home.meters.count' },
        config.meterbridge and Button { "Meter|Bridge", setting='meters.count' } or Spacer{ },
--      Button { "Headphone|Options" },
        Button { "Serial", setting='serial.port' },
        Button { "ISO|Attenuation", setting='neverclip.attenuation' },
        config.panfollow and Button { "Pan|Follow", setting='outputmix.pan.follow' } or Spacer{},
        --Spacer { },
        --      Button { "Memory", page='Memory' },
--      Spacer { },
        config.zaxnet and Button { "Zaxnet" } or Spacer{ },
        config.nomad and Spacer { } or Button { "User|Interface" },
        config.nomad and Spacer { } or Button { 'Backlight', setting='controller.backlight' },
        config.nomad and Spacer{ } or Button { "Clock" },
        Spacer { },

        config.tablet and Spacer{ } or Button{ 'Window|Frames', onClick = function() ToggleWindowFrames() end },
        config.tablet and not config.uicfg and Spacer{ } or Button{ 'Save|Layout', onClick = function() saveWindowPositions() end },
        Spacer { },
        Spacer { },
        AboutButton { },

        --[[
        EditButton{ 'Meter Hold',   setting='meters.meter.hold' },
        EditButton{ 'Meter Decay',  setting='meters.meter.decay' },
        EditButton{ 'Peak Hold',    setting='meters.peak.hold' },
        EditButton{ 'Peak Decay',   setting='meters.peak.decay' },
        EditButton{ 'UI Update',    setting='ui.update.period' },
        --]]
    },

}

