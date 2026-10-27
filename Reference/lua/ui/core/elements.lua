
UIElement = Object:subclass('UIElement')
UIElement:attributes { 'id', 'visible', 'x', 'y', 'w', 'h', 'margin', 'padding', 'constrain', 'color', 'background' }

function UIElement:setVisible(visible)
    self.visible = visible
end

function UIElement:isVisible()
    return self.visible
end

function UIElement:setGeometry(r)
--  print(self.__classname, ':setGeometry', r)
    self.rect = r
end

function UIElement:onPaint(painter)
    for _,c in ipairs(self) do
        if c.visible == nil or c.visible then
            c:onPaint(painter)
        end
    end
end

LayoutElement = UIElement:subclass('LayoutElement')
LayoutElement:attributes { '[]', 'rows', 'columns' }
LayoutElement.margin = 0
function LayoutElement:layout(r)
    r = r or self.rect
    if r then
        local layout = Layout(r)
        layout:layoutChildren(self)
    end
end

GraphicElement = UIElement:subclass('GraphicElement')
GraphicElement:attributes { 'graphic' }
--GraphicElement.constrain = 1
function GraphicElement:setContent(content)
    self.graphic = content
end
GraphicElement.setGraphic = GraphicElement.setContent

TextElement = UIElement:subclass('TextElement')
TextElement:attributes { 'text', 'align', 'fit', 'color', 'font', 'overline' }
function TextElement:setContent(content)
    self.text = content
end
TextElement.setText = TextElement.setContent

function TextElement:setColor(color)    self.color = color end
function TextElement:setBackground(color) self.background = color end
function TextElement:setFont(font)      self.font = font end

RectElement = UIElement:subclass('RectElement')
RectElement:attributes { 'color' }
function RectElement:onPaint(painter)
    painter:fillRect(self.rect, self.color or 'blue')
end
