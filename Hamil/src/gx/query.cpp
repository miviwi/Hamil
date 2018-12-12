#include <gx/query.h>

#include <cassert>

namespace gx {

Query::Query(Type type) :
  m_target(type)
{
  glGenQueries(1, &m);
}

Query::~Query()
{
  if(deref()) return;

  glDeleteQueries(1, &m);
}

QueryScope Query::begin()
{
  assert(m_target != Timestamp && "begin() called on a Query(Timestamp)!"
    " (did you mean to call queryCounter() instead?");

  return QueryScope(*this);  // calls glBeginQuery(m_target, m)
}

Query& Query::queryCounter()
{
  assert(m_target == Timestamp && "queryCounter() may only be called on a Query(Timestamp)!");

  glQueryCounter(m, m_target);

  return *this;
}

bool Query::resultAvailable() const
{
  int available = 0;
  glGetQueryObjectiv(m, GL_QUERY_RESULT_AVAILABLE, &available);

  return available;
}

int Query::resulti()
{
  int result = 0;
  glGetQueryObjectiv(m, GL_QUERY_RESULT, &result);

  return result;
}

bool Query::resultb()
{
  return (bool)resulti();
}

size_t Query::resultsz()
{
  GLuint64 result = 0;
  glGetQueryObjectui64v(m, GL_QUERY_RESULT, &result);

  return result;
}

ConditionalRenderScope Query::conditionalRender(ConditionalMode mode)
{
  return ConditionalRenderScope(*this, mode);
}

QueryScope::QueryScope(Query& q) :
  m(&q)
{
  glBeginQuery(m->m_target, m->m);
}

QueryScope::QueryScope(QueryScope&& other) :
  m(other.m)
{
  other.m = nullptr;
}

QueryScope::~QueryScope()
{
  if(deref()) return;

  end();
}

void QueryScope::end()
{
  if(!m) return;

  glEndQuery(m->m_target);
  m = nullptr;
}

thread_local GLuint p_active_conditional = ~0u;

ConditionalRenderScope::ConditionalRenderScope(Query& q, Query::ConditionalMode mode) :
  m(&q)
{
  assert(p_active_conditional == ~0u && "attempted to start a nested conditionalRender()!");

  glBeginConditionalRender(m->m, mode);
  p_active_conditional = m->m;
}

ConditionalRenderScope::ConditionalRenderScope(ConditionalRenderScope&& other) :
  m(other.m)
{
  other.m = nullptr;
}

ConditionalRenderScope::~ConditionalRenderScope()
{
  if(deref()) return;

  end();
}

void ConditionalRenderScope::end()
{
  if(!m) return;

  glEndConditionalRender();
  p_active_conditional = ~0u;
}

}