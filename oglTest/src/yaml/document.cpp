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

  default: assert(1); break;
  }

  return Node::Ptr(); // unreachable
}

Node::Ptr Parser::scalar()
{
  auto data = m_event.data.scalar;

  auto scalar = new Scalar(data.value, data.length, data.tag ? (const char *)data.tag : Node::Tag());
  auto ptr = Node::Ptr(scalar);

  if(data.anchor) m_anchors.insert({ data.anchor, ptr });

  return ptr;
}

Node::Ptr Parser::sequence()
{
  auto data = m_event.data.sequence_start;

  auto seq = data.tag ? new Sequence((const char *)data.tag) : new Sequence();
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

  auto map = data.tag ? new Mapping((const char *)data.tag) : new Mapping();
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

std::string Document::Error::what() const
{
  return util::fmt("%lld, %lld: %s", line, column, reason.c_str());
}

Document::Document()
{
}

Document Document::from_string(const char *doc, size_t len)
{
  Document document;
  document.m_root = Parser().input(doc, len ? len : strlen(doc)).parse();

 // puts(document.m_root->repr().c_str());

  return document;
}

Document Document::from_string(const std::string& doc)
{
  return from_string(doc.c_str(), doc.length());
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
    auto idx = strtoull(term.c_str(), &end, 0);

    if(end == term.c_str()+term.length()) { // 'term' was an index
      node = node->get(idx);
    } else {
      node = node->get(term);
    }

    if(!node) return Node::Ptr();
  }

  return node;
}

}