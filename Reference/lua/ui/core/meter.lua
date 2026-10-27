
--------------------------------------------------------------------
-- Meter control
--
-- Represents a bank of N meters
--------------------------------------------------------------------

allMeters = { }

Meter = Control:subclass('Meter')
Meter:attributes{ 'vertical', 'source', 'scale' }
Meter.margin = 0

function Meter:__init(items)
    Control.__init(self, items)
    self.nmeters = items.nmeters or items[1] or 1
    self.vertical = items.vertical
    
    if self.scale == nil then  -- show meter scale text?
        self.scale = self.vertical or false 
    end
    
    if not self.titles then
        self.titles = { }
        for i = 1,self.nmeters do self.titles[i] = i end
    end

    if not self.source then     -- create a private MeterState if not using a named source
        self.state = MeterState{ } 
    end
    
    assert(#self.titles == 0 or #self.titles == self.nmeters)
    table.insert(allMeters,  self)
end

MeterScale = Control:subclass('MeterScale')
MeterScale:attributes{ 'vertical' }
MeterScale.margin = 0

