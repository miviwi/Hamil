#pragma once

#include <util/istring.h>

#include <cstdint>
#include <cassert>
#include <memory>
#include <array>
#include <string>
#include <algorithm>
#include <iterator>
#include <utility>
#include <unordered_set>
#include <unordered_map>

namespace glang {

typedef uint32_t SymbolId;
enum { InvalidSymbolId = ~0 };

using SymbolIdSet = std::unordered_set<SymbolId>;

template <int SlotClasses>
class ScopeManager {
public:
  typedef std::shared_ptr<ScopeManager<SlotClasses>> Ptr;

  ScopeManager(std::string fn_name_) : m_fn_name(fn_name_) { }

  enum { InvalidSymbol = ~0 };

  const std::string& fnName() const { return m_fn_name; }

  SymbolId genSymId(unsigned slot_class);
  unsigned numClassSyms(unsigned slot_class);

  bool owns(SymbolId id) const;

private:
  std::string m_fn_name;

  std::array<SymbolIdSet, SlotClasses> m_slots;
  static SymbolIdSet symbol_ids;

  static SymbolId gen_id();
};

class Symbol {
public:
  enum Tag : unsigned {
    Invalid,
    ArgSymbol, LocalSymbol, UpvalueSymbol,
    MacroSymbol,

    NumClasses = 4,
  };

  Symbol() : m_tag(Invalid), m_name("") { }

  Symbol(Tag tag_, unsigned slot_, const InternedString& name_) :
    m_tag(tag_), m_slot(slot_), m_name(name_) { }

  Tag tag() const { return m_tag; }

  unsigned slot() const { return m_slot; }
  const InternedString& name() const { return m_name; }

private:
  Tag m_tag;

  unsigned m_slot;
  InternedString m_name;
};

using Scope = ScopeManager<Symbol::NumClasses>;
using SymbolSet = std::unordered_set<const Symbol *>;

class SymbolTable {
public:
  typedef std::shared_ptr<SymbolTable> Ptr;

  using SymbolNameMap = std::unordered_map<InternedString, SymbolId>;
  using SymbolMap = std::unordered_map<SymbolId, Symbol>;

  using Sym = std::pair<SymbolId, const Symbol *>;

  const Sym invalid_sym{ InvalidSymbolId, nullptr };

  SymbolTable(SymbolTable *parent_, Scope::Ptr scope_) :
    m_parent(parent_), m_scope(scope_) { }

  Sym genSym(Symbol::Tag tag, const std::string& name);

  Sym lookup(InternedString name);

  const Symbol *get(SymbolId id);

  SymbolSet upvalues();

private:
  SymbolTable *m_parent;

  Scope::Ptr m_scope;
  SymbolNameMap m_sym_names;
  SymbolMap m_syms;
};

}