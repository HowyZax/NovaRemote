local which    = 'tablet'
local debug    = true  -- cleanme
local platform = 'windows'
local product  = nil

-- Choose product type:   Uncommment following line for Nomad TOUCH which is a full screen application
product = 'nova'

--------------------------------------------------------------------------------
if applicationFileName then
    print (applicationFileName)
    if     _APP_PATH:lower():find('oasis.*%.app/contents/macos')  then which   = 'surface'
    elseif _APP_PATH:lower():find('deva32.*%.app/contents/macos') then which   = 'desktop'
    elseif _APP_PATH:lower():find('nova.*%.app/contents/macos')   then product = 'nova'
    elseif _APP_PATH:lower():find('contents/macos')               then which   = 'desktop'
    elseif applicationFileName:lower():find('oasis')              then which   = 'surface'
    elseif applicationFileName:lower():find('deva32')             then which   = 'desktop'
    elseif applicationFileName:lower():find('nova')               then product = 'nova'
    end

    if _APP_PATH:lower():find('contents/macos') then platform = 'mac' end

end

--------------------------------------------------------------------------------
if product then
    if product == 'nova' then 
        which = 'tablet'
    end
end


--[[
local f = io.open('license.dat')
if f then
    which = f:read('l')
    f:close()
end
--]]

--------------------------------------------------------------------------------
-- Defaults
-- @see sources.lua for sources configuration
local config = {
    tablet     = (which=='tablet'),
    surface    = (which=='surface'),
    version    = '0.92.10',
    debug      = debug,

    bonjour    = (os.platform == 'windows'),
    movieslate = (os.platform == 'windows'),
    recorder   = (os.platform == 'windows'),
    player     = (os.platform == 'windows'),
    scriptview = (os.platform == 'windows'),
    dante      = (os.platform == 'windows'),
    panfollow  = true,
    ntracks    = 8,
    noutputs   = 8,

    rx12        = true,
    tapemix     = true,
    monomix     = true,
    hp2mix      = true,
    faderassign = (which ~= 'tablet'),
    meterbridge = (which ~= 'tablet'),
    slate       = (which == 'tablet'),
    camera      = (which == 'tablet'),
    zaxnet      = true,
    autosave    = false,
    uicfg       = (which ~= 'tablet')
}

--------------------------------------------------------------------------------
if product and product == 'nova' then
    config.nova  = true
    config.nomad = true     -- Nova is derived from Nomad, so uses most Nomad features
    config.fullscreen = false
end

if config.nomad == nil then
    config.nomad = config.tablet or config.surface or config.nova
end

if config.fullscreen == nil then
    config.fullscreen = config.tablet
end

--------------------------- NOVA SETUP -----------------------------------------------------
-- Features
if config.nova then
    assert(config.nomad)    -- Nova is specialization of Nomad
    config.bonjour     = false
    config.movieslate  = false
    config.recorder    = (os.platform == 'windows')
    config.player      = false
    config.scriptview  = false
    config.dante       = false
    config.panfollow   = false
    config.rx12        = false
    config.tapemix     = false
    config.monomix     = false
    config.hp2mix      = false
    config.faderassign = false
    config.meterbridge = false
    config.slate       = false
    config.camera      = false
    config.zaxnet      = false

-- config.surface = true -- cleanme see if oasis works with nova build

    config.ntracks  = 16 -- was 20
    config.noutputs = 8

    config.autosave = true      -- true to auto-load/save local settings at startup & shutdown
    config.uicfg = true         -- true to support button to save window layout (file ui.cfg)
elseif config.nomad then
    config.ntracks  = 12
    config.noutputs = 6
end

return config
