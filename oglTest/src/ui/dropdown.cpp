#include <ui/dropdown.h>

#include <algorithm>
#include <utility>

namespace ui {

DropDownFrame::~DropDownFrame()
{
}

bool DropDownFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  bool mouse_over = geometry().intersect(cursor.pos());
  if(!mouse_over && !m_dropped && m_state != Pressed) {
    m_ui->capture(nullptr);
    return false;
  }

  if(m_state == Default) {
    m_state = Hover;
    m_ui->capture(this);
  }

  auto mouse = input->get<win32::Mouse>();
  if(!mouse) return false;

  using win32::Mouse;
  if(m_dropped) {
    if(inputDropped(cursor.pos(), mouse)) return true;
  }

  if(mouse->event == Mouse::Down) {
    if(mouse_over && (mouse->buttons & Mouse::Left)) {
      m_state = Pressed;
      m_ui->keyboard(nullptr);
    } else {
      m_ui->capture(nullptr);
    }
  } else if(m_state == Pressed && mouse->buttonUp(Mouse::Left)) {
    if(mouse_over) {
      m_state = Hover;

      m_dropped = !m_dropped;
    } else if(m_dropped) {
      m_state = mouse_over ? Hover : Default;
    } else {
      m_ui->capture(nullptr);
    }
  }

  return true;
}

void DropDownFrame::paint(VertexPainter& painter, Geometry parent)
{
  const Style& style = m_ui->style();
  const auto& combobox = style.combobox;

  Geometry g = geometry();

  Geometry button_g = buttonGeometry();
  Geometry highlight_g = {
    g.x + (g.w-ButtonWidth)*0.02f, g.y + g.h*0.08f,
    (g.w-ButtonWidth)*0.96f, g.h*0.5f
  };
  auto luminance = combobox.color[1].luminance().r;
  byte factor = 0;
  switch(m_state) {
  case Default: factor = 0; break;
  case Pressed: factor = luminance; break;
  case Hover:   factor = luminance/4; break;
  }

  Color color[] = {
    combobox.color[0].lighten(factor),
    combobox.color[1].lighten(factor),
  };

  unsigned corners[2] = {
    VertexPainter::TopLeft|VertexPainter::BottomLeft,
    VertexPainter::TopRight|VertexPainter::BottomRight,
  };

  auto pipeline = gx::Pipeline()
    .scissor(m_ui->scissorRect(parent.clip(g)))
    .alphaBlend()
    .primitiveRestart(Vertex::RestartIndex)
    ;

  painter
    .pipeline(pipeline)
    .roundedRect({ g.x, g.y, g.w-ButtonWidth, g.h }, combobox.radius, corners[0], color[0])
    .roundedRect({ g.x+g.w-ButtonWidth, g.y, ButtonWidth, g.h }, combobox.radius, corners[1], color[0])
    .roundedRect(highlight_g, combobox.radius, VertexPainter::All, color[1], color[1])
    .line(button_g.pos(), { button_g.x, button_g.y+button_g.h }, 1, VertexPainter::CapButt, black(), black())
    .triangle(button_g.center(), 7, PIf, white())
    .roundedBorder(g, combobox.radius, VertexPainter::All, black())
    ;

  if(m_selected) painter.textLeft(*style.font, m_selected->caption, { g.x+10, g.y, g.w, g.h }, white());

  if(m_dropped) {
    Geometry dropdown_g = {
      g.x, g.y-10,
      g.w-ButtonWidth, m_items.size()*g.h + 20
    };

    Color dropdown_color = black().opacity(0.9);

    auto pipeline = gx::Pipeline()
      .scissor(m_ui->scissorRect(dropdown_g))
      .alphaBlend()
      .primitiveRestart(Vertex::RestartIndex)
      ;

    painter
      .beginOverlay()
      .pipeline(pipeline)
      .rect(dropdown_g, dropdown_color)
      ;

    for(unsigned i = 0; i < m_items.size(); i++) {
      const auto& item = m_items[i];
      Geometry item_g = itemGeometry(i);

      if(&item == m_selected) {
        Color c = color[1].lighten(item.id == m_highlighted ? 50 : 0).opacity(1);
        painter.rect(item_g, c);
      } else if(item.id == m_highlighted) {
        painter.rect(item_g, dropdown_color.lighten(80));
      }
      painter.textLeft(*style.font, item.caption, item_g.translate({ 10, 0 }), white());
    }

    painter.endOverlay();
  }
}

DropDownFrame& DropDownFrame::item(DropDownItem item)
{
  if(item.id == DropDownItem::Invalid) item.id = (DropDownItem::Id)m_items.size();

  m_items.push_back(std::move(item));

  return *this;
}

DropDownFrame& DropDownFrame::selected(DropDownItem::Id id)
{
  auto it = std::find_if(m_items.begin(), m_items.end(), [=](const auto& item) -> bool {
    return item.id == id;
  });

  m_selected = it != m_items.end() ? &(*it) : nullptr;
  m_highlighted = m_selected ? m_selected->id : DropDownItem::Invalid;
  m_on_change.emit(this);

  return *this;
}

DropDownItem::Id DropDownFrame::selected() const
{
  return m_selected ? m_selected->id : DropDownItem::Invalid;
}

DropDownFrame& DropDownFrame::onChange(OnChange::Slot on_change)
{
  m_on_change.connect(on_change);

  return *this;
}

DropDownFrame::OnChange& DropDownFrame::change()
{
  return m_on_change;
}

void DropDownFrame::losingCapture()
{
  m_state = Default;
  m_dropped = false;
}

vec2 DropDownFrame::sizeHint() const
{
  const ft::Font& font = *m_ui->style().font;
  float width = 110.0f;

  for(auto& item : m_items) {
    auto s = font.stringMetrics(item.caption);

    width = std::max(s.width(), width);
  }

  return { width+ButtonWidth+20, font.height()*1.4f };
}

Geometry DropDownFrame::buttonGeometry() const
{
  Geometry g = geometry();
  return {
    g.x+g.w - ButtonWidth, g.y,
    ButtonWidth, g.h
  };
}

Geometry DropDownFrame::itemGeometry(unsigned idx) const
{
  Geometry g = geometry();
  return {
    g.x, g.y + (idx*g.h),
    g.w-ButtonWidth, g.h
  };
}

bool DropDownFrame::inputDropped(vec2 mouse_pos, win32::Mouse *mouse)
{
  using win32::Mouse;

  unsigned i = 0;
  for(; i < m_items.size(); i++) {
    auto& item = m_items[i];
    Geometry item_g = itemGeometry(i);

    if(!item_g.intersect(mouse_pos)) continue;

    m_highlighted = item.id;
    if(mouse->buttonDown(Mouse::Left)) {
      return true;
    } else if(mouse->buttonUp(Mouse::Left)) {
      m_selected = &item;
      m_on_change.emit(this);
      
      return m_dropped = false;;
    }
    break;
  }
  if(i == m_items.size()) m_highlighted = DropDownItem::Invalid;

  return i != m_items.size();
}

}