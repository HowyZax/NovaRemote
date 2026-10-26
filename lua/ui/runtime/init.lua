assert(_TARGET, "_TARGET must be defined (qt or zx)")

require 'ui.runtime.object'
require 'ui.runtime.page'
require 'ui.runtime.control'
require 'ui.runtime.button'
require 'ui.runtime.text'
require 'ui.runtime.title'
require 'ui.runtime.matrix'
require 'ui.runtime.meter'
require 'ui.runtime.faders'
require 'ui.runtime.timecode'
require 'ui.runtime.panel'
require 'ui.runtime.battery'
require 'ui.runtime.scroller'

-- Load runtime customizations for intended target (Qt or ZX)
print('Loading uiruntime customizations for target: ui' .. _TARGET)
require('ui.runtime.'.._TARGET)

