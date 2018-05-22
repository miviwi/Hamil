#include <ui/console.h>

#include <win32/input.h>

namespace ui {

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
}

ConsoleFrame::ConsoleFrame(Ui& ui) :
  ConsoleFrame(ui, "")
{
}

ConsoleFrame::~ConsoleFrame()
{
}

bool ConsoleFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  if(auto kb = input->get<win32::Keyboard>()) {
    if(!kb->special()) return m_prompt->input(cursor, input);

    using win32::Keyboard;

    std::string s;
    
    switch(kb->key) {
    case Keyboard::Up:   s = m_buffer->historyPrevious(); break;
    case Keyboard::Down: s = m_buffer->historyNext(); break;

    default: return m_prompt->input(cursor, input);
    }

    m_prompt->text(s);
    return true;
  }

  return m_prompt->input(cursor, input);
}

void ConsoleFrame::paint(VertexPainter& painter, Geometry parent)
{
  m_console->paint(painter, geometry());
}

void ConsoleFrame::losingCapture()
{
  m_console->losingCapture();
}

void ConsoleFrame::attached()
{
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

Geometry ConsoleFrame::make_geometry()
{
  float hcenter = (1.0f/2.0f)*(FramebufferSize.x - ConsoleSize.x);

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

  vec2 pos = {
    g.x + BufferPixelMargin,
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
  m_buffer.push_front(prompt->text());
  if(m_buffer.size() > BufferDepth) m_buffer.pop_back();

  m_cursor = -1;

  prompt->text("");
}

const std::string&  ConsoleBufferFrame::historyPrevious()
{
  if(m_cursor < 0) {
    m_cursor = 0;
  }

  return m_buffer.front();
}

const std::string& ConsoleBufferFrame::historyNext()
{
  if(m_cursor < 0) {
    m_cursor = (int)m_buffer.size() - 1;
  }

  return m_buffer.back();
}

}