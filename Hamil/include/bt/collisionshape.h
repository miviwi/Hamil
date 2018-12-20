#pragma once

#include <bt/bullet.h>

#include <util/ref.h>
#include <util/hash.h>
#include <math/geometry.h>

#include <vector>
#include <memory>
#include <unordered_set>
#include <functional>

namespace bt {

class CollisionShapeManager;

// Non-owning wrapper around btCollisionShape
//   - the ref-count is only used by the CollisionShapeManager
//   - btCollisionShape::getUserPointer() returns a pointer
//     to the owning CollisionShapeManager for this CollisionShape,
//     manager() returns it as a reference
class CollisionShape : public Ref {
public:
  CollisionShape();

  // Returns the underlying handle
  //   to a btCollisionShape as a void *
  void *get() const;

  const char *name() const;

  // Returns a reference to the CollisionShapeManager
  //   which owns this CollisionShape
  CollisionShapeManager& manager() const;

  vec3 calculateLocalInertia(float mass);

  operator bool() const;

  bool operator==(const CollisionShape& other) const;
  bool operator!=(const CollisionShape& other) const;

  // TODO: maybe hash these in a better way someday...
  struct Hash {
    size_t operator()(const CollisionShape& shape) const
    {
      auto h = util::XXHash<void *>();
      return h(shape.get());
    }
  };

protected:
  CollisionShape(btCollisionShape *shape);

  btCollisionShape *bt() const;

private:
  friend class DynamicsWorld;
  friend class RigidBody;
  friend class CollisionShapeManager;

  btCollisionShape *m;
};

// Keeps track of all allocated CollisionShapes and disposes of them
//   automatically
//
// TODO: Make box(), sphere() etc. return an existing CollisionShape
//       if a similar one exists instead of always allocating a new one.
//       A good starting point would propably be to make CollisionShape::Hash
//       smarter
class CollisionShapeManager {
public:
  enum {
    InitialAlloc = 256,
  };

  CollisionShapeManager();
  CollisionShapeManager(const CollisionShapeManager& other) = delete;
  ~CollisionShapeManager();

  CollisionShape box(vec3 half_extents);
  CollisionShape sphere(float radius);

  // 'verts' must be a pointer to a strided array of (X, Y, Z)
  //   coordinates of vertices
  //  - After this function returns the memory for 'verts'
  //    can be reused as the CollisionShape keeps an
  //    internal copy
  CollisionShape convexHull(float *verts, size_t num_verts, size_t stride);
  // 'next_vert' should return 'true' when there are vertices
  //   remaining and write the next set of cooridinates to
  //   'dst'. If there aren't any vertices the function should
  //   return 'false' and terminate.
  CollisionShape convexHull(std::function<bool(vec3& dst)> next_vert);

  // Destroy a collision shape and reclaim it's memory
  //   - Only one refernce the the shape can exist
  //     before calling this function
  void destroy(CollisionShape& shape);

private:
  // - Sets the UserPointer to 'this' (the CollisionShapeManager)
  // - Inserts it into the 'm_shapes' set
  CollisionShape initShape(CollisionShape&& shape);

  std::unordered_set<CollisionShape, CollisionShape::Hash> m_shapes;
};

}