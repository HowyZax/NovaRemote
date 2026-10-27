
function Scroller:create(parent)
    super(Scroller).create(self, parent)
end

function Scroller:onPaint(painter)
    painter:text{'Scroller', background='blue' }
end

