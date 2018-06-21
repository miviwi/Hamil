#pragma once

#include <common.h>

#include <yaml/node.h>

#include <string>
#include <memory>

namespace yaml {

class Document {
public:
  static Document from_string(const std::string& doc);

private:
  Node::Ptr m_root;
};

}