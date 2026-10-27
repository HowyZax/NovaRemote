require 'settings.classes'
require 'settings.storage'
require 'settings.menu'
require 'settings.settings'
require 'settings.edit'

if _SETTINGS then require(_SETTINGS)
elseif _APP  then require(_APP .. '.settings')
else              require 'remote.settings'
end

