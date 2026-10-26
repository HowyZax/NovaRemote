--{{{ Faders
function Faders:create(parent)
    --print('','', 'Faders.create in parent', parent.__classname)
    tprint(widgets)
    local w = widgets.control(parent)
    self.widget = w
    w:setOwner(self)
    return w
end
--}}}

