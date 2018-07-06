#include <ui/console.h>
#include <ui/animation.h>

#include <util/format.h>
#include <win32/input.h>

#include <unordered_map>
#include <functional>
#include <sstream>

namespace ui {

class ConsoleBufferFrame : public Frame {
public:
  using LineBuffer = std::deque<std::string>;

  static constexpr float BufferHeight = ConsoleFrame::ConsoleSize.y - 30.0f,
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

  LineBuffer m_buffer;
  LineBuffer m_input;
  size_t m_history = CursorNotSet;
  size_t m_scroll = 0;
};

ConsoleFrame::ConsoleFrame(Ui& ui, const char *name) :
  Frame(ui, name, make_geometry()),
  m_prompt(new TextBoxFrame(ui)), m_buffer(new ConsoleBufferFrame(ui))
{
  m_prompt->hint("Type commands here...");

  auto& console = create<RowLayoutFrame>(ui, name, geometry())
    .frame(m_buffer)
    .frame(m_prompt)
    ;
    
  m_console = &console;

  m_prompt->onSubmit([this](TextBoxFrame *target) {
    const auto& cmd = target->text();

    m_buffer->input(cmd);
    if(cmd.length() && cmd.front() == '$') {
      consoleCommand(cmd);
    } else {
      m_on_command.emit(this, cmd.data());
    }

    target->text("");
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

    std::string s;
    
    switch(kb->key) { // TODO: work out how to make this reachable when m_prompt has keyboard...
    case Keyboard::Up:   s = m_buffer->historyPrevious(); break;
    case Keyboard::Down: s = m_buffer->historyNext(); break;

    default: return m_console->input(cursor, input);
    }

    m_prompt->text(s);
    return true;
  }

  return m_console->input(cursor, input);
}

void ConsoleFrame::paint(VertexPainter& painter, Geometry parent)
{
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
    { "$cls", [this]() { m_buffer->clear(); } },
  };
  fns[cmd]();
}

bool ConsoleBufferFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  if(auto mouse = input->get<win32::Mouse>()) {
    using win32::Mouse;
    if(mouse->event != Mouse::Wheel) return false;

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
  auto& font = *m_ui->style().monospace;

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
    .scissor(Ui::scissor_rect(parent.clip(g)))
    .alphaBlend()
    .primitiveRestart(0xFFFF)
    ;

  painter
    .pipeline(pipeline)
    .rect(g, black().opacity(0.8))
    .rect(scrollbar_g, grey().opacity(0.8))
    ;

  if(m_buffer.empty()) return;

  float line_height = font.height();

  vec2 pos = g.pos() + vec2{
    BufferPixelMargin,
    g.h - (font.ascender() + BufferPixelMargin)
  };
  for(auto it = m_buffer.begin()+m_scroll; it != m_buffer.end(); it++) {
    const auto& line = *it;
    painter
      .text(font, line, pos, white())
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
  auto wrap_limit = columns();
  
  std::istringstream stream(str);
  std::string line;
  while(std::getline(stream, line)) {
    if(line.length() > wrap_limit) {
      util::linewrap(line, wrap_limit, [this](const std::string& str, size_t line_no) {
        m_buffer.push_front(str);
      });
    } else {
      m_buffer.push_front(line);
    }
  }

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
    m_history = 0;
  } else if(m_history+1 < m_input.size()) {
    m_history++;
    return m_input[m_history];
  }

  return "";
}

std::string ConsoleBufferFrame::historyNext()
{
  if(m_history == CursorNotSet) {
    m_history = m_input.size()-1;
  } else if(m_history > 0) {
    m_history--;
    return m_input[m_history];
  }

  return "";
}

size_t ConsoleBufferFrame::rows() const
{
  const auto& font = *m_ui->style().monospace;
  auto height = BufferHeight - BufferPixelMargin;

  return (size_t)(height / font.height());
}

size_t ConsoleBufferFrame::columns() const
{
  const auto& font = *m_ui->style().monospace;
  auto width = ConsoleFrame::ConsoleSize.x - (BufferPixelMargin+ScrollBarWidth);

  return (size_t)floor(width / font.monospaceWidth())-1;
}

}