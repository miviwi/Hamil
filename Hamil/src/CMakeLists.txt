set (ProjectDir "${PROJECT_SOURCE_DIR}/Hamil")
set (SrcDir     "${ProjectDir}/src")
set (Generated  "${ProjectDir}/Generated")

target_sources (Hamil PUBLIC
  "${SrcDir}/bt/bullet.cpp"
  "${SrcDir}/bt/btcommon.cpp"
  "${SrcDir}/bt/collisionshape.cpp"
  "${SrcDir}/bt/ray.cpp"
  "${SrcDir}/bt/rigidbody.cpp"
  "${SrcDir}/bt/world.cpp"

  "${SrcDir}/cli/cli.cpp"
  "${SrcDir}/cli/resourcegen.cpp"
  "${SrcDir}/cli/sh.cpp"

  "${SrcDir}/ek/euklid.cpp"
  "${SrcDir}/ek/sharedobject.cpp"
  "${SrcDir}/ek/constbuffer.cpp"
  "${SrcDir}/ek/mempool.cpp"
  "${SrcDir}/ek/rendertarget.cpp"
  "${SrcDir}/ek/occlusion.cpp"
  "${SrcDir}/ek/visibility.cpp"
  "${SrcDir}/ek/visobject.cpp"
  "${SrcDir}/ek/renderer.cpp"
  "${SrcDir}/ek/renderobject.cpp"
  "${SrcDir}/ek/renderview.cpp"

  "${SrcDir}/ft/cache.cpp"
  "${SrcDir}/ft/font.cpp"
  "${SrcDir}/ft/gxuniforms.cpp"
  
  "${SrcDir}/gx/gx.cpp"
  "${SrcDir}/gx/info.cpp"
  "${SrcDir}/gx/context.cpp"
  "${SrcDir}/gx/buffer.cpp"
  "${SrcDir}/gx/commandbuffer.cpp"
  "${SrcDir}/gx/fence.cpp"
  "${SrcDir}/gx/framebuffer.cpp"
  "${SrcDir}/gx/memorypool.cpp"
  "${SrcDir}/gx/pipeline.cpp"
  "${SrcDir}/gx/program.cpp"
  "${SrcDir}/gx/query.cpp"
  "${SrcDir}/gx/renderpass.cpp"
  "${SrcDir}/gx/resourcepool.cpp"
  "${SrcDir}/gx/texture.cpp"
  "${SrcDir}/gx/vertex.cpp"

  "${SrcDir}/hm/hamil.cpp"
  "${SrcDir}/hm/cmdline.cpp"
  "${SrcDir}/hm/component.cpp"
  "${SrcDir}/hm/componentmeta.cpp"
  "${SrcDir}/hm/componentref.cpp"
  "${SrcDir}/hm/entity.cpp"
  "${SrcDir}/hm/entityman.cpp"
  "${SrcDir}/hm/prototype.cpp"
  "${SrcDir}/hm/prototypecache.cpp"
  "${SrcDir}/hm/prototypechunk.cpp"
  "${SrcDir}/hm/cachedprototype.cpp"
  "${SrcDir}/hm/chunkhandle.cpp"
  "${SrcDir}/hm/chunkman.cpp"
  "${SrcDir}/hm/world.cpp"
  "${SrcDir}/hm/components/gameobject.cpp"
  "${SrcDir}/hm/components/hull.cpp"
  "${SrcDir}/hm/components/light.cpp"
  "${SrcDir}/hm/components/material.cpp"
  "${SrcDir}/hm/components/mesh.cpp"
  "${SrcDir}/hm/components/rigidbody.cpp"
  "${SrcDir}/hm/components/transform.cpp"
  "${SrcDir}/hm/components/visibility.cpp"

  "${SrcDir}/math/brdf.cpp"
  "${SrcDir}/math/compare.cpp"
  "${SrcDir}/math/frustum.cpp"
  "${SrcDir}/math/intrin.cpp"
  "${SrcDir}/math/ltc.cpp"
  "${SrcDir}/math/neldermead.cpp"
  "${SrcDir}/math/quaternion.cpp"
  "${SrcDir}/math/transform.cpp"
  "${SrcDir}/math/util.cpp"
  "${SrcDir}/math/xform.cpp"

  "${SrcDir}/mesh/halfedge.cpp"
  "${SrcDir}/mesh/loader.cpp"
  "${SrcDir}/mesh/mesh.cpp"
  "${SrcDir}/mesh/obj.cpp"
  "${SrcDir}/mesh/util.cpp"

  "${SrcDir}/py/python.cpp"
  "${SrcDir}/py/collections.cpp"
  "${SrcDir}/py/exception.cpp"
  "${SrcDir}/py/module.cpp"
  "${SrcDir}/py/object.cpp"
  "${SrcDir}/py/types.cpp"
  "${SrcDir}/py/modules/btmodule.cpp"
# "${SrcDir}/py/modules/hmmodule.cpp"
  "${SrcDir}/py/modules/mathmodule.cpp"
  "${SrcDir}/py/modules/resmodule.cpp"
  "${SrcDir}/py/modules/win32module.cpp"
  "${SrcDir}/py/modules/yamlmodule.cpp"

  "${SrcDir}/res/res.cpp"
  "${SrcDir}/res/cache.cpp"
  "${SrcDir}/res/handle.cpp"
  "${SrcDir}/res/image.cpp"
  "${SrcDir}/res/io.cpp"
  "${SrcDir}/res/loader.cpp"
  "${SrcDir}/res/lut.cpp"
  "${SrcDir}/res/manager.cpp"
  "${SrcDir}/res/mesh.cpp"
  "${SrcDir}/res/resource.cpp"
  "${SrcDir}/res/shader.cpp"
  "${SrcDir}/res/text.cpp"
  "${SrcDir}/res/texture.cpp"

  "${SrcDir}/sched/scheduler.cpp"
  "${SrcDir}/sched/job.cpp"
  "${SrcDir}/sched/pool.cpp"

  "${SrcDir}/ui/ui.cpp"
  "${SrcDir}/ui/uicommon.cpp"
  "${SrcDir}/ui/animation.cpp"
  "${SrcDir}/ui/button.cpp"
  "${SrcDir}/ui/console.cpp"
  "${SrcDir}/ui/cursor.cpp"
  "${SrcDir}/ui/drawable.cpp"
  "${SrcDir}/ui/dropdown.cpp"
  "${SrcDir}/ui/event.cpp"
  "${SrcDir}/ui/frame.cpp"
  "${SrcDir}/ui/label.cpp"
  "${SrcDir}/ui/layout.cpp"
  "${SrcDir}/ui/painter.cpp"
  "${SrcDir}/ui/scrollframe.cpp"
  "${SrcDir}/ui/slider.cpp"
  "${SrcDir}/ui/style.cpp"
  "${SrcDir}/ui/textbox.cpp"
  "${SrcDir}/ui/window.cpp"

  "${SrcDir}/util/allocator.cpp"
  "${SrcDir}/util/bitvector.cpp"
  "${SrcDir}/util/fixedbitvector.cpp"
  "${SrcDir}/util/polystorage.cpp"
  "${SrcDir}/util/dds.cpp"
  "${SrcDir}/util/format.cpp"
  "${SrcDir}/util/hash.cpp"
  "${SrcDir}/util/hashindex.cpp"
  "${SrcDir}/util/lfsr.cpp"
  "${SrcDir}/util/opts.cpp"
  "${SrcDir}/util/ref.cpp"
  "${SrcDir}/util/staticstring.cpp"
  "${SrcDir}/util/str.cpp"
  "${SrcDir}/util/unit.cpp"

  "${SrcDir}/os/os.cpp"
  "${SrcDir}/os/error.cpp"
  "${SrcDir}/os/panic.cpp"
  "${SrcDir}/os/cpuid.cpp"
  "${SrcDir}/os/cpuinfo.cpp"
  "${SrcDir}/os/stdstream.cpp"
  "${SrcDir}/os/path.cpp"
  "${SrcDir}/os/file.cpp"
  "${SrcDir}/os/time.cpp"
  "${SrcDir}/os/input.cpp"
  "${SrcDir}/os/inputman.cpp"
  "${SrcDir}/os/waitable.cpp"
  "${SrcDir}/os/thread.cpp"
  "${SrcDir}/os/mutex.cpp"
  "${SrcDir}/os/rwlock.cpp"
  "${SrcDir}/os/conditionvar.cpp"
  "${SrcDir}/os/window.cpp"
  "${SrcDir}/os/glcontext.cpp"
  "${SrcDir}/os/clipboard.cpp"

  "${SrcDir}/yaml/document.cpp"
  "${SrcDir}/yaml/node.cpp"
  "${SrcDir}/yaml/schema.cpp"

)

#  --- Platform specific sources ---
if (WIN32)
  target_sources (Hamil PUBLIC
      "${SrcDir}/win32/win32.cpp"
      "${SrcDir}/win32/clipboard.cpp"
      "${SrcDir}/win32/conditionvar.cpp"
      "${SrcDir}/win32/cpuid.cpp"
      "${SrcDir}/win32/cpuinfo.cpp"
      "${SrcDir}/win32/event.cpp"
      "${SrcDir}/win32/file.cpp"
      "${SrcDir}/win32/glcontext.cpp"
      "${SrcDir}/win32/handle.cpp"
      "${SrcDir}/win32/inputman.cpp"
      "${SrcDir}/win32/mman.cpp"
      "${SrcDir}/win32/mutex.cpp"
      "${SrcDir}/win32/panic.cpp"
      "${SrcDir}/win32/rwlock.cpp"
      "${SrcDir}/win32/stdstream.cpp"
      "${SrcDir}/win32/thread.cpp"
      "${SrcDir}/win32/time.cpp"
      "${SrcDir}/win32/waitable.cpp"
      "${SrcDir}/win32/window.cpp"
  )
endif()

if (UNIX)
  target_sources (Hamil PUBLIC
      "${SrcDir}/sysv/sysv.cpp"
      "${SrcDir}/sysv/cpuid.cpp"
      "${SrcDir}/sysv/cpuinfo.cpp"
      "${SrcDir}/sysv/stdstream.cpp"
      "${SrcDir}/sysv/file.cpp"
      "${SrcDir}/sysv/time.cpp"
      "${SrcDir}/sysv/inputman.cpp"
      "${SrcDir}/sysv/thread.cpp"
      "${SrcDir}/sysv/mutex.cpp"
      "${SrcDir}/sysv/panic.cpp"
      "${SrcDir}/sysv/rwlock.cpp"
      "${SrcDir}/sysv/conditionvar.cpp"
      "${SrcDir}/sysv/x11.cpp"
      "${SrcDir}/sysv/window.cpp"
      "${SrcDir}/sysv/glcontext.cpp"
      "${SrcDir}/sysv/clipboard.cpp"
  )
endif()



