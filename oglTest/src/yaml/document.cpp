#include <yaml/document.h>

#include <cassert>

#include <yaml.h>

namespace yaml {

class Parser {
public:
  struct Error { };

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

  yaml_parser_t m_parser;
  yaml_event_t m_event;
};

Parser::Parser()
{
  if(!yaml_parser_initialize(&m_parser)) throw Error();
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
    if(!nextEvent()) throw Error();

    switch(m_event.type) {
    case YAML_NO_EVENT: break;

    case YAML_STREAM_START_EVENT: break;
    case YAML_DOCUMENT_START_EVENT:
      /* TODO: maybe use some of the info from the event? */
      return parse();

    default: return node();
    }

  } while(m_event.type != YAML_STREAM_END_EVENT);
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
  return yaml_parser_parse(&m_parser, &m_event)
    || 0 && printf("event %s\n", p_event_type_names[m_event.type]);
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
  return std::make_shared<Scalar>(m_event.data.scalar.value, m_event.data.scalar.length);
}

Node::Ptr Parser::sequence()
{
  auto seq = new Sequence();
  do {
    if(!nextEvent()) throw Error();

    switch(m_event.type) {
    case YAML_SEQUENCE_END_EVENT: return Node::Ptr(seq);
    }

    auto item = node();
    seq->append(item);
  } while(m_event.type != YAML_STREAM_END_EVENT);

  return Node::Ptr(); // shouldn't be reached
}

Node::Ptr Parser::mapping()
{
  auto map = new Mapping();
  do {
    if(!nextEvent()) throw Error();

    switch(m_event.type) {
    case YAML_MAPPING_END_EVENT: return Node::Ptr(map);
    }

    Node::Ptr key, value;

    key = node();
    nextEvent();
    value = node();

    map->append(key, value);
  } while(m_event.type != YAML_STREAM_END_EVENT);

  return Node::Ptr(); // shouldn't be reached
}

Node::Ptr Parser::alias()
{
  printf("got alias\n");
  return Node::Ptr();
}

Document Document::from_string(const std::string& doc)
{
  Document document;
  document.m_root = Parser().input(doc.c_str(), doc.length()).parse();

  puts(document.m_root->repr().c_str());

  auto e = document.m_root->as<Mapping>()->get(std::make_shared<Scalar>("e"));
  puts(e->repr().c_str());

  return document;

}

}