--------------------------------------------------------------------
-- Battery control
--
-- Represents a battery indicator, as used on the home pages
--------------------------------------------------------------------
Battery = Button:subclass('Battery')

function Battery:__init(items)
    super(Battery).__init(self, items)
end

