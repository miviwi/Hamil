#pragma once

#include <common.h>

#include <map>
#include <set>
#include <array>
#include <memory>
#include <functional>

namespace mesh {

struct HalfEdge {
  using Index = uint;
  enum : Index {
    None = ~0u,
  };

  using Vertex = Index;
  using Edge = std::pair<Vertex, Vertex>;

  struct Face {
    Face() :
      v({ None, None, None })
    { }

    Face(Vertex v0, Vertex v1, Vertex v2) :
      v({ v0, v1, v2 })
    { }

    std::array<Vertex, 3> v;
  };

  Index opposite;
  Index next;

  Vertex vertex;
  Index face;
  Index edge;
};

class HalfEdgeStructure {
public:
  using EdgeSet   = std::set<HalfEdge::Edge>;
  using Edges     = std::vector<HalfEdge::Edge>;
  using Faces     = std::vector<HalfEdge::Face>;
  using HalfEdges = std::vector<HalfEdge>;

  using IndexVector = std::vector<HalfEdge::Index>;

  using EdgeToHalfEdge = std::map<HalfEdge::Edge, HalfEdge::Index>;

  struct Error {
  };

  struct ButterflyVertexError : public Error {
  };

  HalfEdgeStructure();

  void addTraingle(HalfEdge::Vertex v0, HalfEdge::Vertex v1, HalfEdge::Vertex v2);

  // This method needs to be called after adding all the edges
  //   to actually build the half-edge structure
  void build(size_t num_verts);

private:
  // Intermediate structures
  std::unique_ptr<EdgeSet> m_edge_set;
  Edges m_edges;
  Faces m_faces;

  HalfEdges m_halfedges;

  IndexVector m_vert_he;
  IndexVector m_face_he;
  IndexVector m_edge_he;

  EdgeToHalfEdge m_edge_to_halfedge;
};

}