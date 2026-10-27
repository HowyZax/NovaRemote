local config = require 'config'

if not config.nomad then 
    if config.deva24 then 
        Page {
            --name = 'MainMenu',
            title = "Main Menu",
            Panel {
                --y = 0.08,
                columns = 4,
                Button { "Home", page='Home' },
                Button { "Track|Mix", page='TrackMix' },
                Button { "Output|Mix", page='OutputMix' },
                Button { "Fader|Assign", page='FaderAssign' },
                Button { "Input|Configure", page='InputConfigure' },
                Button { "Meters", page='Meters' },
                Button { "Tone|Off" },
                Button { "Record|Tracks", page='RecordChannels' },
                Button { "Note", page='Note' },
                Button { "Input|Names", page='InputNames' },
                Button { "Wireless|Receivers" , page='RX12' },
                Button { "Exit" , onClick=function() mainWindow:close() end, background='darkred' },
            }
        }    
    else Page {
        --name = 'MainMenu',
        title = "Main Menu",
        Panel {
            --y = 0.08,
            columns = 5,
            Button { "Home", page='Home' },
            Button { "Track|Mix", page='TrackMix' },
            Button { "Output|Mix", page='OutputMix' },
            Button { "Fader|Assign", page='FaderAssign' },
            Button { "Player", page='Player' },
            Button { "Dante|Assign", page='DanteRouter' },
            Button { "Setup" , page='SetupMenu' },
            Button { "Input|Configure", page='InputConfigure' },
            Button { "My Deva", page='MyDeva' },
            Button { "Cue|Mode", page='Cue' },
            Button { "Meters", page='Meters' },
            Button { "Tone|Off" },
            Button { "Playback|Router", page='PlaybackRouter' },
            Button { "Record|Tracks", page='RecordChannels' },
            Button { "Note", page='Note' },
            Button { "Input|Names", page='InputNames' },
            Button { "Wireless|Receivers" , page='WirelessRx' },
            Spacer { },
            Spacer { },
            Button { "Exit" , onClick=function() mainWindow:close() end, background='darkred' },
        }
    }
    end
else 

local fullscreen = Setting { 'OFF', 'ON', default=1 }
function fullscreen:value()
    return mainWindow:isFullScreen() and 1 or 0
end
function fullscreen:setValue(value)
    return mainWindow:show(value == 0 and 'normal' or 'fullscreen')
end

Page {     -- For tablet
    --name = 'MainMenu',
    title = "Main Menu",
    Panel {
        --y = 0.08,
        columns = 5,
--      Button { "Home", page='Home' },
--      Button { "Scene|Take", page='Home' },
        Button { "Output|Mix", page='OutputMix' },
        Button { "Track|Mix", page='TrackMix' },
        Button { "Headphone|Mix", page='HeadphoneMix' },
        config.tapemix and Button { "Tape|Mix", page='TapeMix' } or Spacer{ },
        config.monomix and Button { "Mono|Mix", page='MonoMix' } or Spacer{ },

        Button { "Record|Enables", page='RecordChannels' },
        Button { "Track|Names", page='TrackNames' },
        config.hp2mix      and Button { "Hph 2|Mix", page='Hph2Mix' }         or Spacer{ },
        Button { "Input|Config", page='InputConfigure' },
        config.slate       and Button { "Slate" , page='Slate' }              or Spacer{ },

        config.player      and Button { "Player",       page='Player' }       or Spacer{ },
        config.faderassign and Button { "Fader|Assign", page='FaderAssign' }  or Spacer{ },
        config.meterbridge and Button { "Meters",       page='Meters' }       or Spacer{ },
--      Spacer { }, --Button { "Input|Names", page='InputNames' },
        config.rx12 and Button { 'RX12', page='RX12' } or Spacer{ },
        Button { "Setup" , page='SetupMenu' },

--      Button { "Meters", page='Meters' },
--      Button { "My Deva", page='MyDeva' },
--      Button { "Note", page='Note' },
        Button { "Tone", setting='tone.enable' },
        Spacer { }, --Button { "Wireless|Receivers" , page='WirelessRx' },
        Spacer { },
        Button { 'Full Screen', setting=fullscreen }, 
        Button { "Exit" , onClick=function() mainWindow:close() end, background='darkred' },
    },
}

end

