#include <ui/console.h>
#include <ui/animation.h>

#include <win32/input.h>

namespace ui {

class ConsoleBufferFrame : public Frame {
public:
  using LineBuffer = std::deque<std::string>;

  static constexpr float BufferHeight = 290.0f,
    BufferPixelMargin = 5.0f;

  enum : size_t {
    CursorNotSet = ~0u,

    BufferDepth = 512,
  };

  using Frame::Frame;

  virtual bool input(CursorDriver& cursor, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  virtual vec2 sizeHint() const;

  void submitCommand(TextBoxFrame *prompt);

  void print(const std::string& str);
  void clear();

  std::string historyPrevious();
  std::string historyNext();

private:
  LineBuffer m_buffer;
  size_t m_cursor = CursorNotSet;
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

  m_prompt->onSubmit([this](auto target)
  {
    m_on_command.emit(this, target->text().c_str());
    m_buffer->submitCommand(target);
  });

  m_buffer->print("putting some text into the buffer...");
  m_buffer->print("some more text");
  m_buffer->print("wow such text (!)");
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
  m_dropdown.stop();
}

ConsoleFrame& ConsoleFrame::toggle()
{
  return dropped(!m_dropped);
}

ConsoleFrame& ConsoleFrame::dropped(bool val)
{
  m_dropped = val;
  m_dropdown.start();

  return *this;
}

ConsoleFrame& ConsoleFrame::print(const char *str)
{
  m_buffer->print(str);

  return *this;
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

bool ConsoleBufferFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  return false;
}

void ConsoleBufferFrame::paint(VertexPainter& painter, Geometry parent)
{
  auto& font = *m_ui->style().monospace;

  Geometry g = geometry();

  auto pipeline = gx::Pipeline()
    .scissor(Ui::scissor_rect(parent.clip(g)))
    .alphaBlend()
    .primitiveRestart(0xFFFF)
    ;

  painter
    .pipeline(pipeline)
    .rect(g, black().opacity(0.8))
    ;

  if(m_buffer.empty()) return;

  float line_height = font.height();

  vec2 pos = g.pos() + vec2{
    BufferPixelMargin,
    g.h - (font.ascender() + BufferPixelMargin)
  };
  for(auto& line : m_buffer) {
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

void ConsoleBufferFrame::submitCommand(TextBoxFrame *prompt)
{
  print(prompt->text());

  prompt->text("");
}

void ConsoleBufferFrame::print(const std::string& str)
{
  m_buffer.push_front(str);
  if(m_buffer.size() > BufferDepth) m_buffer.pop_back();

  m_cursor = CursorNotSet;
}

void ConsoleBufferFrame::clear()
{
  m_buffer.clear();
  m_cursor = CursorNotSet;
}

std::string ConsoleBufferFrame::historyPrevious()
{
  if(m_cursor == CursorNotSet) {
    m_cursor = 0;
  } else if(m_cursor < m_buffer.size()) {
    m_cursor++;
    return m_buffer[m_cursor];
  }

  return "";
}

std::string ConsoleBufferFrame::historyNext()
{
  if(m_cursor == CursorNotSet) {
    m_cursor = m_buffer.size()-1;
  } else if(m_cursor > 0) {
    m_cursor--;
    return m_buffer[m_cursor];
  }

  return "";
}

}