#include <py/modules/yamlmodule.h>
#include <py/module.h>
#include <py/types.h>
#include <py/collections.h>

#include <yaml/document.h>
#include <yaml/node.h>

#include <cassert>
#include <utility>

namespace py {

struct NodeToken;
static MemberDefList<NodeToken> NodeMembers;
static MethodDefList<NodeToken> NodeMethods;

struct Node {
  PyObject_HEAD;

  yaml::Node::Ptr m;
};

static int Node_Check(PyObject *obj);
static PyObject *Node_FromPtr(const yaml::Node::Ptr& p);

#define NODE_INIT_ERR "Node can only be default initialized"

static int Node_Init(Node *self, PyObject *args, PyObject *kwds)
{
  if(PyTuple_Size(args) || kwds) {
    PyErr_SetString(PyExc_TypeError, NODE_INIT_ERR);
    return -1;
  }

  new(&self->m) yaml::Node::Ptr();
  return 0;
}

static void Node_Dealloc(Node *self)
{
  self->m.reset();
  TypeObject::freeObject(self);
}

static PyObject *Node_Repr(Node *self)
{
  return Unicode(self->m->repr()).move();
}

static PyObject *Node_GetAttr(Node *self, PyObject *attr)
{
  yaml::Node::Ptr node;
  if(PyUnicode_Check(attr)) {
    Unicode u = Object::ref(attr);

    node = self->m->get(u.str());
  } else if(PyNumber_Check(attr)) {
    Long l = PyNumber_Long(attr);

    node = self->m->get(l.sz());
  }

  return node ? Node_FromPtr(node) : PyObject_GenericGetAttr((PyObject *)self, attr);
}

static TypeObject Node_Type =
  TypeObject()
    .name("yaml.Node")
    .doc("object representing a yaml value")
    .size(sizeof(Node))
    .init((initproc)Node_Init)
    .destructor((::destructor)Node_Dealloc)
    .getattr((getattrofunc)Node_GetAttr)
    .repr((reprfunc)Node_Repr)
  ;

static int Node_Check(PyObject *obj)
{
  return Node_Type.check(obj);
}

static PyObject *Node_FromPtr(const yaml::Node::Ptr& p)
{
  auto obj = Node_Type.newObject<Node>();
  new(&obj->m) yaml::Node::Ptr(p);

  return (PyObject *)obj;
}

struct DocumentToken;
static MemberDefList<DocumentToken> DocumentMembers;
static MethodDefList<DocumentToken> DocumentMethods;
static GetSetDefList<DocumentToken> DocumentGetSet;

struct Document {
  PyObject_HEAD;

  yaml::Document m;
};

static int Document_Check(PyObject *obj);
static PyObject *Document_FromString(PyObject *str);

#define DOCUMENT_INIT_ERR "Document can only be default initialized (use one of the Document.from_<...>() methods)"

static int Document_Init(Document *self, PyObject *args, PyObject *kwds)
{
  if(PyTuple_Size(args) || kwds) {
    PyErr_SetString(PyExc_TypeError, DOCUMENT_INIT_ERR);
    return -1;
  }

  new(&self->m) yaml::Document();
  return 0;
}

static void Document_Dealloc(Document *self)
{
  self->m.get().reset();
  TypeObject::freeObject(self);
}

static PyObject *Document_GetAttr(Document *self, PyObject *attr)
{
  return PyObject_GetAttr(Node_FromPtr(self->m.get()), attr);
}

static PyObject *Document_Repr(Document *self)
{
  Unicode doc(self->m.get()->repr());

  return Unicode::from_format("Document(%s)", doc.repr().data()).move();
}

static PyObject *Document_Str(Document *self)
{
  return Unicode(self->m.get()->repr()).move();
}

static PyObject *Document_FromString_(PyObject *Py_UNUSED(klass), PyObject *arg)
{
  if(!PyUnicode_Check(arg)) {
    PyErr_SetString(PyExc_TypeError, "Docuemnt.from_string() only accepts a 'str' as an argument");
    return nullptr;
  }

  return Document_FromString(arg);
}

static PyObject *Document_Root(Document *self, void *Py_UNUSED(closure))
{
  return Node_FromPtr(self->m.get());
}

static TypeObject Document_Type =
  TypeObject()
    .name("yaml.Document")
    .doc("holds the root yaml.Node")
    .size(sizeof(Document))
    .init((initproc)Document_Init)
    .destructor((::destructor)Document_Dealloc)
    //.getattr((getattrofunc)Document_GetAttr)
    .methods(DocumentMethods(
      MethodDef()
        .name("from_string")
        .doc("parses a str and returns a Docuemnt")
        .method(Document_FromString_)
        .flags(METH_CLASS | METH_O)))
    .getset(DocumentGetSet(
      GetSetDef()
        .name("root")
        .doc("the root Node of the Document")
        .get((getter)Document_Root)))
    .repr((reprfunc)Document_Repr)
    .str((reprfunc)Document_Str)
  ;

static int Document_Check(PyObject *obj)
{
  return Document_Type.check(obj);
}

static PyObject *Document_FromString(PyObject *str)
{
  auto obj = Document_Type.newObject<Document>();

  ssize_t sz = 0;
  const char *doc = PyUnicode_AsUTF8AndSize(str, &sz);
  new(&obj->m) yaml::Document(yaml::Document::from_string(doc, (size_t)sz));

  return (PyObject *)obj;
}

ModuleDef YamlModule =
  ModuleDef()
    .name("yaml")
    .doc("a module for parsing and writing YAML documents based on a wrapper for libyaml")
  ;

static const char *p_yaml_test_str = R"yaml(
a: 1
b: 2
c: hello!
d:  [1, 2, 3]
)yaml";

PyObject *PyInit_yaml()
{
  auto self = Module::create(YamlModule.py())
    .addType(Document_Type)

    .addString("test_str", p_yaml_test_str)
    ;

  return *self;
}

}