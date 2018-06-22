#pragma once

#include <common.h>

#include <yaml/node.h>

#include <string>
#include <memory>
#include <utility>

namespace yaml {

class Document {
public:
  struct Error {
  public:
    Error(size_t line_, size_t column_, std::string&& reason_) :
      line(line_+1), column(column_+1), reason(std::move(reason_)) /* yaml_mark_t values are 0-based */
    { }

    std::string what() const;

    const size_t line, column;
    const std::string reason;
  };

  struct ParseError : public Error {
  public:
    using Error::Error;
  };

  struct AliasError : public Error {
  public:
    using Error::Error;
  };

  Document();

  static Document from_string(const char *doc, size_t len = 0);
  static Document from_string(const std::string& doc);

  // returns the root element
  Node::Ptr get() const;

  // 'what' can contain either mapping keys or integer indices
  // separated by '.' (dots) to signify nested elements
  Node::Ptr get(const std::string& what) const;
  Node::Ptr operator()(const std::string& what) const { return get(what); }

private:
  Node::Ptr m_root;
};

}