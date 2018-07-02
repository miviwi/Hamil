#pragma once

#include <yaml/document.h>
#include <yaml/node.h>

#include <string>
#include <vector>
#include <initializer_list>
#include <utility>
#include <optional>

namespace yaml {

class SchemaCondition {
public:
  using Ptr = std::shared_ptr<SchemaCondition>;

  virtual bool validate(const Node::Ptr& doc) = 0;
};

class ScalarCondition : public SchemaCondition {
public:
  // when 'type' is not given a value any Scalar will pass validation
  ScalarCondition(Scalar::DataType type = Scalar::Invalid);

  virtual bool validate(const Node::Ptr& node);

private:
  Scalar::DataType m_type;
};

class SequenceCondition : public SchemaCondition {
public:
  virtual bool validate(const Node::Ptr& node);
};

class MappingCondition : public SchemaCondition {
public:
  virtual bool validate(const Node::Ptr& node);
};

// yaml::Document auto-validation, usage:
//   auto schema =
//     Schema()
//       .scalar("a", Scalar::Int)
//       .sequence("b");
//
//   if(schema.validate(...)) panic();
class Schema {
public:
  // returns a pointer to the first Node which failed validation
  // or an empty Node::Ptr if validation succeded
  Node::Ptr validate(const Document& doc);

  Schema& scalar(const std::string& node, Scalar::DataType type);
  Schema& scalar(const std::string& node);
  Schema& sequence(const std::string& node);
  Schema& mapping(const std::string& node);

private:
  Schema& condition(const std::string& node, SchemaCondition *cond);

  std::vector<std::pair<std::string, SchemaCondition::Ptr>> m_conditions;
};

}