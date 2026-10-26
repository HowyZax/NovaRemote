--require 'debugger'
-----------------------------------------------------------------------------
_APP = 'remote'
_UI_CFG = 'ui.cfg'
_SETTINGS_CFG = 'settings.cfg'

local config = require 'config'

require 'settings'              -- main settings classes
require 'ui'

-----------------------------------------------------------------------------
require 'remote.settings'
require 'remote.timecode'
require 'remote.ui'

-------------------------------------------------------------------
-- Specify names of editor pages in settings classes

SettingNumber.editor = 'NumberEditor'
SettingText.editor = 'TextEditor'
SettingTimecode.editor = 'TimecodeEditor'
SettingUserbits.editor = 'UserbitsEditor'
-------------------------------------------------------------------

if config.movieslate then require 'movieslate' end
require 'serial.slave'

require 'strict'
for _,page in pairs(pages) do
    page:create()
end

-------------------------------------------------------------------
if config.fullscreen then
    mainWindow:show('fullscreen')
end

--pages['MainMenu']:setHome()
--restoreWindowPositions(pages['MainMenu'])
pages['Home']:setHome()
if _TARGET == 'qt' then
    restoreWindowPositions(pages['Home'])
else
    pages['Home']:enter()
end


-------------------------------------------------------------------


