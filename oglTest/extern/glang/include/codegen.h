#pragma once

#include <util/stream.h>
#include <parser.h>
#include <symbols.h>
#include <asm.h>

#include <cstdio>
#include <cmath>
#include <string>
#include <stack>
#include <set>

namespace glang {

class Token;

class AST_Root;
class AST_Atom;
class AST_Form;
class AST_Collection;

class ASTVisitor {
public:
  virtual void visit(const AST_Root& tree) = 0;
  virtual void visit(const AST_Atom& tree) = 0;
  virtual void visit(const AST_Form& tree) = 0;
  virtual void visit(const AST_Collection& tree) = 0;

  virtual ~ASTVisitor() { }
};

class ASTDumper : public ASTVisitor {
public:
  ASTDumper() :
    m_indent(0) { }

  virtual void visit(const AST_Root& tree);
  virtual void visit(const AST_Atom& tree);
  virtual void visit(const AST_Form& tree);
  virtual void visit(const AST_Collection& tree);

private:
  void printIndent()
  {
    for(int i = 0; i < m_indent; i++) printf("    ");
  }

  int m_indent;
};

class CodeGen : public ASTVisitor {
public:
  struct Error {
    virtual std::string what() const = 0;

    virtual unsigned line() const = 0;
    virtual unsigned column() const = 0;

  };
  struct ErrorWithToken : public Error {
  public:
    ErrorWithToken(const Token& tok_) : m_tok(tok_) { }

    virtual std::string what() const
    {
      std::ostringstream o;

      o << "error: ";
      message(o, m_tok);

      return o.str();
    }

    virtual unsigned line() const { return m_tok.line(); }
    virtual unsigned column() const { return m_tok.column(); }

  protected:
    virtual void message(std::ostringstream& o, const Token& tok) const = 0;

  private:
    Token m_tok;
  };
  struct SpecialFormArgError : public ErrorWithToken {
  public:
    SpecialFormArgError(const Token& tok, const std::string& func_,
                        const std::string& gotten_, const std::string& expected_) :
      ErrorWithToken(tok), m_func(func_), m_gotten(gotten_), m_expected(expected_) { }

  protected:
    virtual void message(std::ostringstream& o, const Token& tok) const
    {
      if(!m_expected.empty()) {
        o << "expected " << m_expected << " for function '" << m_func << "' (got "
          << m_gotten << ")";
      } else {
        o << "argument type error for funtion '" << m_func << "' (got '" << m_gotten << "')";
      }
    }

  private:
    std::string m_func, m_gotten, m_expected;
  };
  struct SpecialFormArgCountError : public ErrorWithToken {
  public:
    SpecialFormArgCountError(const Token& tok, const std::string& func_, int gotten_, int expected_) :
      ErrorWithToken(tok), m_func(func_), m_gotten(gotten_), m_expected(expected_) { }

  protected:
    virtual void message(std::ostringstream& o, const Token& tok) const
    {
      o << "expected " << (m_expected < 0 ? "at least " : "") << abs(m_expected)
        << " arguments for function '" << m_func << "' (got "
        << m_gotten << ")";
    }

  private:
    std::string m_func;
    int m_gotten, m_expected;
  };
  struct DefAtNonGlobalScopeError : public ErrorWithToken {
  public:
    using ErrorWithToken::ErrorWithToken;

  protected:
    virtual void message(std::ostringstream& o, const Token& tok) const
    {
      o << "attempted to define at non-global scope";
    }
  };
  struct FuncArgSpecifierError : public ErrorWithToken {
  public:
    using ErrorWithToken::ErrorWithToken;

  protected:
    virtual void message(std::ostringstream& o, const Token& tok) const
    {
      o << "function argument is neither a symbol nor destructure vector/list";
    }
  };
  struct MapStaryKeyError : public ErrorWithToken {
  public:
    using ErrorWithToken::ErrorWithToken;

  protected:
    virtual void message(std::ostringstream& o, const Token& tok) const
    {
      o << "stray key at end of map";
    }
  };
  struct InvalidOperator : public ErrorWithToken {
  public:
    using ErrorWithToken::ErrorWithToken;

  protected:
    virtual void message(std::ostringstream& o, const Token& tok) const
    {
      o << "invalid operator";
    }
  };
  struct ArgsAfterVarArgError : public ErrorWithToken {
  public:
    using ErrorWithToken::ErrorWithToken;

  protected:
    virtual void message(std::ostringstream& o, const Token& tok) const
    {
      o << "positional arguments after varadic";
    }
  };
  struct UnquoteOutsideSyntaxQuote : public ErrorWithToken {
  public:
    using ErrorWithToken::ErrorWithToken;

  protected:
    virtual void message(std::ostringstream& o, const Token& tok) const
    {
      o << "unquote/unquote-splicing used outside of syntax quote";
    }
  };
  struct StrayAndInArgsError : public ErrorWithToken {
  public:
    using ErrorWithToken::ErrorWithToken;

  protected:
    virtual void message(std::ostringstream& o, const Token& tok) const
    {
      o << "'&' not followed by arg vector name in function argument list";
    }
  };
  struct InvalidDestructureError : public ErrorWithToken {
  public:
    using ErrorWithToken::ErrorWithToken;

  protected:
    virtual void message(std::ostringstream& o, const Token& tok) const
    {
      o << "invalid destructure";
    }
  };

  CodeGen(OutputStream& out_);

  virtual void visit(const AST_Root& tree);
  virtual void visit(const AST_Atom& tree);
  virtual void visit(const AST_Form& tree);
  virtual void visit(const AST_Collection& tree);

  void def(const AST_Form& form);
  void defn(const AST_Form& form);

  void fn(const AST_Form& form);

  void quote(const AST_Form& form);
  void syntaxQuote(const AST_Form& form);

  void unquoteError(const AST_Form& form);
  
  void recur(const AST_Form& form);

  void loop(const AST_Form& form);

  void cons(const AST_Form& form);
  void list(const AST_Form& form);

  void car(const AST_Form& form);
  void cdr(const AST_Form& form);

  void if_(const AST_Form& form);
  
  void cond(const AST_Form& form);

  void let(const AST_Form& form);

  void apply(const AST_Form& form);
  void eval_(const AST_Form& form);

  void plus(const AST_Form& form);
  void minus(const AST_Form& form);
  void mult(const AST_Form& form);
  void div(const AST_Form& form);

  void eq(const AST_Form& form);
  void neq(const AST_Form& form);
  void lt(const AST_Form& form);
  void lte(const AST_Form& form);
  void gt(const AST_Form& form);
  void gte(const AST_Form& form);

private:
  void quoteAtom(const AST_Atom& atom);
  void quoteSeq(const SyntaxTree& tree, const ParentTree& seq);
  void syntaxQuoteSeq(const SyntaxTree& tree, const ParentTree& seq);
  void unquote(const AST_Form& form);

  void makeSeq(const SyntaxTree& tree, int num_elems);

  void eval(const SyntaxTree& tree);
  void evalForm(const AST_Form& form);
  void evalFormApplication(const AST_Form& form);

  void evalFuncBody(const AST_Form& form, bool has_name, const std::string& name, bool& var_args);

  void artm(const AST_Form& form, void (AsmGen::*fn)(int));
  void cmp(const AST_Form& form, AsmGen::Cond cond);

  void pushCollection(const AST_Collection& coll);
  void evalCollection(const AST_Collection& coll);

  void destructure(const AST_Collection& coll, SymbolId id);

  void pushVar(const Symbol& var);
  void pushEnvVar(InternedString var);

  void pushTrampoline(const AST_Atom& fn);
  void genTrampolines();

  SymbolTable::Sym defLocalSymbol(const Token& tok, const AST_Atom& sym);
  SymbolTable::Sym defArg(const Token& tok, const SyntaxTree& arg);

  void defFunctionArgs(const AST_Collection& args, const Token& form_tok, bool& var_args);

  void snoopUpvalues(const AST_Atom& atom, Scope::Ptr outer_scope);
  void snoopUpvalues(const ParentTree& forms, Scope::Ptr outer_scope);

  void ifFastPath(const AST_Form& form, const AST_Form& cond);

  // when
  //   expected < 0
  // "at least" is added to the error mewssage
  void formArgCountError(const AST_Form& form, int expected);

  void location(const SyntaxTree& s);

  static bool is_special_form(InternedString op);
  static bool is_syscall(InternedString op);

  AsmGen m_gen;
  AsmConstTable m_consts;

  std::stack<Scope::Ptr> m_scope_stack;
  std::stack<SymbolTable::Ptr> m_sym_stack;

  unsigned m_lambda_id, m_if_no, m_cond_no;

  std::set<std::string> m_special_trampolines, m_syscall_trampolines;
};

}