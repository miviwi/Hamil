#pragma once

#include "util/stream.h"

#include <cassert>
#include <string>
#include <sstream>
#include <utility>

// #define DEBUG_PRINT_TOKENS

namespace glang {

class Tokenizer;

class Token {
public:
  enum Tag {
    Invaild,
    Int, Float, Ratio, Char,  String, Symbol, Keyword,
    LParen, RParen, LSquareBracket, RSquareBracket, LBrace, RBrace,
    Separator,
    Quote, SyntaxQuote, Unquote, UnquoteSplicing, Dispatch,
    EndOfFile,
  };

  Token(Tag tag_, std::string str_, unsigned line_, unsigned colum_) :
    m_tag(tag_), m_str(str_), m_line(line_), m_column(colum_) { }

  operator bool() const { return m_tag != EndOfFile; }

  Tag tag() const { return m_tag; }

  const char *tagLiteral() const
  {
    static const char *lits[] = {
      "Invaild",
      "Int", "Float", "Ratio", "Char",  "String", "Symbol", "Keyword",
      "LParen", "RParen", "LSquareBracket", "RSquareBracket", "LBrace", "RBrace",
      "Separator",
      "Quote", "SyntaxQuote", "Unquote", "UnquoteSplicing", "Dispatch",
      "EndOfFile",
    };
    return lits[m_tag];
  }

  std::string toString() const
  {
    std::ostringstream s;
    s << line() << ", " << column() <<  ": " << "(" << tagLiteral() << ") " << "`" << str() << "'";
    return s.str();
  }

  const std::string& str() const { return m_str; }

  long long toInt() const { return stoll(m_str); }
  double toFloat() const { return stod(m_str); }
  std::pair<long long, long long> toRatio() const
  {
    if(m_tag != Ratio) return {};

    size_t pos = m_str.find('/');
    return { stoll(m_str.substr(0, pos)), stoll(m_str.substr(pos+1)) };
  }
  char toChar() const { return m_str.front(); }

  unsigned line() const { return m_line; }
  unsigned column() const { return m_column; }

private:
  Tag m_tag;

  std::string m_str;
  unsigned m_line, m_column;
};

class Tokenizer {
public:
  enum State {
    Invalid,
    Begin,
    ReadNumber, ReadFloat, ReadRatio, ReadChar, ReadString, ReadSymbol,
    ReadUnquote,
    ReadSeparator, SkipWhitespace, SkipComment,

    NumStates,
  };

  struct Error {
    virtual std::string what() const { assert(0); return ""; }
  };

  struct InvalidNumberError : public Error {
    InvalidNumberError(std::string str_) :
      m_str(str_) { }

    virtual std::string what() const
    {
      std::ostringstream s;
      s << "Error reading number: " << m_str;
      return s.str();
    }

  private:
    std::string  m_str;
  };

  struct InvalidSymbolError : public Error {
    InvalidSymbolError(std::string str_) :
      m_str(str_) { }

    virtual std::string what() const
    {
      std::ostringstream s;
      s << "Error reading symbol: " << m_str;
      return s.str();
    }

  private:
    std::string  m_str;
  };

  struct InvalidEscapeSequenceError : public Error {
    InvalidEscapeSequenceError(char ch_) :
      m_ch(ch_) { }

    virtual std::string what() const
    {
      std::ostringstream s;
      s << "Inavlid escape sequence \\" << m_ch;
      return s.str();
    }

  private:
    char m_ch;
  };

  Tokenizer(InputStream& input_) :
    m_state(Begin), m_input(input_), m_tok_ind(0),
    m_tok{ token(), Token(Token::Invaild, "", 0, 0) }
  { }

  Token peekToken()
  {
    return m_tok[m_tok_ind];
  }

  Token nextToken()
  {
    Token& ret = m_tok[m_tok_ind];
    m_tok_ind ^= 1;

    m_tok[m_tok_ind] = token();
    return ret;
  }

  bool hasNext()
  {
    return m_tok[m_tok_ind].tag() != Token::EndOfFile;
  }

private:
  Token token();

  State m_state;
  InputStream& m_input;

  unsigned m_tok_ind;
  Token m_tok[2];
};

}
