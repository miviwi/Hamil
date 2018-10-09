#pragma once

#include <common.h>

#include <map>
#include <memory>
#include <functional>

namespace mesh {

struct HalfEdge {
  using Ptr = std::shared_ptr<HalfEdge>;

  struct Vertex {
    using Ptr = std::shared_ptr<Vertex>;

    uint idx;
  };
  struct Face {
    using Ptr = std::shared_ptr<Face>;

    Vertex v[3];
  };

  Ptr opposite;
  Ptr next;

  Vertex::Ptr vertex;
  Face::Ptr face;
};

class HalfEdgeStructure {
public:

private:
};

}