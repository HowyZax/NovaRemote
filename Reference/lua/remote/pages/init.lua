local config = require 'config'

local m = ...
local import = function(s) return require (m .. '.' .. s) end

import('MsgBox' )
import('PopupMsgBox' )
import('Editors' )

import('Home' )
import('MixPage' )
import('SceneTakeNote' )
import('PresetNotes' )
import('InputConfigure' )
import('InputNames' )
import('Timecode' )
if not config.deva24 then
    import('SetupMenu' )
end
import('MainMenu' )
import('RecordChannels' )

if config.deva24 then
    import('CommandMonitor' )
    import('FaderAssign' )
    import('HeadphoneMix' )
    import('InputEq' )
    import('Meters' )
elseif not config.nomad then
    import('Cue' )
    import('MyDeva' )
    import('CommandMonitor' )
    import('OperatingMode' )
    import('FaderAssign' )
    import('HeadphoneMix' )
    if config.scriptview then
        import('ScriptViewer' )
    end
    if config.dante then
        import('DanteRouter' )
    end
    import('DiskUtils' )
    import('InputEq' )
    import('PlaybackRouter' )
    import('Memory' )
    import('Meters' )
else
    import('HphMix' )
--  import('MonitorMix' )
end

import('WirelessRx' )

if config.tablet then
    import('Slate' )
end

if config.surface then
    import('FaderAssign' )
    import('SourceRouting' )
    import('Meters' )
end


