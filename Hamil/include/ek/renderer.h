#pragma once

#include <ek/euklid.h>
#include <ek/renderobject.h>
#include <ek/renderview.h>

#include <sched/job.h>
#include <hm/entity.h>
#include <hm/componentref.h>

#include <memory>

namespace hm {
struct Transform;
}

namespace ek {

class Renderer {
public:
  using ObjectVector = std::vector<RenderObject>;
  using ExtractObjectsJob = std::unique_ptr<
    sched::Job<ObjectVector, hm::Entity, RenderView *>
  >;

  ExtractObjectsJob extractForView(hm::Entity scene, RenderView& view);

private:
  ObjectVector doExtractForView(hm::Entity scene, RenderView& view);

  // Extracts an Entity and all it's children and appends
  //   them to 'objects'
  void extractOne(ObjectVector& objects, const frustum3& frustum,
    hm::Entity e, const mat4& parent);
};

}