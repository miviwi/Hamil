#pragma once

#include <gx/gx.h>

#include <util/ref.h>

namespace gx {

class QueryScope;
class ConditionalRenderScope;

// Only ONE query of a given Type can be active at any
//   given time so care must be taken to avoid launching
//   multiple queries of the same type simultaneously
class Query : public Ref {
public:
  enum Type {
    // The result is the number of samples
    //   which passed the depth test
    SamplesPassed        = GL_SAMPLES_PASSED,

    // The result is a bool which is == false when no
    //   generated samples have passed the depth test
    //   and 'true' otherwise
    AnySamplesPassed     = GL_ANY_SAMPLES_PASSED,

    // The result is the number of primitives sent
    //   to the Geometry Shader by the scoped commands
    XfbPrimitivesWritten = GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN,

    // The result is the time in nanoseconds
    TimeElapsed = GL_TIME_ELAPSED,

    // The result is the time in nanoseconds
    //   - MUST be used with queryCounter()
    //     instead of begin() to generate the
    //     result
    Timestamp   = GL_TIMESTAMP,
  };

  enum ConditionalMode {
    // Render if and only if the Query result
    //   is 'true' (i.e. stall when it isn't
    //   available)
    Wait   = GL_QUERY_WAIT,
    // Skip rendering if the Query result is
    //   available and == false, when it's
    //   not available or the result is 'true'
    //   the scoped commands will be executed
    NoWait = GL_QUERY_NO_WAIT,

    ByRegionWait   = GL_QUERY_BY_REGION_WAIT,
    ByRegionNoWait = GL_QUERY_BY_REGION_NO_WAIT,
  };

  Query(Type type);
  ~Query();

  // Returns an object which automatically ends
  //   the Query when it's scope expires
  //  - When type == Timestamp queryCounter()
  //    must be used instead of this method!
  QueryScope begin();

  Query& queryCounter();

  // Returns 'true' when the result<b,i,sz>()
  //   methods can returns without blocking
  bool resultAvailable() const;
  
  // Returns the result as a bool
  //   - Blocks when the result is not
  //     yet available at the time of
  //     the method call
  int resulti();
  // Returns the result as an int
  //   - See note above resulti()
  bool resultb();
  // Returns the result as a size_t
  //   - See note above resulti()
  size_t resultsz();

  ConditionalRenderScope conditionalRender(ConditionalMode mode);

private:
  friend QueryScope;
  friend ConditionalRenderScope;

  Type m_target;
  GLuint m;
};

class QueryScope : public Ref {
public:
  QueryScope(QueryScope&& other);
  ~QueryScope();

  // Ends the query before the objects's
  //   scope expires
  void end();

private:
  friend Query;

  QueryScope(Query& q);

  Query *m;
};

class ConditionalRenderScope : public Ref {
public:
  ConditionalRenderScope(ConditionalRenderScope&& other);
  ~ConditionalRenderScope();

  // Ends the conditionalRender() before
  //   the object's scope expires
  void end();

private:
  friend Query;

  ConditionalRenderScope(Query& q, Query::ConditionalMode mode);

  Query *m;
};

}