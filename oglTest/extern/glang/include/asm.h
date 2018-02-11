#pragma once

#include <util/stream.h>
#include <util/istring.h>
#include <vm/vm.h>

#include <cassert>
#include <cstring>
#include <unordered_set>
#include <utility>

namespace glang {

class AsmGen;

struct AsmOperand {
  enum Type {
    Invalid,
    Nil,
    Int, Float, Ratio, Char,
    ArgRef, LocalRef, UpvalueRef,
    ConstRef,
    Label, Offset,
  };

  Type type;
  union {
    long long i;
    double d;
    struct { long long num, denom; } ratio;
    char ch;

    unsigned ref;

    long long off;

    InternedString lbl;
  };

  AsmOperand() { }

  AsmOperand(long long i_) : type(Int), i(i_) { }
  AsmOperand(double d_) : type(Float), d(d_) { }
  AsmOperand(std::pair<long long, long long> r) : type(Ratio), ratio{ r.first, r.second } { }
  AsmOperand(char ch_) : type(Char), ch(ch_) { }

  AsmOperand(Type type_, unsigned ref_) : type(type_), ref(ref_) { }
  AsmOperand(Type type_, InternedString lbl_) : type(type_), lbl(lbl_) { }

  static AsmOperand nil()
  {
    return AsmOperand{ AsmOperand::Nil, 0 };
  }

  static AsmOperand aref(unsigned ref)
  {
    return AsmOperand{ AsmOperand::ArgRef, ref };
  }
  static AsmOperand lref(unsigned ref)
  {
    return AsmOperand{ AsmOperand::LocalRef, ref };
  }
  static AsmOperand uref(unsigned ref)
  {
    return AsmOperand{ AsmOperand::UpvalueRef, ref };
  }
  static AsmOperand cref(InternedString lbl)
  {
    return AsmOperand{ AsmOperand::ConstRef, lbl };
  }
};

struct AsmConst {
  enum Type {
    Invalid,
    Function,
    String, Symbol, Keyword,
    Int, Float, Ratio,
  };

  Type type;
  int id;

  std::string str;
  InternedString sym;
  union {
    long long i;
    double f;
    struct {
      long long num, denom;
    } r;
  } val;

  struct FunctionDesc {
    unsigned num_upvalues, num_args, num_locals;
    bool var_args;

    bool operator==(const FunctionDesc& r) const
    {
      return !!memcmp(this, &r, sizeof(FunctionDesc));
    }
  } fn;

  AsmConst(FunctionDesc fn_) : type(Function), fn(fn_) { }
  AsmConst(Type type_, InternedString sym_) : type(type_), sym(sym_) { }
  AsmConst(const std::string& str_) : type(String), str(str_) { }
  AsmConst(long long i) : type(Int) { val.i = i; }
  AsmConst(double f) : type(Float) { val.f = f; }
  AsmConst(long long num, long long denom) : type(Ratio) { val.r = { num, denom }; }

  static AsmConst symbol(const InternedString& symbol)
  {
    return { AsmConst::Symbol, symbol };
  }

  static AsmConst keyword(const InternedString& kw)
  {
    return { AsmConst::Keyword, kw };
  }

  static AsmConst function(unsigned num_upvalues, unsigned num_args, unsigned num_locals, bool var_args)
  {
    return { FunctionDesc{num_upvalues, num_args, num_locals, var_args} };
  }

  std::string typeStr() const
  {
    static const char *lits[] = {
      "Invalid",
      "Function",
      "String", "Symbol", "Keyword",
      "Int", "Float", "Ratio",
    };
    return lits[type];
  }
};

struct AsmConstTable {
public:
  using Const = std::pair<InternedString, AsmConst>;

  struct HashConst {
    typedef AsmConstTable::Const argument_type;
    typedef size_t result_type;

    result_type operator()(argument_type const& s) const
    {
      static const char hash_name[] = {
        0,
        1,
        1, 0, 0,
        1, 1, 1,
      };
      return hash_name[s.second.type] ? s.first.hash() : s.second.sym.hash();
    }
  };
  struct CmpConst {
    typedef size_t result_type;
    typedef AsmConstTable::Const first_argument_type;
    typedef AsmConstTable::Const second_argument_type;

    bool operator()(const AsmConstTable::Const& l, const AsmConstTable::Const& r) const
    {
      if(l.second.type != r.second.type) return false;
      switch(l.second.type) {
      case AsmConst::Function:
        return l.second.fn == l.second.fn;
      case AsmConst::String:
        return l.second.str == r.second.str;
      case AsmConst::Keyword:
      case AsmConst::Symbol:
        return l.second.sym == r.second.sym;
      case AsmConst::Int:
        return l.second.val.i == r.second.val.i;
      case AsmConst::Float:
        return l.second.val.f == r.second.val.f;
      case AsmConst::Ratio:
        return l.second.val.r.num == r.second.val.r.num &&
          l.second.val.r.denom == r.second.val.r.denom;

      default: assert(0);
      }

      return false;
    }
  };

  AsmConstTable() : m_next_id(0) { }

  InternedString append(AsmConst c)
  {
    auto lbl = InternedString("$"+c.typeStr()+"_"+std::to_string(m_next_id));
    c.id = m_next_id;
    auto result = m_table.insert({ lbl, c });
    if(!result.second) return result.first->first;

    m_next_id++;
    return lbl;
  }

  void emit(AsmGen& gen)
  {
    emitLabel(gen);
    emitIf(gen, AsmConst::Function);
    emitIf(gen, AsmConst::Symbol);
    emitIf(gen, AsmConst::Keyword);
    emitIf(gen, AsmConst::String);
    emitIf(gen, AsmConst::Int);
    emitIf(gen, AsmConst::Float);
    emitIf(gen, AsmConst::Ratio);
  }

private:
  void emitIf(AsmGen& gen, AsmConst::Type type)
  {
    for(auto& c : m_table) if(c.second.type == type) emitOne(c, gen);
  }

  void emitLabel(AsmGen& gen);
  void emitOne(const Const& c, AsmGen& gen);

  int m_next_id;
  std::unordered_set<Const, HashConst, CmpConst> m_table;
};

class AsmGen {
public:
  friend struct AsmConstTable;

  struct InvalidOperandError { };

  AsmGen(OutputStream& out_) :
    m_out(out_), m_indent(0) { }

  void nop();

  void push(AsmOperand operand);
  void pop(unsigned n = 1);

  void swap();
  void dup();

  void store(AsmOperand dst);

  void envgets();
  void envget(const char *var);
  void envsets();
  void envset(const char *var);

  void call(int num_args);
  void rcall(int num_args);
  void ret();

  enum class Cond { Eq, Neq, Gt, Gte, Lt, Lte };
  void b(const char *lbl);
  void b(int n);
  void b(Cond cond, const char *lbl);
  void b(Cond cond, int n);

  void cons();
  void list(int num_elems);
  void vec(int num_elems);
  void map(int num_elems);
  void set(int num_elems);

  void fn(const char *desc);

  void seqget();

  void car();
  void cdr();

  void add(int num_elems);
  void sub(int num_elems);
  void mul(int num_elems);
  void div(int num_elems);

  void apply();

  void syscall(int what);

  void exit();

  void label(const char *lbl, bool leading_newline = true);
  void localLabel();

  void location(unsigned line, unsigned column);
  void proc(const char *name);
  void arg(const char *name);
  void endp(const char *name);

  void indent() { m_indent++; }
  void unindent(bool leading_newline = true) { if(leading_newline) newline(); m_indent--; }

  void newline() { m_out.next('\n'); }

private:
  template <typename... Args>
  void out(const char *fmt_, Args&&... args)
  {
    for(int i = 0; i < m_indent*2; i++) m_out.next(' ');
    m_out.next(fmt(fmt_, args...).c_str());
  }

  OutputStream& m_out;
  int m_indent;
};

}
