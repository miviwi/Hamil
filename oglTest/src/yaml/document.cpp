#include <yaml/document.h>
#include <util/format.h>

#include <yaml.h>

#include <cassert>
#include <cstdlib>
#include <unordered_map>
#include <sstream>

namespace yaml {

class Parser {
public:
  Parser();
  ~Parser();

  Parser& input(const char *input, size_t sz);

  Node::Ptr parse();

private:
  bool nextEvent();

  // returns a Node::Ptr based on current m_event
  Node::Ptr node();

  Node::Ptr scalar();
  Node::Ptr sequence();
  Node::Ptr mapping();

  Node::Ptr alias();

  Document::ParseError parseError() const;

  yaml_parser_t m_parser;
  yaml_event_t m_event;

  std::unordered_map<string, Node::Ptr> m_anchors;
};

Parser::Parser()
{
  if(!yaml_parser_initialize(&m_parser)) throw Document::Error(0, 0, "failed to initialie libyaml!");
}

Parser::~Parser()
{
  yaml_event_delete(&m_event);
  yaml_parser_delete(&m_parser);
}

Parser& Parser::input(const char *input, size_t sz)
{
  yaml_parser_set_input_string(&m_parser, (const unsigned char *)input, sz);
  return *this;
}

Node::Ptr Parser::parse()
{
  do {
    if(!nextEvent()) throw parseError();

    switch(m_event.type) {
    case YAML_NO_EVENT: break;

    case YAML_STREAM_START_EVENT: break;
    case YAML_DOCUMENT_START_EVENT:
      /* TODO: maybe use some of the info from the event? */
      return parse();

    default: return node();
    }

  } while(m_event.type != YAML_STREAM_END_EVENT);

  return Node::Ptr();
}

static const char *p_event_type_names[] = {
  /** An empty event. */
  "YAML_NO_EVENT",

  /** A STREAM-START event. */
  "YAML_STREAM_START_EVENT",
  /** A STREAM-END event. */
  "YAML_STREAM_END_EVENT",

  /** A DOCUMENT-START event. */
  "YAML_DOCUMENT_START_EVENT",
  /** A DOCUMENT-END event. */
  "YAML_DOCUMENT_END_EVENT",

  /** An ALIAS event. */
  "YAML_ALIAS_EVENT",
  /** A SCALAR event. */
  "YAML_SCALAR_EVENT",

  /** A SEQUENCE-START event. */
  "YAML_SEQUENCE_START_EVENT",
  /** A SEQUENCE-END event. */
  "YAML_SEQUENCE_END_EVENT",

  /** A MAPPING-START event. */
  "YAML_MAPPING_START_EVENT",
  /** A MAPPING-END event. */
  "YAML_MAPPING_END_EVENT"
};

bool Parser::nextEvent()
{
  auto result = yaml_parser_parse(&m_parser, &m_event);

#if 0
  if(m_event.type == YAML_SCALAR_EVENT)
    printf("type %s anchor %s tag %s value %s\n",
      p_event_type_names[m_event.type],
      m_event.data.scalar.anchor,
      m_event.data.scalar.tag,
      m_event.data.scalar.value);
  else if(m_event.type == YAML_MAPPING_START_EVENT || m_event.type == YAML_SEQUENCE_START_EVENT)
    printf("type %s anchor %s tag %s\n",
      p_event_type_names[m_event.type],
      m_event.data.mapping_start.anchor,
      m_event.data.mapping_start.tag);
#endif

  return result;
}

Node::Ptr Parser::node()
{
  switch(m_event.type) {
  case YAML_NO_EVENT:
    nextEvent();
    return node();

  case YAML_SCALAR_EVENT:         return scalar();

  case YAML_SEQUENCE_START_EVENT: return sequence();
  case YAML_MAPPING_START_EVENT:  return mapping();

  case YAML_ALIAS_EVENT:          return alias();

  default: assert(0); break;
  }

  return Node::Ptr(); // unreachable
}

Node::Ptr Parser::scalar()
{
  auto data = m_event.data.scalar;

  auto style = Node::Any;
  switch(data.style) {
  case YAML_SINGLE_QUOTED_SCALAR_STYLE: style = Node::SingleQuoted; break;
  case YAML_DOUBLE_QUOTED_SCALAR_STYLE: style = Node::DoubleQuoted; break;

  case YAML_FOLDED_SCALAR_STYLE:       style = Node::Folded; break;
  case YAML_LITERAL_SCALAR_STYLE:      style = Node::Literal; break;
  }

  auto scalar = new Scalar(data.value, data.length, data.tag ? (const char *)data.tag : Node::Tag(), style);
  auto ptr = Node::Ptr(scalar);

  if(data.anchor) m_anchors.insert({ data.anchor, ptr });

  return ptr;
}

Node::Ptr Parser::sequence()
{
  auto data = m_event.data.sequence_start;
  
  auto style = Node::Any;
  switch(data.style) {
  case YAML_BLOCK_SEQUENCE_STYLE: style = Node::Block; break;
  case YAML_FLOW_SEQUENCE_STYLE:  style = Node::Flow; break;
  }

  auto seq = data.tag ? new Sequence((const char *)data.tag, style) : new Sequence(std::nullopt, style);
  auto ptr = Node::Ptr(seq);

  if(data.anchor) m_anchors.insert({ data.anchor, ptr });

  do {
    if(!nextEvent()) throw parseError();

    switch(m_event.type) {
    case YAML_SEQUENCE_END_EVENT: return ptr;
    }

    auto item = node();
    seq->append(item);
  } while(m_event.type != YAML_STREAM_END_EVENT);

  return Node::Ptr(); // should never be reached
}

Node::Ptr Parser::mapping()
{
  auto data = m_event.data.mapping_start;

  auto style = Node::Any;
  switch(data.style) {
  case YAML_BLOCK_MAPPING_STYLE: style = Node::Block; break;
  case YAML_FLOW_MAPPING_STYLE:  style = Node::Flow; break;
  }

  auto map = data.tag ? new Mapping((const char *)data.tag, style) : new Mapping(std::nullopt, style);
  auto ptr = Node::Ptr(map);

  if(data.anchor) m_anchors.insert({ data.anchor, ptr });

  do {
    if(!nextEvent()) throw parseError();

    switch(m_event.type) {
    case YAML_MAPPING_END_EVENT: return ptr;
    }

    Node::Ptr key, value;

    key = node();
    nextEvent();
    value = node();

    map->append(key, value);
  } while(m_event.type != YAML_STREAM_END_EVENT);

  return Node::Ptr(); // shouldn never be reached
}

Node::Ptr Parser::alias()
{
  auto data = m_event.data.alias;

  auto it = m_anchors.find(data.anchor);
  if(it == m_anchors.end()) {
    auto line = m_parser.mark.line, column = m_parser.mark.column;

    throw Document::AliasError(line, column, util::fmt("anchor '%s' has not been defined", data.anchor));
  }

  return it->second;
}

Document::ParseError Parser::parseError() const
{
  auto line = m_parser.problem_mark.line, column = m_parser.problem_mark.column;

  return Document::ParseError(line, column, m_parser.problem);
}

class Emitter {
public:
  enum : size_t { InitialReserve = 256 };

  Emitter();
  ~Emitter();

  std::string emit(const Node::Ptr& node);

private:
  static int write_handler(void *data, byte *buf, size_t sz);

  void emitNode(const Node::Ptr& node);

  void emitScalar(const Scalar *scalar);
  void emitSequence(const Sequence *seq);
  void emitMapping(const Mapping *map);

  // cleans up 'event'
  void emitEvent(yaml_event_t *event);

  Document::EmitError emitError() const;

  yaml_emitter_t m_emitter;
};

Emitter::Emitter()
{
  if(!yaml_emitter_initialize(&m_emitter)) throw Document::Error(0, 0, "failed to initialize libyaml!");
}

Emitter::~Emitter()
{
  yaml_emitter_delete(&m_emitter);
}

std::string Emitter::emit(const Node::Ptr& node)
{
  if(!node) return "";

  std::string out;
  out.reserve(InitialReserve);

  yaml_emitter_set_output(&m_emitter, write_handler, &out);
  yaml_emitter_set_width(&m_emitter, 80);

  yaml_event_t event;

  yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
  emitEvent(&event);

  yaml_document_start_event_initialize(&event,
    nullptr,
    nullptr, nullptr,
    1);
  emitEvent(&event);

  emitNode(node);

  yaml_document_end_event_initialize(&event, 1);
  emitEvent(&event);

  yaml_stream_end_event_initialize(&event);
  emitEvent(&event);

  yaml_emitter_flush(&m_emitter);
  return out;
}

int Emitter::write_handler(void *data, byte *buf, size_t sz)
{
  auto out = (std::string *)data;
  out->append((const char *)buf, sz);

  return 1;
}

void Emitter::emitNode(const Node::Ptr& node)
{
  if(!node) return;

  switch(node->type()) {
  case Node::Scalar:   emitScalar(node->as<Scalar>()); break;
  case Node::Sequence: emitSequence(node->as<Sequence>()); break;
  case Node::Mapping:  emitMapping(node->as<Mapping>()); break;
  }
}

void Emitter::emitScalar(const Scalar *scalar)
{
  yaml_event_t event;

  bool has_tag = scalar->tag().has_value();

  yaml_scalar_style_t style = YAML_ANY_SCALAR_STYLE;
  switch(scalar->style()) {
  case Node::Any: break;

  case Node::Literal:      style = YAML_LITERAL_SCALAR_STYLE; break;
  case Node::Folded:       style = YAML_FOLDED_SCALAR_STYLE; break;
  case Node::SingleQuoted: style = YAML_SINGLE_QUOTED_SCALAR_STYLE; break;
  case Node::DoubleQuoted: style = YAML_DOUBLE_QUOTED_SCALAR_STYLE; break;

  default: throw Document::EmitError(m_emitter.line, m_emitter.column, "invalid scalar style!");
  }

  yaml_scalar_event_initialize(&event,
    nullptr,
    has_tag ? (yaml_char_t *)scalar->tag().value().data() : nullptr,
    (yaml_char_t *)scalar->str(), (int)scalar->size(),
    !has_tag, !has_tag,     // if the Node has a tag - don't omit it 
    style);
  emitEvent(&event);
}

void Emitter::emitSequence(const Sequence *seq)
{
  yaml_event_t event;

  yaml_sequence_style_t style = YAML_ANY_SEQUENCE_STYLE;
  switch(seq->style()) {
  case Node::Any: style = seq->size() > 2 ? YAML_ANY_SEQUENCE_STYLE : YAML_FLOW_SEQUENCE_STYLE; break;

  case Node::Flow:  style = YAML_FLOW_SEQUENCE_STYLE; break;
  case Node::Block: style = YAML_BLOCK_SEQUENCE_STYLE; break;

  default: throw Document::EmitError(m_emitter.line, m_emitter.column, "invalid sequence style!");
  }

  yaml_sequence_start_event_initialize(&event,
    nullptr,
    seq->tag() ? (yaml_char_t *)seq->tag().value().data() : nullptr,
    0,
    style);
  emitEvent(&event);

  for(const auto& node : *seq) emitNode(node);

  yaml_sequence_end_event_initialize(&event);
  emitEvent(&event);
}

void Emitter::emitMapping(const Mapping *map)
{
  yaml_event_t event;

  yaml_mapping_style_t style = YAML_ANY_MAPPING_STYLE;
  switch(map->style()) {
  case Node::Flow:  style = YAML_FLOW_MAPPING_STYLE; break;
  case Node::Block: style = YAML_BLOCK_MAPPING_STYLE; break;

  default: throw Document::EmitError(m_emitter.line, m_emitter.column, "invalid mapping style!");
  }

  yaml_mapping_start_event_initialize(&event,
    nullptr,
    map->tag() ? (yaml_char_t *)map->tag().value().data() : nullptr,
    0,
    style);
  emitEvent(&event);

  for(const auto& pair : *map) {
    emitNode(pair.first);   // key
    emitNode(pair.second);  // value
  }

  yaml_mapping_end_event_initialize(&event);
  emitEvent(&event);
}

void Emitter::emitEvent(yaml_event_t *event)
{
  if(!yaml_emitter_emit(&m_emitter, event)) throw emitError();
}

Document::EmitError Emitter::emitError() const
{
  return Document::EmitError(m_emitter.line, m_emitter.column, (const char *)m_emitter.problem);
}

std::string Document::Error::what() const
{
  return util::fmt("%lld, %lld: %s", line, column, reason.data());
}

Document::Document()
{
}

Document::Document(const Node::Ptr& root) :
  m_root(root)
{
}

Document Document::from_string(const char *doc, size_t len)
{
  return Document(Parser().input(doc, len).parse());
}

Document Document::from_string(const std::string& doc)
{
  return from_string(doc.data(), doc.length());
}

std::string Document::toString()
{
  return Emitter().emit(m_root);
}

Node::Ptr Document::get() const
{
  return m_root;
}

Node::Ptr Document::get(const std::string& what) const
{
  auto node = m_root;

  std::istringstream stream(what);
  std::string term;
  while(std::getline(stream, term, '.')) {
    char *end = nullptr;
    auto idx = strtoull(term.data(), &end, 0);

    if(end == term.data()+term.length()) { // 'term' was an index
      node = node->get(idx);
    } else {
      node = node->get(term);
    }

    if(!node) return Node::Ptr();
  }

  return node;
}

}