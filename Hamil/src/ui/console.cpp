#include <ui/console.h>
#include <ui/drawable.h>
#include <ui/animation.h>

#include <util/format.h>
#include <util/str.h>
#include <win32/input.h>

#include <unordered_map>
#include <functional>
#include <sstream>

namespace ui {

class ConsoleBufferFrame : public Frame {
public:
  using LineBuffer   = std::deque<std::string>;
  using RenderBuffer = std::deque<Drawable>;

  static constexpr float BufferHeight = ConsoleFrame::ConsoleSize.y - 20.0f,
    BufferPixelMargin = 5.0f,
    ScrollBarWidth = 5.0f;

  enum : size_t {
    CursorNotSet = ~0u,

    BufferDepth = 512,
  };

  using Frame::Frame;

  virtual bool input(CursorDriver& cursor, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  virtual vec2 sizeHint() const;

  void print(const std::string& str);
  void input(const std::string& str);
  void clear();

  std::string historyPrevious();
  std::string historyNext();

private:
  size_t rows() const;
  size_t columns() const;

  RenderBuffer m_buffer;
  LineBuffer m_input;
  size_t m_history = CursorNotSet;
  size_t m_scroll = 0;
};

ConsoleFrame::ConsoleFrame(Ui& ui, const char *name) :
  Frame(ui, name, make_geometry()),
  m_prompt(new TextBoxFrame(ui)), m_buffer(new ConsoleBufferFrame(ui))
{
  m_prompt->font(ui.style().monospace);
  m_prompt->hint("Type commands here...");

  auto& console = create<RowLayoutFrame>(ui, name, geometry())
    .frame(m_buffer)
    .frame(m_prompt)
    ;
    
  m_console = &console;

  m_prompt->onSubmit([this](TextBoxFrame *target) {
    const auto& cmd = target->text();

    m_buffer->input(cmd);
    if(cmd.length() && cmd.front() == '.') {
      consoleCommand(cmd);
    } else {
      m_on_command.emit(this, cmd.data());
    }

    target->text("");
  });

  m_prompt->onKeyDown([this](TextBoxFrame *target, const InputPtr& input) {
    specialKey(input);
  });
}

ConsoleFrame::ConsoleFrame(Ui& ui) :
  ConsoleFrame(ui, "")
{
}

ConsoleFrame::~ConsoleFrame()
{
  delete m_console;
}

bool ConsoleFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  if(auto kb = input->get<win32::Keyboard>()) {
    using win32::Keyboard;
    if(!kb->special() || kb->event != Keyboard::KeyDown) return m_console->input(cursor, input);

    return specialKey(input);
  }

  return m_console->input(cursor, input);
}

void ConsoleFrame::paint(VertexPainter& painter, Geometry parent)
{
  if(!m_dropped && m_dropdown.done()) return;

  auto y = m_dropdown.done() ? 0.0f : m_dropdown.channel<float>(0);
  m_console->position({
    make_geometry().x,
    m_dropped ? y : (-ConsoleSize.y - y)
  });

  m_console->paint(painter, geometry());
}

void ConsoleFrame::losingCapture()
{
  m_console->losingCapture();
}

void ConsoleFrame::attached()
{
}

ConsoleFrame& ConsoleFrame::toggle()
{
  return dropped(!m_dropped);
}

ConsoleFrame& ConsoleFrame::dropped(bool val)
{
  if(m_dropped == val) return *this;

  m_dropped = val;
  m_dropdown.start();

  return *this;
}

ConsoleFrame& ConsoleFrame::print(const char *str)
{
  m_buffer->print(str);

  return *this;
}

ConsoleFrame& ConsoleFrame::print(const std::string& str)
{
  return print(str.data());
}

ConsoleFrame& ConsoleFrame::clear()
{
  m_buffer->clear();

  return *this;
}

ConsoleFrame& ConsoleFrame::onCommand(OnCommand::Slot on_command)
{
  m_on_command.connect(on_command);

  return *this;
}

ConsoleFrame::OnCommand& ConsoleFrame::command()
{
  return m_on_command;
}

vec2 ConsoleFrame::sizeHint() const
{
  return ConsoleSize;
}

constexpr Geometry ConsoleFrame::make_geometry()
{
  float hcenter = (FramebufferSize.x - ConsoleSize.x)/2.0f;

  return {
    hcenter, 0,
    ConsoleSize
  };
}

void ConsoleFrame::consoleCommand(const std::string& cmd)
{
  std::unordered_map<std::string, std::function<void()>> fns = {
    { ".cls", [this]() { m_buffer->clear(); } },
  };
  fns[cmd]();
}

bool ConsoleFrame::specialKey(const InputPtr& input)
{
  using win32::Keyboard;
  auto kb = input->get<Keyboard>();

  std::string s;
  switch(kb->key) {
  case Keyboard::Up:   s = m_buffer->historyPrevious(); break;
  case Keyboard::Down: s = m_buffer->historyNext(); break;

  default: return false;
  }

  m_prompt->text(s);
  return true;
}

bool ConsoleBufferFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  if(auto mouse = input->get<win32::Mouse>()) {
    using win32::Mouse;
    if(mouse->event != Mouse::Wheel) return false;

    if(m_buffer.size() < rows()) return true; // don't scroll if the screen
                                              // hasn't been filled yet

    auto direction = mouse->ev_data > 0 ? 1 : -1;
    if(direction > 0 && m_buffer.size()) {
      auto max_scroll = m_buffer.size() < rows() ? m_buffer.size() : m_buffer.size()-rows();
      if(m_scroll >= 0 && m_scroll < max_scroll) m_scroll += direction;
    } else {
      if(m_scroll > 0) m_scroll += direction;
    }

    return true;
  }

  return false;
}

void ConsoleBufferFrame::paint(VertexPainter& painter, Geometry parent)
{
  auto& font = *ui().style().monospace;

  Geometry g = geometry();

  float content_height = (float)m_buffer.size(),
    visible_content = std::min(rows() / content_height, 1.0f),
    scrollbar_height = std::max(visible_content, 0.1f) * BufferHeight,
    scrollbar_position = visible_content*font.height()*(float)m_scroll;
  Geometry scrollbar_g = {
    g.x + (g.w-ScrollBarWidth), g.y + (g.h-scrollbar_position-scrollbar_height-BufferPixelMargin),
    ScrollBarWidth, scrollbar_height,
  };

  auto pipeline = gx::Pipeline()
    .scissor(ui().scissorRect(parent.clip(g)))
    .alphaBlend()
    .primitiveRestart(Vertex::RestartIndex)
    ;

  painter
    .pipeline(pipeline)
    .rect(g, black().opacity(0.8))
    .rect(scrollbar_g, grey().opacity(0.8))
    ;

  // Paint the text
  if(m_buffer.empty()) return;

  float line_height = font.height();

  vec2 pos = g.pos() + vec2{
    BufferPixelMargin,
    g.h - (font.ascender() + BufferPixelMargin)
  };

  auto end = std::min(m_scroll + rows(), m_buffer.size());
  for(size_t i = m_scroll; i < end; i++) {
    const auto& line = m_buffer.at(i);
    painter
      .drawable(line, pos)
      ;

    pos.y -= line_height;
  }
}

vec2 ConsoleBufferFrame::sizeHint() const
{
  return { 0, BufferHeight };
}

void ConsoleBufferFrame::print(const std::string& str)
{
  auto font = ui().style().monospace;
  auto wrap_limit = columns();
  
  util::splitlines(str, [&](const std::string& line) {
    util::linewrap(line, wrap_limit, [this,&font](const std::string& str, size_t line_no) {
      auto text = ui().drawable().fromText(font, str, white());

      m_buffer.push_front(text);
    });
  });

  while(m_buffer.size() > BufferDepth) m_buffer.pop_back();

  m_history = CursorNotSet;
  m_scroll = 0;
}

void ConsoleBufferFrame::input(const std::string& str)
{
  print(str);
  if(m_buffer.size() > BufferDepth) m_input.pop_back();

  m_input.push_front(str);
}

void ConsoleBufferFrame::clear()
{
  m_buffer.clear();
  m_input.clear();
  m_history = CursorNotSet;
  m_scroll = 0;
}

std::string ConsoleBufferFrame::historyPrevious()
{
  if(m_history == CursorNotSet) {
    if(m_input.empty()) return "";
    
    m_history = 0;
    return m_input.front();
  } 

  std::string s = m_input[m_history];
  m_history = clamp((int)m_history+1, 0, (int)m_input.size()-1);

  return s;
}

std::string ConsoleBufferFrame::historyNext()
{
  if(m_history == CursorNotSet) {
    return "";
  }

  std::string s = m_input[m_history];
  m_history = clamp((int)m_history-1, 0, (int)m_input.size()-1);

  return s;
}

size_t ConsoleBufferFrame::rows() const
{
  const auto& font = *ui().style().monospace;
  auto height = BufferHeight - BufferPixelMargin;

  return (size_t)(height / font.height());
}

size_t ConsoleBufferFrame::columns() const
{
  const auto& font = *ui().style().monospace;
  auto width = ConsoleFrame::ConsoleSize.x - (BufferPixelMargin+ScrollBarWidth);

  return (size_t)floor(width / font.monospaceWidth())-1;
}

}