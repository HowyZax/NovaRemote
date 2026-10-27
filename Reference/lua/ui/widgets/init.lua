_TARGET = _TARGET or 'qt'

assert(_TARGET, "_TARGET must be defined (qt or zx)")

widgets = require('ui.widgets.' .. _TARGET)

return widgets

