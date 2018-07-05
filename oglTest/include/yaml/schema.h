#pragma once

#include <yaml/document.h>
#include <yaml/node.h>

#include <string>
#include <vector>
#include <initializer_list>
#include <utility>
#include <regex>
#include <optional>

namespace yaml {

enum Flags : unsigned {
  Default = 0,
  Optional = 1<<0,
};

class SchemaCondition {
public:
  using Ptr = std::shared_ptr<SchemaCondition>;

  SchemaCondition(Flags flags);

  bool validate(const Node::Ptr& node) const;

protected:
  // only called with a valid Node
  virtual bool doValidate(const Node::Ptr& node) const = 0;

private:
  Flags m_flags;
};

class ScalarCondition : public SchemaCondition {
public:
  // when 'type' is not given a value any Scalar will pass validation
  ScalarCondition(Flags flags = Default, Scalar::DataType type = Scalar::Invalid);

protected:
  virtual bool doValidate(const Node::Ptr& node) const;

private:
  Scalar::DataType m_type;
};

class RegexCondition : public SchemaCondition {
public:
  RegexCondition(Flags flags = Default);

protected:
  virtual bool doValidate(const Node::Ptr& node) const;

  virtual const std::regex& regex() const = 0;
};

// Matches a path of the form
//   /<dir>/<dir>/<file>
//   /<dir>/<dir>/
// where <dir> and <file> include any valid win32
// path characters except spaces
// 
// Or an empty string
class PathCondition : public RegexCondition {
public:
  using RegexCondition::RegexCondition;

protected:
  virtual const std::regex& regex() const;
};

// Matches a valid win32 file name with no spaces
class FileCondition : public RegexCondition {
public:
  using RegexCondition::RegexCondition;

protected:
  virtual const std::regex& regex() const;
};

class SequenceCondition : public SchemaCondition {
public:
  SequenceCondition(Flags flags = Default);

protected:
  virtual bool doValidate(const Node::Ptr& node) const;
};

class MappingCondition : public SchemaCondition {
public:
  MappingCondition(Flags flags = Default);

private:
  virtual bool doValidate(const Node::Ptr& node) const;
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
  // i.e. the return value evaluates to TRUE when validation fails
  Node::Ptr validate(const Document& doc) const;

  Schema& scalar(const std::string& node, Scalar::DataType type, Flags flags = Default);
  Schema& scalar(const std::string& node, Flags flags = Default);
  Schema& path(const std::string& node, Flags flags = Default);
  Schema& file(const std::string& node, Flags flags = Default);
  Schema& sequence(const std::string& node, Flags flags = Default);
  Schema& mapping(const std::string& node, Flags flags = Default);

private:
  Schema& condition(const std::string& node, SchemaCondition *cond);

  std::vector<std::pair<std::string, SchemaCondition::Ptr>> m_conditions;
};

}