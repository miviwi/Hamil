#include <mesh/halfedge.h>

#include <cassert>

#include <algorithm>
#include <utility>

namespace mesh {

using DirectedEdgeMap = std::map<HalfEdge::Edge, HalfEdge::Index>;

HalfEdgeStructure::HalfEdgeStructure()
{
  m_edge_set = std::make_unique<EdgeSet>();
}

void HalfEdgeStructure::addTraingle(HalfEdge::Vertex v0, HalfEdge::Vertex v1, HalfEdge::Vertex v2)
{
  m_edge_set->emplace(std::min(v0, v1), std::max(v0, v1));
  m_edge_set->emplace(std::min(v1, v2), std::max(v1, v2));
  m_edge_set->emplace(std::min(v2, v0), std::max(v2, v0));

  m_faces.emplace_back(v0, v1, v2);
}

// Source: https://github.com/yig/halfedge/blob/master/trimesh.cpp
void HalfEdgeStructure::build(size_t num_verts)
{
  for(auto& e : *m_edge_set) {
    m_edges.emplace_back(e);
  }

  // No longer needed
  m_edge_set.reset();

  DirectedEdgeMap edge_map;

  const auto add_to_edge_map = [&](HalfEdge::Vertex a, HalfEdge::Vertex b, HalfEdge::Index i) {
    edge_map.emplace(std::make_pair(
      HalfEdge::Edge(a, b),
      i
    ));
  };

  const auto directed_edge_to_face = [&](HalfEdge::Vertex a, HalfEdge::Vertex b) -> HalfEdge::Index {
    auto it = edge_map.find(HalfEdge::Edge(a, b));

    if(it == edge_map.end()) {
      assert(edge_map.find(HalfEdge::Edge(b, a)) != edge_map.end());
      return HalfEdge::None;
    }

    return it->second;
  };

  for(HalfEdge::Index i = 0; i < m_faces.size(); i++) {
    const auto& tri = m_faces[i];

    add_to_edge_map(tri.v[0], tri.v[1], i);
    add_to_edge_map(tri.v[1], tri.v[2], i);
    add_to_edge_map(tri.v[2], tri.v[0], i);
  }

  m_vert_he.resize(num_verts, HalfEdge::None);
  m_face_he.resize(m_faces.size(), HalfEdge::None);
  m_edge_he.resize(m_edges.size(), HalfEdge::None);

  m_halfedges.reserve(m_edges.size() * 2);

  for(HalfEdge::Index ei = 0; ei < m_edges.size(); ei++) {
    const auto& e = m_edges[ei];

    const auto he0_idx = (HalfEdge::Index)m_halfedges.size();
    auto& he0 = m_halfedges.emplace_back();

    const auto he1_idx = (HalfEdge::Index)m_halfedges.size();
    auto& he1 = m_halfedges.emplace_back();

    he0.face = directed_edge_to_face(e.first, e.second);
    he0.vertex = e.second;
    he0.edge = ei;

    he1.face = directed_edge_to_face(e.second, e.first);
    he1.vertex = e.first;
    he1.edge = ei;

    he0.opposite = he1_idx;
    he1.opposite = he0_idx;

    m_edge_to_halfedge.emplace(std::make_pair(
      HalfEdge::Edge(e.first, e.second),
      he0_idx
    ));
    m_edge_to_halfedge.emplace(std::make_pair(
      HalfEdge::Edge(e.second, e.first),
      he1_idx
    ));

    if(m_vert_he[he0.vertex] == HalfEdge::None || he1.face == HalfEdge::None) {
      m_vert_he[he0.vertex] = he0.opposite;
    }
    if(m_vert_he[he1.vertex] == HalfEdge::None || he0.face == HalfEdge::None) {
      m_vert_he[he1.vertex] = he1.opposite;
    }

    if(he0.face != HalfEdge::None && m_face_he[he0.face] == HalfEdge::None) {
      m_face_he[he0.face] = he0_idx;
    }
    if(he1.face != HalfEdge::None && m_face_he[he1.face] == HalfEdge::None) {
      m_face_he[he1.face] = he1_idx;
    }

    m_edge_he[ei] = he0_idx;
  }

  IndexVector boundary;
  for(HalfEdge::Index hei = 0; hei < m_halfedges.size(); hei++) {
    auto& he = m_halfedges[hei];

    if(he.face == HalfEdge::None) {
      boundary.emplace_back(hei);
      continue;
    }

    const auto& face = m_faces[he.face];
    const auto i = he.vertex;

    HalfEdge::Index j = HalfEdge::None;
    if(face.v[0] == i)      j = face.v[1];
    else if(face.v[1] == i) j = face.v[2];
    else if(face.v[2] == i) j = face.v[0];

    he.next = edge_map.at(HalfEdge::Edge(i, j));
  }

  std::map<HalfEdge::Index, std::set<HalfEdge::Index>> vert_to_boundary;
  for(auto& hei : boundary) {
    const auto opposite = m_halfedges[hei].opposite;
    const auto origin = m_halfedges[opposite].vertex;

    vert_to_boundary[origin].insert(hei);
    if(vert_to_boundary[origin].size() > 1) throw ButterflyVertexError();
  }

  for(auto& hei : boundary) {
    auto& he = m_halfedges[hei];

    auto& outgoing = vert_to_boundary[he.vertex];
    if(outgoing.empty()) continue;

    const auto it = outgoing.begin();
    he.next = *it;

    outgoing.erase(it);
  }

  // No longer needed as well
  m_edges.clear();
  m_edges.shrink_to_fit();
}

const HalfEdge& HalfEdgeStructure::halfedge(HalfEdge::Vertex a, HalfEdge::Vertex b) const
{
  auto it = m_edge_to_halfedge.find({ a, b });
  if(it == m_edge_to_halfedge.end()) throw InvalidQueryError();

  return m_halfedges.at(it->second);
}

void HalfEdgeStructure::walkVertexNeighbours(HalfEdge::Vertex v, VertexVisitor visitor) const
{
  const auto start = m_vert_he[v];
  auto hei = start;

  do {
    const auto& he = m_halfedges[hei];
    const auto& opposite = m_halfedges[he.opposite];

    visitor(he.vertex);

    hei = opposite.next;
  } while(hei != start);
}

void HalfEdgeStructure::walkFaceNeighbours(HalfEdge::Vertex v, FaceVisitor visitor) const
{
  const auto start = m_vert_he[v];
  auto hei = start;

  do {
    const auto& he = m_halfedges[hei];
    const auto& opposite = m_halfedges[he.opposite];

    if(he.face != HalfEdge::None) {
      const auto& face = m_faces[he.face];
      visitor(face);
    }

    hei = opposite.next;
  } while(hei != start);
}

}