--local mixerStorage = Storage()
local config = require 'config'
local sources = require 'remote.sources'
local _, serial = pcall( function() return require 'serial' end )

-------------------------------------------------------------------
-- Setting to select from available serial ports
SettingSerialPort = SettingBase:subclass('SettingSerialPort')
SettingSerialPort.default = 'AUTO'
function SettingSerialPort:incValue()
    local value = self:value() or ''
    local ports = serial.ports()
    table.insert(ports, 1, 'AUTO')
    table.insert(ports, 'TCP')
    if #ports > 0 then
        local idx
        for i,v in ipairs(ports) do
            if v == value then idx = i end
        end
        if not idx or idx >= #ports then
            self:setValue(ports[1])
        else
            self:setValue(ports[idx + 1])
        end
   end
end

-------------------------------------------------------------------
-- sample
Setting {
    name = 'sample.rate',
    menu = config.nova and { '48000', '48048', '88200', '96000' } 
           or              { '44100', '47952', '48000', '48048', '88200', '96000'}, -- '192000' },
}

-------------------------------------------------------------------
-- record
Setting {
    name = 'record.preroll',
    range = { 0, 10 },  --  translate = { [0] = 'NONE' },
    zero='OFF',
    units = 'sec',
}

Setting {
    name='record.tracks',
    '', 'X',
    array = { 32 }
}

-------------------------------------------------------------------
-- tone
Setting {
    name = 'tone.level',
    range = { -20, -12, step = 2 },
    default = -12,
    units = 'dB'
}

Setting { name='tone.enable', 'OFF', 'ON' }

-------------------------------------------------------------------
-- meters
if config.nomad then
    Setting { name='meters.count', range = {8,14}, default=8 }  -- Meter-bridge meters
else
    Setting { name='meters.count', range = {8,16}, default=16 }
end

-------------------------------------------------------------------
-- track
do

local ntracks = config.ntracks -- config.nomad and 12 or 32

SettingText{ name='track.name',  description='Track Name', array = { ntracks } }
Setting{ name='track.color',  0, 0xFFFFFF, array = { ntracks } }

end

-------------------------------------------------------------------
-- trackmix
do

local npresets = 8 -- I dont thjink we are using presets in Nova or Nomad
local ntracks = config.ntracks -- config.nomad and 12 or 8
local nisotracks = 24

local nsources = config.nomad and sources.numberOfSources() or 32  -- TODO: remove    was 22 for noamd

Setting { name='trackmix.preset', range = { 1, npresets }, protocol={offset=-1} }
SettingText{ name='trackmix.names', array={ npresets }, description='Preset Name' }
Setting { name='trackmix.phase', 'Normal', 'Invert' }
Setting { name='trackmix.fader', 'Pre', 'Post' }
Setting { name='trackmix.matrix', array = {npresets, ntracks, nsources}, default=0, range={0, 0x7} }

Setting { name='trackmix.isomatrix', array = {npresets, nisotracks }, default=0, range={0, 0x7FFF} }   -- presets, source; value = { chan, flags }
Setting { name='trackmix.comp', 'OUT', 'IN', array={npresets, ntracks} }
Setting { name='trackmix.tone', '', 'X', array={npresets, ntracks} }
Setting { name='trackmix.slate', '', 'X', array={npresets, ntracks} }
Setting { name='trackmix.neverclip', '', 'X', array={npresets,ntracks} }

if config.nova then
    Setting { name='trackmix.which', 'TRACK 1', 'TRACK 2', 'TRACK 3', 'TRACK 4', 'TRACK 5', 'TRACK 6', 'TRACK 7', 'TRACK 8', 'TRACK 9', 'TRACK 10', 'TRACK 11', 'TRACK 12', 'TRACK 13', 'TRACK 14', 'TRACK 15', 'TRACK 16' }
--  end
--  else if config.nomad then
--    Setting { name='trackmix.which', 'TRACK 1', 'TRACK 2', 'TRACK 3', 'TRACK 4', 'TRACK 5', 'TRACK 6', 'TRACK 7', 'TRACK 8', 'TRACK 9', 'TRACK 10', 'TRACK 11', 'TRACK 12' }
--  end
  else
    Setting { name='trackmix.which', 'TRACK 1-2', 'TRACK 3-4', 'TRACK 5-6', 'TRACK 7-8', 'ISO 9-32' }
  end

end

-------------------------------------------------------------------
-- outputmix
do

local npresets = config.nomad and 1 or 8   -- I dont thjink we are using presets in Nova or Nomad
local noutputs = config.noutputs
local nsources = config.nomad and sources.numberOfSources() or 32

Setting    { name='outputmix.preset', range = {1, npresets}, description='Output Mix Preset', protocol={offset=-1} }
SettingText{ name='outputmix.names', array={npresets}, description='Preset Name' }
Setting    { name='outputmix.phase', 'Normal', 'Invert' }
Setting    { name='outputmix.fader',  'Pre', 'Post' }

Setting { name='outputmix.matrix',  array = {npresets, noutputs, nsources   }, default=0, range={0,0x7} }   -- presets, channels, sources

Setting { name='outputmix.comp',  'OUT', 'IN', array={npresets, noutputs}, default=0 }
Setting { name='outputmix.tone',  '', 'X', array={npresets, noutputs} }
Setting { name='outputmix.slate', '', 'X', array={npresets, noutputs} }
Setting { name='outputmix.com1',  '', 'X', array={npresets, noutputs} }
Setting { name='outputmix.com2',  '', 'X', array={npresets, noutputs} }
Setting { name='outputmix.trk1',  '', 'X', array={npresets, noutputs} }
Setting { name='outputmix.trk2',  '', 'X', array={npresets, noutputs} }
Setting { name='outputmix.neverclip',  '', 'X', array={npresets,noutputs} }

if config.nomad then
    Setting { name='outputmix.which',  'OUT 1', 'OUT 2', 'OUT 3', 'OUT 4', 'OUT 5', 'OUT 6', 'OUT 7', 'OUT 8' }
--end
--if config.nova then
--    Setting { name='outputmix.which',  'OUT 1', 'OUT 2', 'OUT 3', 'OUT 4', 'OUT 5', 'OUT 6', 'OUT 7', 'OUT 8' }
else
    Setting { name='outputmix.which',  "OUT 1-2", "OUT 3-4", "OUT 5-6", "OUT 7-8" }
end
Setting { name='outputmix.pan.follow',  "OFF", "3-4", "5-6", "3-6" }

end

-------------------------------------------------------------------
-- sourceroute
Setting { name='sourceroute.phase', 'Normal', 'Invert' }
Setting { name='sourceroute.fader', 'Pre', 'Post' }

-------------------------------------------------------------------
-- mixer
-- Settings for hardware mixer control surface

if config.nomad then
    Setting { name='mixer.bank', "Bank 1", "Bank 2", "Bank 3" }
else
    Setting { name='mixer.bank', "Bank 1", "Bank 2", "Bank 3", "Bank 4", "Bank 5" }
end
Setting { name='mixer.enable', 'OFF', 'ON', array={ 16 } }

Setting {   -- See sources.lua
    -- The audio source connected to each mixer input
    name='mixer.source',
    array={5, 16},  -- banks, columns
    default = 0,    -- off
}

Setting {   -- See sources.lua
    -- Zaxnet id assigned to the fader
    -- See also mixer.fader.mode    
    name='faders.zaxnet.id',
    range={1, 30},
    array={5, 16},      -- banks, columns
}

Setting {   -- See sources.lua
    name='faders.mode',
    array={5, 16},   -- banks, columns
    menu={ 
        '',     -- default, fader controls mixer.source
        'T',    -- Trim mode, fader controls trim for mixer.source
        'Z',    -- Zaxnet mode, fader controls Zaxnet channel in faders.zaxnet.id
    },
}

-------------------------------------------------------------------
-- output
Setting{ name='output.source', array = {8}, range={0,32} }

-------------------------------------------------------------------
-- hpmix
do

npresets = config.nomad and 12 or 8

Setting { name='hpmix.preset', range = { 1, npresets }, description='Headphone Mix Preset', protocol={offset=-1} }
SettingText{ name='hpmix.names', array={ npresets }, description='Preset Name' }
Setting{ name='hpmix.phase', 'Normal', 'Invert' }

if config.nomad then
    Setting{ name='hpmix.enable', array={npresets },   -- presets
        menus = { 'OFF', 'ON' },
    }
    Setting{ name='hpmix.tracks', array={npresets, sources.numberOfTracks() },   -- presets, channels
        menus = { '', 'L', 'R', 'LR', '/L', '/R', '/LR', 'L/R', '/L/R' },
    }
    Setting{ name='hpmix.outputs', array={npresets, sources.numberOfOutputs() },   -- presets, channels
        menus = { '', 'L', 'R', 'LR', '/L', '/R', '/LR', 'L/R', '/L/R' },
    }
    Setting{ name='hpmix.returns', array={npresets, sources.numberOfReturns() },   -- presets, channels
        menus = { '', 'L', 'R', 'LR', '/L', '/R', '/LR', 'L/R', '/L/R' },
    }
--  Setting{ name='hpmix.zaxnet', array={npresets, 12 },   -- presets, channels
--      menus = { '', 'LR' },
--  }
    Setting{ name='hpmix.znet.select', range={-1,5}, default=-1, array={npresets }}  -- presets, channels
else
    Setting{ name='hpmix.matrix', array={npresets, 44 },   -- presets, channels
        menus = { '', 'L', 'R', 'LR', '/L', '/R', '/LR' },
    }
end

--Setting{ name='hpmix.name', default='MIX 1', save=false }

end

-------------------------------------------------------------------
-- hpmix2
do

Setting{ name='hpmix2.phase', 'Normal', 'Invert' }

if config.nomad then
    Setting{ name='hpmix2.tracks', array={ sources.numberOfTracks() },   -- presets, channels
        menus = { '', 'L', 'R', 'LR', '/L', '/R', '/LR', 'L/R', '/L/R' },
    }
    Setting{ name='hpmix2.outputs', array={ sources.numberOfOutputs() },   -- presets, channels
        menus = { '', 'L', 'R', 'LR', '/L', '/R', '/LR', 'L/R', '/L/R' },
    }
    Setting{ name='hpmix2.returns', array={ sources.numberOfReturns() },   -- presets, channels
        menus = { '', 'L', 'R', 'LR', '/L', '/R', '/LR', 'L/R', '/L/R' },
    }
else
    Setting{ name='hpmix2.matrix', array={ 44 },   -- presets, channels
        menus = { '', 'L', 'R', 'LR', '/L', '/R', '/LR' },
    }
end

end

-------------------------------------------------------------------
-- tapemix

if config.nomad then
    Setting{ name='tapemix.phase', 'Normal', 'Invert' }

    Setting{ name='tapemix.tracks', array={ sources.numberOfTracks() },   -- channels
        menus = { '', 'X', '/X' },
    }
    Setting{ name='tapemix.outputs', array={ sources.numberOfOutputs() },   -- channels
        menus = { '', 'X', '/X' },
    }
    Setting{ name='tapemix.returns', array={ sources.numberOfReturns() },   -- channels
        menus = { '', 'X', '/X' },
    }
end
-------------------------------------------------------------------
-- monomix

if config.nomad then
    Setting{ name='monomix.phase', 'Normal', 'Invert' }

    Setting{ name='monomix.tracks', array={ sources.numberOfTracks() },   -- channels
        menus = { '', 'X', '/X' },
    }
    Setting{ name='monomix.outputs', array={ sources.numberOfOutputs() },   -- channels
        menus = { '', 'X', '/X' },
    }
    Setting{ name='monomix.returns', array={ sources.numberOfReturns() },   -- channels
        menus = { '', 'X', '/X' },
    }
end

-------------------------------------------------------------------
-- metadata
Setting { name='metadata.continuity',  'Approve', 'Allow' }
SettingTable{ name='metadata.history', array={10} }

-------------------------------------------------------------------
-- scene
Setting{ name='scene.current',    'SCENE 1', maxchars=12, description='Scene Name', save=false }
Setting{ name='scene.next',       'SCENE 1', maxchars=12, description='Scene Name' }
Setting{ name='scene.continuity', '',        maxchars=12, description='Scene Name' }

-------------------------------------------------------------------
-- take
Setting{ name='take.current',     '1', maxchars=12, description='Take Name', save=false }
Setting{ name='take.next',        '1', maxchars=12, description='Take Name' }
Setting{ name='take.continuity',  '',  maxchars=12, description='Take Name' }

-------------------------------------------------------------------
-- note
local noteDefault = config.surface and 'Zaxcom Oasis' or 'Zaxcom Nomad'
if config.nova then noteDefault = 'Nova Touch' end
Setting{ name='note.current',  noteDefault, maxchars=32, description='Note', save=false }
Setting{ name='note.next',     noteDefault, maxchars=32, description='Note' }
Setting{ name='note.preset',                maxchars=32, description='Preset Note', array={10}}

-------------------------------------------------------------------
-- input
do

local ninputs = sources.inputConfigureCount()
--local nanalog = config.nomad and 6+4 --[[ +8 --]] or 32
local ncompress = config.nomad and 3 or ninputs

if config.nomad then
    Setting{ name='input.channel', 1, ninputs,
        text=function(self, ...) return sources.inputNameRaw(self:value(...)) end,
        protocol={offset=-1}
    }
    Setting{ name='input.configure', 1, ninputs,    -- Selection for Input Configure page (analog only)
        text=function(self, ...) return sources.inputNameRaw(self:value(...)) end,
        protocol={offset=-1}
    }
else
    Setting{ name='input.channel', 1, ninputs, protocol={offset=-1} }
    Setting{ name='input.configure', 1, ninputs, protocol={offset=-1} }
end
Setting{ name='input.warning', range={0,1}, save=false, array={ninputs}  }

Setting{ name='input.level',  'MIC', 'LINE', array={ninputs}  }
Setting{ name='input.power48V',  'OFF', 'ON', array={ninputs} }

Setting{ name='input.delay.enable',  'OFF', 'ON', array={ ninputs } }
Setting{ name='input.delay.time', range={0,100,step=0.1}, units='ms', array={ ninputs } }

Setting{ name='input.hpf.enable', 'OFF', 'ON', array={ ninputs } }
Setting{ name='input.hpf.freq', range={40, 230, step=10}, units='Hz', array={ ninputs } }

Setting{ name='input.trim.level', range={ -20,30 }, units='dB', array={ninputs}  }
Setting{ name='input.trim.mode', 'NOMAD', 'ZAXNET',array={ninputs}  }

Setting{ name='input.zaxnet.id', range={1,30}, array={ninputs}  }

Setting{ name='input.compress.enable', 'OFF', 'ON', array={ninputs} }    -- per input
Setting{ name='input.compress.link', 'NONE', 'GROUP A', 'GROUP B', 'GROUP C', array={ninputs} } -- per input

Setting{ name='input.compress.attack',  default = 5, range={ 1,100}, units='mS', array={ncompress} }    -- per compressor
Setting{ name='input.compress.decay',  default = 100, range={ 1,100}, units='mS', array={ncompress} }
Setting{ name='input.compress.thresh',  default = -6, range={ -20, 0,}, units='dB', array={ncompress} }
Setting{ name='input.compress.ratio',  default = 4.0, range={ 1.1, 20.0, step=0.1 },units=':1', array={ncompress} }
Setting{ name='input.compress.gain',  default = 0, range={ -12, 12 },  units='dB', array={ncompress} }

Setting{ name='input.source',
    0, 64, array={ ninputs },
    -- TODO: remove input.source methods
    text = function(self, index)
        local v = self:value(index)
        if v == 0 then return index > 16 and tostring(index - 16) or tostring(index)
        else return tostring(v) end
    end,
    graphic = function(self, index)
        local v = self:value(index)

        if v == 0 then return index > 16 and 'pulse' or 'sine'
        else return 'dante' end
    end,
}    -- 0 = default; 1-64 = Dante input N
Setting{ name='input.eq',  'OFF', 'ON', array={ ninputs } }
Setting{ name='input.cl',  'OFF', 'ON', array={ ninputs } }
Setting{ name='input.hpf',  'OFF', 'ON', array={ ninputs } }
Setting{ name='input.ntc1',  'OFF', 'ON', array={ ninputs } }
Setting{ name='input.ntc2',  'OFF', 'ON', array={ ninputs } }

Setting{ name='input.phase',  'Normal', 'Invert', array={ ninputs } }
Setting{ name='input.dly',  'OFF', 'ON', array={ ninputs } }
SettingText{ name='input.name',  description='Input Name', array = { ninputs } }
Setting{ name='input.color',  0, 0xFFFFFF, array = { ninputs } }

local nnotch = 2
Setting{ name='input.notch.enable',  'OFF', 'ON', array={ ninputs, nnotch } }
Setting{ name='input.notch.freq', range={20,15000}, units='Hz', array={ ninputs, nnotch } }
Setting{ name='input.notch.q', range={0.1,5.0,step=0.1}, array={ ninputs, nnotch } }

-- Meter bridge input assigments, used instead of fader assign routing for any values that are set by Nomad
-- Value read as -1 if not assigned here, 0 if disabled
Setting { name='meters.source', range={-1,ninputs}, default=-1, array={16}, save=false }  -- Inputs assigned to meter bridge

end

-------------------------------------------------------------------

SettingText{ name='names.character.name',  array={ 16 }, description='Character Name' }
Setting{ name='names.character.color',  0, 0xFFFFFF, array = { 16 } }

SettingText{ name='names.preset.name',  array={ 32 }, description='Preset Name' } -- was 16
Setting{ name='names.preset.color',  0, 0xFFFFFF, array = { 16 } }



-------------------------------------------------------------------
-- timecode
Setting{ name='timecode.out',  'Disk', 'Generator' }
Setting{ name='timecode.displayed',  'Disk', 'Generator' } -- 'Gen Stop'
Setting{ name='timecode.framerate',  '23.98', '24', '25', '29.97NDF', '29.97DF', '30NDF', '30DF' }
Setting{ name='timecode.incuserbits',  'OFF', 'ON' }
Setting{ name='timecode.runmode',  'Free Run', 'Rec-Run', 'C-Jam All', 'C-Jam TC', 'C-Jam UB' }
Setting{ name='timecode.autojam',  'OFF', 'ON' }
Setting{ name='timecode.reader.tc',  timecode=true, description='Reader Time Code' }
Setting{ name='timecode.reader.ub',  userbits='00000000', description='Reader User Bits' }
Setting{ name='timecode.generator.tc',  timecode=true, description='Generator Time Code' }
Setting{ name='timecode.generator.ub',  userbits='00000000', description='Generator User Bits' }

-------------------------------------------------------------------
-- mirror
Setting{ name='mirror.drive',  'Internal DVD',  'Firewire #1' }
Setting{ name='mirror.firewire.power', 'OFF', 'ON' }
Setting{ name='mirror.filetype', 'Wav Mono', 'Wav Poly', 'Wav Mono F', 'Wav Poly F', 'ZAX File' }
Setting{ name='mirror.resolution',  '16 bits', '24 bits' }
Setting{ name='mirror.src',  'Off', '44100', '47952', '48000', '48048', '88200', '96000', '96096', '192000'}
Setting{ name='mirror.mode',  'OFF', 'ON' }
Setting{ name='mirror.startseg',  range={0, 999} }
Setting{ name='mirror.endseg',  range={0, 999} }
Setting{ name='mirror.timestamp',  'Pull Up', 'Pull Down' }
Setting{ name='mirror.tracks',  array = { 32 }, range={0,1} }

-------------------------------------------------------------------
-- setup
Setting{ name='setup.transport',  'Auto-load', 'Normal', 'T.C. Chase' }
Setting{ name='setup.gpi1.remoteroll', 'Off', 'Rising Edge', 'Falling Edge' }
Setting{ name='setup.serial.remoteroll',  'Off', 'On' }
Setting{ name='setup.serial.mode',  'RS-422', 'RS-232' }
Setting{ name='setup.master.mode',  'Off', 'Play Switches' }

-------------------------------------------------------------------
-- script
SettingText{ name='script.file',  description="Script file" }
Setting{ name='script.show.actions',  'OFF', 'ON' }
Setting{ name='script.edit.enable',  'OFF', 'ON' }
Setting { name='script.scroll.step',
    menu = {
        { '1/4', value=1/4. },
        { '1/3', value=1/3. },
        { '1/2', value=1/2. },
        { '3/4', value=3/4. },
        { '1',   value=1. },
    },
    units = ' page'
}
Setting {
    name='script.font',
    menu = {
        { 'Normal', value=1.  },
        { 'Medium', value=1.4 },
        { 'Large',  value=1.8 },
    },
}

-------------------------------------------------------------------
-- store
Setting { name='store.current',  '---' }
Setting { name='store.segment',  '---' }
Setting { name='store.folder',  '---' }

-------------------------------------------------------------------
-- serial
SettingSerialPort{ name='serial.port' }

-------------------------------------------------------------------
-- camera
Setting{ name='camera.name', 'A', 'B', 'C', 'D', 'E', 'F', 'G' }

-------------------------------------------------------------------
-- home
Setting{ name='home.meters.source', menu=config.nomad and { 'TRACKS', 'OUT', } or
                                                   { 'TRACKS 1-8',  'OUT 1-8', }
}

if config.nova then
    Setting { name='home.meters.count', range = {8,config.ntracks}, default=8 }  -- Home page meters
elseif config.nomad then
    Setting { name='home.meters.count', range = {8,config.ntracks}, default=8 }  -- Home page meters
else
    Setting { name='home.meters.count', range = {8,16}, default=16 }
end

Setting { name='mp3record.mode', 'Follow Recorder', 'Manual' }

SettingTable{ name='mp3.record.file' }
SettingTable{ name='mp3.record.metadata' }

-------------------------------------------------------------------
-- wirelessrx
Setting{ name='wirelessrx.centerfreq', range={550, 590}, default=555, units='MHz' }

-------------------------------------------------------------------
-- controller
Setting{ name='controller.backlight', range={0, 30}, default=10 }

-------------------------------------------------------------------
-- neverclip
Setting { name='neverclip.attenuation', 'OFF', '-6dB', '-12dB', '-18dB', '-24dB' }

-------------------------------------------------------------------
-- battery
Setting { name='battery.status', default='---', save=false }

-------------------------------------------------------------------
-- status
Setting { name='drive.time.remaining', default='000:00:00', save=false }
Setting { name='drive.time.elapsed', default='000:00:00', save=false }

Setting { name='status.text', default='', save=false }

-------------------------------------------------------------------
-- debug
Setting { name='meters.meter.hold',  range={0.01,5.0,step=0.01}, default=0.30, units='s', save=false }
Setting { name='meters.meter.decay', range={0.01,5.0,step=0.01}, default=1.25, units='s', save=false }
Setting { name='meters.peak.hold',   range={0.01,5.0,step=0.01}, default=1.50, units='s', save=false }
Setting { name='meters.peak.decay',  range={0.01,5.0,step=0.01}, default=4.00, units='s', save=false }
Setting { name='ui.update.period',   range={5,100}, default=20.00, units='ms', save=false }

if wantSettings or config.autosave then
    SettingsLoad()
end

