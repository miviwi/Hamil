#pragma once

#include "tokenizer.h"

#include <cassert>
#include <memory>
#include <utility>
#include <vector>
#include <string>

namespace glang {

class ASTVisitor;

class SyntaxTree {
public:
  enum Tag {
    Invalid,
    Root,
    Atom, Form, Collection,
  };

  typedef std::unique_ptr<SyntaxTree> Ptr;

  template <typename T>
  static const T& ref_cast(const Ptr& ptr)
  {
    return (const T&)*((const SyntaxTree *)ptr.get());
  }

  SyntaxTree(Tag tag_, Token token_) :
    m_tag(tag_), m_token(token_) { }

  virtual ~SyntaxTree() { }

  Tag tag() const { return m_tag; }

  const char *tagLiteral() const
  {
    static const char *lits[] = {
      "Invalid",
      "Root",
      "Atom", "Form", "Collection",
    };
    return lits[m_tag];
  }

  const std::string& str() const { return m_token.str(); }

  const Token& tok() const { return m_token; }
  std::string tokenRepr() const { return m_token.toString(); }

  virtual void accept(ASTVisitor& visitor) const = 0;

private:
  Tag m_tag;
  Token m_token;
};

class ITaggedTree {
public:
  virtual int typeTag() const = 0;

  virtual const char *typeTagLiteral() const = 0;
};

class ParentTree {
public:
  typedef std::vector<SyntaxTree::Ptr> ChildrenVector;

  void appendChild(SyntaxTree::Ptr child)
  {
    m_children.push_back(std::move(child));
  }

  ChildrenVector::const_iterator begin() const { return m_children.cbegin(); }
  ChildrenVector::const_iterator end() const { return m_children.cend(); }

  size_t childCount() const { return m_children.size(); }
  bool empty() const { return m_children.empty(); }

  const SyntaxTree& child(unsigned index) const
  {
    return *(const SyntaxTree *)m_children[index].get();
  }
  
protected:
  ChildrenVector m_children;
};

class AST_Atom : public SyntaxTree, public ITaggedTree {
public:
  enum TypeTag : int {
    Invalid,
    Int, Float, Ratio, Char, String,
    Symbol, Keyword,
  };

  AST_Atom(TypeTag type_tag_, Token token_) :
    SyntaxTree(SyntaxTree::Atom, token_), m_type_tag(type_tag_) { }

  virtual int typeTag() const { return m_type_tag; }

  virtual const char *typeTagLiteral() const
  {
    static const char *lits[] = {
      "Invalid",
      "Int", "Float", "Ratio", "Char", "String",
      "Symbol", "Keyword",
    };
    return lits[m_type_tag];
  }

  virtual void accept(ASTVisitor& visitor) const;

private:
  TypeTag m_type_tag;
};

class AST_Form : public SyntaxTree, public ParentTree {
public:
  AST_Form(Token token_) :
    SyntaxTree(SyntaxTree::Form, token_) { }

  virtual void accept(ASTVisitor& visitor) const;
};

class AST_Collection : public SyntaxTree, public ITaggedTree, public ParentTree {
public:
  enum TypeTag : int {
    Invalid,
    Vector, Map, Set,
  };
  
  AST_Collection(TypeTag type_tag_, Token token_) :
    SyntaxTree(SyntaxTree::Collection, token_), m_type_tag(type_tag_) { }

  virtual int typeTag() const { return m_type_tag; }

  virtual const char *typeTagLiteral() const
  {
    static const char *lits[] = {
      "Invalid",
      "Vector", "Map", "Set",
    };
    return lits[m_type_tag];
  }

  virtual void accept(ASTVisitor& visitor) const;

private:
  TypeTag m_type_tag;
};

class AST_Root : public SyntaxTree, public ParentTree {
public:
  AST_Root() :
    SyntaxTree(SyntaxTree::Root, Token(Token::Invaild, "", 0, 0)) { }

  virtual void accept(ASTVisitor& visitor) const;
};

class Parser {
public:
  Parser(Tokenizer& tokenizer_) :
    m_tokenizer(tokenizer_) { }

  struct Error {
    virtual std::string what() const = 0;
  };

  struct InvalidMacroError : public Error {
  public:
    InvalidMacroError(unsigned line, unsigned column, char ch_) :
      m_line(line), m_column(column), m_ch(ch_) { }

    virtual std::string what() const
    {
      return fmt("%d, %d: invalid macro #%c", m_line, m_column, m_ch);
    }

  private:
    unsigned m_line, m_column;
    char m_ch;
  };

  struct ExpectedSeparatorError : public Error {
  public:
    ExpectedSeparatorError(unsigned line, unsigned column, const char *got) :
      m_line(line), m_column(column), m_got(got) { }

    virtual std::string what() const
    {
      return fmt("%d, %d: expected ' ' or ',' got %s", m_line, m_column, m_got);
    }

  private:
    unsigned m_line, m_column;
    const char *m_got;
  };

  SyntaxTree::Ptr parseTree();

private:
  SyntaxTree::Ptr parseForms();
  SyntaxTree::Ptr parseForm(const Token& tok);

  SyntaxTree::Ptr parseAtom(const Token& tok);

  SyntaxTree::Ptr parseList(const Token& tok);
  SyntaxTree::Ptr parseVector(const Token& tok);
  SyntaxTree::Ptr parseMap(const Token& tok);

  SyntaxTree::Ptr parseQuoteUnquote(const Token& tok);

  SyntaxTree::Ptr doDispatchMacro(const Token& tok);

  typedef SyntaxTree::Ptr (Parser::*MacroFn)(const Token&);
  SyntaxTree::Ptr setMacro(const Token& tok);

  void expectSeparator(const Token& tok);
  bool expectTerminatorOrSeparator(const Token& tok, Token::Tag tag);

  SyntaxTree::Ptr symbol(const std::string& sym);

  void appendForms(ParentTree& form, Token::Tag terminator);

  Tokenizer& m_tokenizer;
};

}