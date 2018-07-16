#include <yaml/schema.h>

namespace yaml {

SchemaCondition::SchemaCondition(Flags flags) :
  m_flags(flags)
{
}

bool SchemaCondition::validate(const Node::Ptr& node) const
{
  if(node) return doValidate(node);

  return m_flags & Optional; // if there's no value return true only when
                             // it's optional
}

ScalarCondition::ScalarCondition(Flags flags, Scalar::DataType type) :
  SchemaCondition(flags), m_type(type)
{
}

bool ScalarCondition::doValidate(const Node::Ptr& node) const
{
  if(auto scalar = node->as<Scalar>()) {
    if(m_type == Scalar::Invalid) return true;

    return scalar->dataType() == m_type;
  }

  return false;
}

RegexCondition::RegexCondition(Flags flags) :
  SchemaCondition(flags)
{
}

bool RegexCondition::doValidate(const Node::Ptr& node) const
{
  if(auto path = node->as<Scalar>()) {
    return std::regex_match(path->str(), regex());
  }

  return false;
}

static const std::regex p_path_regex("^((/[^<>:\"/\\|?* ]*)+/?)?$", std::regex::optimize);
const std::regex& PathCondition::regex() const
{
  return p_path_regex;
}

static const std::regex p_filename_regex("^[^<>;\"\\|?* ]+$", std::regex::optimize);
const std::regex& FileCondition::regex() const
{
  return p_filename_regex;
}

SequenceCondition::SequenceCondition(Flags flags) :
  SchemaCondition(flags)
{
}

bool SequenceCondition::doValidate(const Node::Ptr& node) const
{
  if(auto seq = node->as<Sequence>()) {
    return true;
  }

  return false;
}

MappingCondition::MappingCondition(Flags flags) :
  SchemaCondition(flags)
{
}

bool MappingCondition::doValidate(const Node::Ptr& node) const
{
  if(auto map = node->as<Mapping>()) {
    return true;
  }

  return false;
}


AllOfCondition::AllOfCondition(SchemaCondition::Ptr cond, Flags flags) :
  SchemaCondition(flags)
{
}

bool AllOfCondition::doValidate(const Node::Ptr& node) const
{
  if(auto seq = node->as<Sequence>()) {
    for(auto& elem : *seq) if(!m_cond->validate(elem)) return false;

    return true; // all the seqence elements passed validation
  }

  return false;
}

Node::Ptr Schema::validate(const Document& doc) const
{
  for(const auto& cond : m_conditions) {
    auto node = doc(cond.first);

    // bail out when the node failes validation
    if(!cond.second->validate(node)) return node;
  }

  // validation successful
  return {};
}

Schema& Schema::condition(const std::string& node, SchemaCondition *cond)
{
  m_conditions.emplace_back(node, cond);
  return *this;
}

Schema& Schema::scalar(const std::string& node, Scalar::DataType type, Flags flags)
{
  return condition(node, new ScalarCondition(flags, type));
}

Schema& Schema::scalar(const std::string& node, Flags flags)
{
  return condition(node, new ScalarCondition(flags));
}

Schema& Schema::path(const std::string& node, Flags flags)
{
  return condition(node, new PathCondition(flags));
}

Schema& Schema::file(const std::string& node, Flags flags)
{
  return condition(node, new FileCondition(flags));
}

Schema& Schema::sequence(const std::string& node, Flags flags)
{
  return condition(node, new SequenceCondition(flags));
}

Schema& Schema::mapping(const std::string& node, Flags flags)
{
  return condition(node, new MappingCondition(flags));
}

Schema& Schema::allOf(const std::string& node, SchemaCondition::Ptr cond, Flags flags)
{
  return condition(node, new AllOfCondition(cond, flags));
}

Schema& Schema::scalarSequence(const std::string& node, Scalar::DataType type, Flags flags)
{
  return allOf(node, cond<ScalarCondition>(Default, type), flags);
}

}