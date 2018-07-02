#include <python/modules/resmodule.h>
#include <python/module.h>

#include <util/format.h>
#include <res/res.h>
#include <res/manager.h>
#include <res/resource.h>
#include <res/text.h>

#include <string>
#include <map>
#include <optional>

namespace python {

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
  auto guid = res::Resource::InvalidId;
  auto flags = res::LoadDefault;
  if(!PyArg_ParseTuple(args, "n|i:load", &guid, &flags)) return nullptr;

  try {
    auto ptr = res::resource().load(guid, flags);
    if(auto r = ptr.lock()) {
      Py_RETURN_TRUE;
    }
  } catch(const res::ResourceManager::Error&) {
    PyErr_SetString(PyExc_ValueError,
      util::fmt("failed to load resource with guid=0x%.16llx", guid).c_str());
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