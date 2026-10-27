--------------------------------------------------------------------
-- Core Lua code for generating Nomad user-interface.
--
-- This file does the work of generating the C code for the user interface.
-- The user interface definition is in file ui\ui.lua. The user interface
-- is described and constructed using Lua objects which are then converted
-- to C code in uitables.c. The generated code consists mostly of C structures
-- and  message handlers.
--
-- This file defines Lua class-like objects that are used in ui.lua to
-- construct the user interface. These objects include pages, controls
-- and message handlers that correspond to the equivalent C structures used
-- in the runtime code.
--
-- Note that the user interface can be defined directly in C, however doing
-- it via Lua is considerably less error-prone and allows the underlying
-- C structures to be changed easily. Additionally having the UI defined
-- in Lua allows various optimizations and operations to be handled at
-- compile-time instead of run-time.
--
-- Portions Copyright One-N Ltd 2007-2010
--
-- David Brenan <dbrenan@one-n.com>
--------------------------------------------------------------------

require 'class'
require 'object'
require 'rect'
require 'utils'

--------------------------------------------------------------------

require 'ui.core.layout'
require 'ui.core.elements'

--------------------------------------------------------------------
-- Import classes
--------------------------------------------------------------------
--
-- The classes model the equivalent C runtime structures
-- that represent various UI objects, including message handlers, controls
-- and pages.
--
require 'ui.core.control'
require 'ui.core.panel'
require 'ui.core.text'
require 'ui.core.title'
require 'ui.core.button'
require 'ui.core.timecode'
require 'ui.core.meter'
require 'ui.core.matrix'
require 'ui.core.faders'
require 'ui.core.battery'
require 'ui.core.scroller'
require 'ui.core.keyboard'
require 'ui.core.checkedgroup'
require 'ui.core.list'

require 'ui.core.page'

