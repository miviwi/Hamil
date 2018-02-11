#pragma once

#include <util/stream.h>
#include <util/istring.h>
#include <vm/vm.h>
#include <vm/codeobject.h>
#include <debug/table.h>

#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>

namespace glang {

namespace assembler {

class Token {
public:
  enum Tag {
    Invalid,
    Imm, Ref, ConstRef, Offset,
    IntArg,
    Label,  LocalLabel,
    Instruction, Directive,
    DirectiveArg,
    Newline,
    EndOfFile,
  };

  enum {
    NullData,
    ImmNil, ImmInt, ImmRatio, ImmChar,
    RefArg, RefLocal, RefUpvalue,
    OffsetLabel, OffsetLocalF, OffsetLocalB,
    DirectiveArgInt, DirectiveArgString,
  };

  Token(Tag tag_, std::string str_, unsigned line_, unsigned column_, unsigned data_ = 0) :
    m_tag(tag_), m_str(str_), m_line(line_), m_column(column_), m_data(data_) { }

  operator bool() const { return m_tag != EndOfFile; }

  Tag tag() const { return m_tag; }

  const char *tagLiteral() const
  {
    static const char *lits[] = {
      "Invalid",
      "Imm", "Ref", "ConstRef", "Offset",
      "IntArg",
      "Label",  "LocalLabel",
      "Instruction", "Directive",
      "DirectiveArg",
      "Newline",
      "EndOfFile",
    };
    return lits[m_tag];
  }

  const char *dataLiteral() const
  {
    static const char *lits[] = {
      "NullData",
      "ImmNil", "ImmInt", "ImmRatio", "ImmChar",
      "RefArg", "RefLocal", "RefUpvalue",
      "OffsetLabel", "OffsetLocalF", "OffsetLocalB",
      "DirectiveArgInt", "DirectiveArgString",
    };
    return lits[m_data];
  }

  std::string toString() const
  {
    std::ostringstream s;
    s << line() << ", " << column() <<  ": " <<
      "(" << tagLiteral() << ", " << dataLiteral() << ") " << "`" << str() << "'" ;
    return s.str();
  }

  const std::string& str() const { return m_str; }

  unsigned line() const { return m_line; }
  unsigned column() const { return m_column; }

  unsigned data() const { return m_data; }

private:
  Tag m_tag;

  std::string m_str;
  unsigned m_line, m_column;
  unsigned m_data;
};

class AsmTokenizer {
public:
  enum State {
    Begin,
    Instruction,
    Label, LocalLabel,
    Directive,
    EndOfStatement,
  };

  struct InvalidTokenError { };

  struct InvalidInstructionError { };

  struct InvalidImmError { };
  struct InvalidRefError { };

  struct InvalidDirectiveError { };

  AsmTokenizer(InputStream& input_) : m_state(Begin), m_input(input_) { }

  Token token();

private:
  void skipWhite();

  Token endOfStatement(Token tok);

  Token doBegin();
  Token doInstruction();
  Token doLabel();
  Token doLocalLabel();
  Token doDirective();

  Token doEndOfStatement();

  Token doMnemonic();
  Token doInstructionArg();
  Token doImmRatio();
  Token doRef(Token::Tag tag, unsigned data);

  Token doDirectiveArg();

  State m_state;

  InputStream& m_input;
};

class StatementArg {
public:
  enum Tag {
    None,
    Int,
    ImmNil, ImmInt, ImmChar, ImmRatio,
    RefArg, RefLocal, RefUpvalue,
    LabelOffset, LocalOffset,
    ConstRef,
    DirectiveStr, DirectiveArg,
  };

  StatementArg() : m_tag(None) { }
  StatementArg(Tag tag_) : m_tag(tag_) { }
  StatementArg(Tag tag_, long long i) : m_tag(tag_) { m_val.i = i; }
  StatementArg(Tag tag_, long long num, long long denom) : m_tag(tag_)
  {
    m_val.r.num = num;
    m_val.r.denom = denom;
  }
  StatementArg(Tag tag_, const std::string& str_) : m_tag(tag_), m_str(str_) { }

  static StatementArg aref(unsigned slot)
  {
    StatementArg arg(RefArg);
    arg.m_val.slot = slot;
    return arg;
  }

  static StatementArg lref(unsigned slot)
  {
    StatementArg arg(RefLocal);
    arg.m_val.slot = slot;
    return arg;
  }
  
  static StatementArg uref(unsigned slot)
  {
    StatementArg arg(RefUpvalue);
    arg.m_val.slot = slot;
    return arg;
  }

  static StatementArg local_offset(long long offset)
  {
    StatementArg arg(LocalOffset);
    arg.m_val.offset = offset;
    return arg;
  }

  static StatementArg fn(byte num_upvalues, byte num_args, byte num_locals, bool var_args)
  {
    StatementArg arg(DirectiveArg);
    arg.m_val.fn.raw = 0;
    arg.m_val.fn = {
      num_upvalues,
      num_locals,
      num_args,
      var_args,
    };
    return arg;
  }

  Tag tag() const { return m_tag; }

  const char *tagLiteral() const
  {
    static const char *lits[] = {
      "None",
      "Int",
      "ImmNil", "ImmInt", "ImmChar", "ImmRatio",
      "RefArg", "RefLocal", "RefUpvalue",
      "LabelOffset", "LocalOffset",
      "ConstRef",
      "DirectiveStr", "DirectiveArg",
    };
    return lits[m_tag];
  }

  std::string toString() const
  {
    std::ostringstream s;
    s << tagLiteral() << ": ";
    switch(m_tag) {
    case None:                                   s << "NULL"; break;
    case Int:                                    s << m_val.i; break;
    case ImmNil:                                 s << "'()"; break;
    case ImmInt:                                 s << m_val.i; break;
    case ImmChar:                                s << "'" << m_val.ch << "'"; break;
    case ImmRatio:                               s << m_val.r.num << "/" << m_val.r.denom; break;
    case RefArg: case RefLocal: case RefUpvalue: s << m_val.slot; break;
    case LabelOffset: case ConstRef:             s << m_str.ptr(); break;
    case LocalOffset:                            s << m_val.offset; break;
    case DirectiveArg:
      s << m_val.i << ";" << *(double *)&m_val.i
        << " (" << (int)m_val.fn.num_args << ", " << (int)m_val.fn.num_upvalues  << ", "
        << (int)m_val.fn.var_args << ")"; break;
    case DirectiveStr:                           s << m_str.ptr(); break;
    }
    return s.str();
  }

  long long intVal() const { return m_val.i; }
  char charVal() const { return m_val.ch; }
  std::pair<long long, long long> ratioVal() const { return std::make_pair(m_val.r.num, m_val.r.denom); }
  std::pair<long long, long long> pairVal() const { return ratioVal(); }
  unsigned slot() const { return m_val.slot; }

  long long offset() const { return m_val.offset; }

  Vm::FnDesc fnDesc() const { return m_val.fn; }

  InternedString str() const { return m_str; }

  size_t strAlignedSize() const
  {
    constexpr size_t sz = sizeof(Vm::Instruction)-1;
    return (m_str.size()+1+sz) & ~sz;
  }

private:
  Tag m_tag;

  union {
    long long i;
    char ch;
    struct {
      long long num, denom;
    } r;
    unsigned slot;
    long long offset;
    Vm::FnDesc fn;
  } m_val;

  InternedString m_str;
};

class Statement {
public:
  enum Tag {
    Invalid,
    Instruction,
    Label, LocalLabel,
    Directive,
  };

  enum Directive {
    DirectiveInvalid,
    DirectiveFunction, DirectiveString, DirectiveKeyword,
    DirectiveSymbol, DirectiveInt, DirectiveFloat, DirectiveRatio,

    DirectiveDebugInfo,
  };

  Statement(Tag tag_) : m_tag(tag_) { }

  Statement(Tag tag_, const std::string& str_, StatementArg arg_) :
    m_tag(tag_), m_str(str_), m_arg(arg_) { }

  Statement(Tag tag_, const std::string& str_) :
    m_tag(tag_), m_str(str_) { }

  static Statement label(const std::string& str)
  {
    return { Label, str };
  }

  static Statement local_label()
  {
    return { LocalLabel };
  }

  Tag tag() const { return m_tag; }

  const char *tagLiteral() const
  {
    static const char *lits[] = {
      "Invalid",
      "Instruction",
      "Label", "LocalLabel",
      "Directive",
    };
    return lits[m_tag];
  }

  InternedString str() const { return m_str; }

  std::string toString() const
  {
    std::ostringstream s;
    s << tagLiteral() << ": `" << m_str.ptr() << "' (" << m_arg.toString() << ") ";
    return s.str();
  }

  const StatementArg& arg() const { return m_arg; }

  unsigned directiveType() const;

  unsigned pcAdvance() const;

private:
  Tag m_tag;
  InternedString m_str;
  StatementArg m_arg;
};

class Assembler {
public:
  struct UnexpectedTokenError { };
  struct InvalidDirectiveError { };
  struct InvalidOpcodeArgumentError { };
  struct ImmediateValueTooLargeError { };
  struct RefSlotTooLargeError { };
  struct UnalignedOffsetError { };
  struct OffsetTooLargeError { };
  struct LabelUndefinedError { };

  Assembler(AsmTokenizer& tokenizer) :
    m_pc(0), m_tokenizer(tokenizer), m_locs(nullptr)
  { }
  Assembler(AsmTokenizer& tokenizer, Table_Location *locs) :
    Assembler(tokenizer)
  {
    m_locs = locs;
  }

  CodeObject assemble();

private:
  using Statements = std::vector<Statement>;
  using Labels = std::unordered_map<InternedString, unsigned long long>;
  using Constants = std::unordered_map<unsigned long long, const Statement *>;

  Token expectToken(Token::Tag tag, unsigned data = 0);

  StatementArg constructRefArg(const Token& tok);
  StatementArg constructOffsetArg(const Token& tok);

  Statement constructIntruction(const Token& tok);
  Statement constructDirective(const Token& tok);

  Statement constructNoArgs(const Token& tok);
  Statement constructPush(const Token& tok);
  Statement constructIntArg(const Token& tok);
  Statement constructStore(const Token& tok);
  Statement constructBranch(const Token& tok);
  Statement constructConstArg(const Token& tok);

  Statements constructStatements();
  void doLabelsAndConstants(const Statements& statements);

  long long labelOffset(InternedString label, long long max);
  long long localBranchOffset(const Statement& stmt, long long max);

  void emitInstruction(const Statement& statement);
  void emitDirective(const Statement& statement);

  void emitNoArgs(const Statement& statement, Vm::Instruction op);
  void emitPush(const Statement& statement, Vm::Instruction op);
  void emitIntArg(const Statement& statement, Vm::Instruction op);
  void emitStore(const Statement& statement, Vm::Instruction op);
  void emitBranch(const Statement& statement, Vm::Instruction op);
  void emitConstArg(const Statement& statement, Vm::Instruction op);
  void emitEnv(const Statement& statement, Vm::Instruction op);
  void emitArtm(const Statement& statement, Vm::Instruction op);

  void emitInt(const Statement& statement);
  void emitFloat(const Statement& statement);
  void emitRatio(const Statement& statement);
  void emitFunction(const Statement& statement);
  void emitStrSymKw(const Statement& statement);

  void emitLocation(const Statement& statement);
  void emitProc(const Statement& statement);
  void emitProcArg(const Statement& statement);
  void emitEndp(const Statement& statement);

  CodeObject m_co;

  unsigned long long m_pc;

  Statements m_statements;
  Labels m_labels;
  Constants m_consts;

  AsmTokenizer& m_tokenizer;

  Table_Location *m_locs;
};

}

}
