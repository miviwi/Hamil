#pragma once

#include <vm/object.h>
#include <vm/objman.h>
#include <vm/codeobject.h>
#include <vm/heap.h>
#include <debug/table.h>
#include <debug/database.h>

#include <cassert>
#include <cstdint>
#include <utility>
#include <new>
#include <unordered_map>
#include <unordered_set>

namespace glang {

// TODO:
//  - stack unwinding on exception (error recovery)
//    (all throw's will have to be replaced with helper method)
//  - think of a better place for eval()
class Vm {
public:
  typedef unsigned __int32 Instruction;
  typedef          __int32 StackOffset;

  typedef intptr_t Address;

  using CodeObject = assembler::CodeObject;

  enum ErrorCode {
    InvalidError,

    InvalidCodeObjectError,
    SeqgetArgCountError,
    SeqgetError,
    NotASequenceError,
    CallingNonCallableError,
    CarCdrError,
    InvalidUnaryOpError,
    ApplyImproperListError,
    ConjError,
    AssocNonMapError,
    DissocNonMapError,
    ArgTypeError,
  };

  struct Error { };
  struct InvalidEnvironmentQueryError : public Error { };
  struct InvalidInstructionError : public Error { };
  struct ArgumentCountError : public Error {
    const int got, expected;
    const bool var_args;

    ArgumentCountError(int got_, int expected_, bool var_args_) :
      got(got_), expected(expected_), var_args(var_args_) { }
  };
  struct RuntimeError : public Error {
    const ErrorCode what;

    RuntimeError(ErrorCode what_) : what(what_) { }

    const char *toString() const;
  };
  struct RedefinitionError : public Error {
    const Address first_defined;

    RedefinitionError(Address first_defined_) :
      first_defined(first_defined_)
    { }
  };

  template <typename T, size_t N>
  struct Stack {
  public:
    Stack() : ptr(data) {}

    void push(const T& d) { boundsCheck(); *ptr++ = d; }

    const T& peek() { boundsCheck(); return *(ptr - 1); }

    T *top() { boundsCheck(); return ptr; }

    const T& pop(int n = 1)
    {
      boundsCheck();
      T *p = ptr-1;
      ptr -= n;
      boundsCheck();

      return *p;
    }

    void set(T *p) { boundsCheck(); ptr = p; }

    size_t size() const { return ptr-data; }

  private:
    T data[N];
    T *ptr;

    void boundsCheck() 
    {
      assert((ptr-data) >= 0 && (ptr-data) < N && "stack over/underflow!");
    }
  };

  struct Environment {
  public:
    ObjectRef get(long long val);
    bool set(long long key, ObjectRef val, Address addr = 0);

    Address getLocation(long long val);
 
    void finalize(ObjectManager& om);

  private:
    using EnvEntry = std::pair<ObjectRef, Address>;

    std::unordered_map<std::string, EnvEntry> m_store;
  };

  struct FnDesc {
    union {
      struct { byte num_upvalues, num_locals, num_args, var_args; };
      Instruction raw;
    };
  };

  enum : unsigned long long {
    InstructionBits = sizeof(Instruction)*8,
    InstructionAlignment = 2,

    OpMask  = 0xF8000000,
    OpShift = InstructionBits-5,

    ImmBits = InstructionBits-7,
    ImmMax  = (1<<ImmBits)-1,

    PushImmRatioBits    = 12,
    PushImmRatioMax     = (1<<PushImmRatioBits)-1,
    PushImmRatioHiShift = 12,
    PushImmTypeShift    = OpShift-2,
    PushImmTypeMask     = 0x3 << PushImmTypeShift,
    PushOffsetBits      = ImmBits,
    PushOffsetMask      = (1<<PushOffsetBits)-1,
    PushOffsetMax       = PushOffsetMask<<2,

    MaxRefSlot = 0xFF,

    BccCondShift   = OpShift-3,
    BccCondMask    = 0x7,
    BccOffsetBits  = InstructionBits-8,
    BccOffsetMask  = (1<<BccOffsetBits)-1,
    BccOffsetMax   = BccOffsetMask<<2,

    ConstTypeShift  = OpShift-3,
    ConstTypeBits   = 3,
    ConstTypeMask   = (1<<ConstTypeBits)-1,
    ConstOffsetBits = InstructionBits-8,
    ConstOffsetMask = (1<<ConstOffsetBits)-1,
    ConstOffsetMax  = ConstOffsetMask<<2,

    RefTypeShift    = ConstTypeShift,
    RefTypeMask     = ConstTypeMask<<RefTypeShift,

    EnvOpShift      = OpShift-2,
    EnvOpMask       = 0x3<<EnvOpShift,
    EnvOpOffsetBits = InstructionBits-8,
    EnvOpOffsetMask = (1<<EnvOpOffsetBits)-1,
    EnvOpOffsetMax  = EnvOpOffsetMask<<2,

    ArtmOpShift       = OpShift-2,
    ArtmOpMask        = 0x3<<ArtmOpShift,
    ArtmOpArgCountMax = 0x00FFFFFF,
  };

  enum Opcode {
    NOP,
    PUSHnil, PUSHimm, PUSHconst, PUSHref, PUSHoff,
    POP, SWAP, DUP,
    STORE,
    ENV,
    CALL, RCALL, RET,
    B, Bcc,
    CONS,
    LIST, VEC, MAP, SET,
    FN,
    SEQGET,
    CAR, CDR,
    ARTM,
    APPLY,
    SYSCALL,
    EXIT,
  };

  enum {
    PUSHimmInt   = 0x02000000,
    PUSHimmChar  = 0x04000000,
    PUSHimmRatio = 0x06000000,

    ENVget     = 0x00000000,
    ENVgets    = 0x02000000,
    ENVset     = 0x04000000,
    ENVsets    = 0x06000000,

    ARTMadd    = 0x00000000,
    ARTMsub    = 0x02000000,
    ARTMmul    = 0x04000000,
    ARTMdiv    = 0x06000000,

    RefArg     = 0x02000000,
    RefLocal   = 0x04000000,
    RefUpvalue = 0x06000000,
  };

  struct ConstType {
    enum { Int, Ratio, Float, String, Symbol, Keyword, Function };
  };

  enum { EQ, NEQ, LT, LTE, GT, GTE };

  enum {
    S_REPR,
    S_STR,
    S_INT, S_FLOAT, S_RATIO,
    S_MOD,
    S_CONJ, S_ASSOC, S_DISSOC,
    S_ATOMp, S_NILp,
    S_INTp, S_FLOATp, S_RATIOp,
    S_STRp, S_SYMp, S_KWp,
    S_NUMp, S_SEQp,
    S_CONSp, S_VECp, S_MAPp, S_SETp,
    S_FNp,
    S_HANDLEp,
    S_DROP,
    S_SIN, S_COS,
  };

  Vm(IHeap *heap);
  ~Vm();

  Object *stackTop()
  {
    ObjectRef ref = m_stack.size() ? m_stack.pop() : ObjectRef::nil();

    if(ref.isInline()) return ref.type() == ObjectRef::Nil ? nil->ref() : m_man.createInt(ref.val());
    if((long long)(ref.val()) < 0 || ref.val() % sizeof(Instruction)) return nil->ref();

    Object *o = m_man.box(ref);
    if(o->header.raw_header <= Object::Handle && o->type() != Object::Invalid) return o;

    return nil->ref();
  }
  Object *stackPop()
  {
    auto o = stackTop();

    return (Object *)o;
  }

  Object *envGet(const char *name)
  {
    ObjectRef o = m_env.get((long long)name);

    return m_man.box(o);
  }
  void envSet(const char *name, Object *o)
  {
    m_env.set((long long)name, m_man.toRef(o));
  }

  Object *call(Object *fn, Object *args[], size_t num_args);
  template <typename... Args>
  Object *call(Object *fn, Args... args)
  {
    Object *args_array[] = { args... };

    return call(fn, args_array, sizeof...(args));
  }

  ObjectManager& objMan() { return m_man; }

  Object *execute(const CodeObject& co);
  Object *execute(const CodeObject& co, DebugDatabase::Ptr debug);

  Object *eval(const char *src);

  Address pc() const;
  Address debugPc(const DebugDatabase *debug) const;
  const DebugDatabase *debug() const;
  const DebugDatabase *debug(Address pc) const;

  std::string locationInfo(const DebugDatabase *dbg, Address debug_pc) const;
  std::string locationInfo() const;

private:
  bool isRunning() { return m_running; }
  void run() { m_running = true; }

  void exInstrution();
  unsigned nextInstrution() { return (*m_pc)>>OpShift; }

  void load(const CodeObject& co);
  void unload(const CodeObject& co);

  ObjectRef *arg(unsigned n);
  ObjectRef *local(unsigned n);
  ObjectRef *upvalue(unsigned n);

  void nop();

  void push(ObjectRef obj);
  void pop(unsigned n);

  void swap();
  void dup();

  void storea(unsigned dst);
  void storel(unsigned dst);
  void storeu(unsigned dst);

  void envgets();
  void envget(long var_off);
  void envsets();
  void envset(long var_off);

  void call(int num_args);
  void rcall(int num_args);
  void ret();

  void bcc(int cond, long offset);

  void cons();
  void list(int num_elems);
  void vec(int num_elems);
  void map(int num_elems);
  void set(int num_elems);

  void car();
  void cdr();

  void seqget();
  
  void artm(int num_elements, long op);

  void apply();

  void syscall(int what);

  void redefinitionError(long long key);

  struct CallFrame {
    FunctionObject *self;

    Instruction *lr;
    ObjectRef *bp;
    ObjectRef *sp;
  };

  using AddressRange = std::pair<Address, Address>;

  std::unordered_set<CodeObject> m_loaded;
  std::vector<std::pair<AddressRange, DebugDatabase::Ptr>> m_debug;

  bool m_running;

  ObjectManager m_man;

  Instruction *m_pc;

  Stack<ObjectRef, 1024> m_stack;
  Stack<CallFrame, 64> m_call_stack;

  Environment m_env;

  Object *nil;

};

}