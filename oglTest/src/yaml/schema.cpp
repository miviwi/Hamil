#include <yaml/schema.h>

namespace yaml {

ScalarCondition::ScalarCondition(Scalar::DataType type) :
  m_type(type)
{
}

bool ScalarCondition::validate(const Node::Ptr& node)
{
  if(auto scalar = node->as<Scalar>()) {
    if(m_type == Scalar::Invalid) return true;

    return scalar->dataType() == m_type;
  }

  return false;
}

bool SequenceCondition::validate(const Node::Ptr& node)
{
  if(auto seq = node->as<Sequence>()) {
    return true;
  }

  return false;
}

bool MappingCondition::validate(const Node::Ptr& node)
{
  if(auto map = node->as<Mapping>()) {
    return true;
  }

  return false;
}

Node::Ptr Schema::validate(const Document& doc)
{
  for(const auto& cond : m_conditions) {
    auto node = doc(cond.first);

    // bail out when the node doesn't exist or it failes validation
    if(!node || !cond.second->validate(node)) return node;
  }

  // validation successful
  return {};
}

Schema& Schema::condition(const std::string& node, SchemaCondition *cond)
{
  m_conditions.emplace_back(node, SchemaCondition::Ptr(cond));
  return *this;
}

Schema& Schema::scalar(const std::string& node, Scalar::DataType type)
{
  return condition(node, new ScalarCondition(type));
}

Schema& Schema::scalar(const std::string& node)
{
  return condition(node, new ScalarCondition());
}

Schema& Schema::sequence(const std::string& node)
{
  return condition(node, new SequenceCondition());
}

Schema& Schema::mapping(const std::string& node)
{
  return condition(node, new MappingCondition());
}

}