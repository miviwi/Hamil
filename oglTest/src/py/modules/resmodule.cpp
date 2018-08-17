#include <py/modules/resmodule.h>
#include <py/types.h>
#include <py/module.h>

#include <util/format.h>
#include <res/res.h>
#include <res/manager.h>
#include <res/resource.h>
#include <res/text.h>

#include <string>
#include <map>
#include <optional>

namespace py {

static PyObject *Res_Guid(PyObject *self, PyObject *args, PyObject *kwds)
{
  static char *kwds_names[] = {
    "tag", "name", "path", nullptr,
  };

  const char *tag_str = nullptr, *name = nullptr, *path = nullptr;

  if(!PyArg_ParseTupleAndKeywords(args, kwds, "sss:guid", kwds_names,
    &tag_str, &name, &path)) return nullptr;

  auto tag = res::resource().make_tag(tag_str);
  if(!tag) {
    PyErr_SetString(PyExc_ValueError, "invalid resource tag!");
    return nullptr;
  }

  auto guid = res::resource().guid(tag.value(), name, path);
  return PyLong_FromSize_t(guid);
}

static PyObject *Res_Load(PyObject *self, PyObject *args)
{
  static_assert(sizeof(ulonglong) == sizeof(res::Resource::Id),
    "fmt for PyArg_ParseTuple() incorrect!");

  auto guid = res::Resource::InvalidId;
  auto flags = res::LoadDefault;
  if(!PyArg_ParseTuple(args, "K|i:load", &guid, &flags)) return nullptr;

  try {
    auto ptr = res::resource().load(guid, flags);
    if(auto r = ptr.lock()) {
      return Unicode::from_format("%s/%s (0x%.16llx): %s",
        r->path().data(), r->name().data(), r->id(), r->getTag().get()).move();
    }
  } catch(const res::ResourceManager::Error&) {
    auto err = util::fmt("failed to load resource with guid=0x%.16llx", guid);

    PyErr_SetString(PyExc_ValueError, err.data());
  }

  return nullptr; // unreachable with no exception set
}

struct ResModuleToken;
static MethodDefList<ResModuleToken> ResModuleMethods;

static ModuleDef ResModule =
  ModuleDef()
    .name("res")
    .doc("resource management module")
    .methods(ResModuleMethods(
      MethodDef()
        .name("guid")
        .method(Res_Guid)
        .flags(METH_VARARGS | METH_KEYWORDS),
      MethodDef()
        .name("load")
        .method(Res_Load)
        .flags(METH_VARARGS)))
  ;

PyObject *PyInit_res()
{
  auto self = Module::create(ResModule.py())

    // res::LoadFlags
    .addInt("LoadDefault", res::LoadDefault)
    .addInt("LoadStatic",  res::LoadStatic)
    .addInt("Precache",    res::Precache)
    ;

  return *self;
}

}