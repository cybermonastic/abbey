#pragma once

#include <iostream>
#include <variant>
#include <string>
#include <unordered_map>
#include <optional>

#include "abbey/debug.h"

/*
// For sample usage, see unit tests.
//
// The text reader and writer are hard-coded to a particular
// syntax, which is json-like. Errors will be raised if
// tokens do not occur in contexts which are valid syntax.
// Whitespace is generally ignored, except that it is treated
// as a separator between tokens. Strings may contain
// whitespace (including newlines) without being broken up
// by the text reader. Within a string, the '\' may be used
// to escape other characters, particularly '\' and '"'.
//
// Below is a sample of this syntax:
//
name:"he said, \"Hello World!\""
count:2
inner:{
  experimental:yes
  collection:[
    100,
    200
  ]
  real1: 0.00
  real2: 7.8965e-9
}
//  
*/
namespace abbey {

enum TokenType {
    KEY,
    ENUM,
    STRING,
    INTEGER,
    REAL,  // precision no worse than 12, decimal.
    BEGIN_STRUCT,
    END_STRUCT,
    BEGIN_SEQ,
    END_SEQ,
};

struct Token {
    TokenType type;
    std::variant<
        std::monostate, // struct, end_seq
        std::size_t, // begin_seq (or monostate)
        std::pair<size_t, size_t>, // (namspace,token) key or enum
        std::string, // string
        long, // integer
        double // real
    > token;

    size_t GetKey() const { return std::get<std::pair<size_t, size_t>>(token).second; }
    size_t GetEnum() const { return std::get<std::pair<size_t, size_t>>(token).second; }
    std::string_view GetString() const { return std::get<std::string>(token); }
    std::string ConsumeString() { return std::move(std::get<std::string>(token)); }
    long GetInteger() const { return std::get<long>(token); }
    double GetReal() const { return std::get<double>(token); }
    std::optional<size_t> GetCount() const { 
        return std::holds_alternative<size_t>(token) 
            ? std::get<size_t>(token) 
            : std::optional<size_t>(); 
    }

    static inline Token Key(size_t namespace_id, size_t id) 
        { return Token{TokenType::KEY, std::pair{namespace_id, id}}; }
    static inline Token Enum(size_t namespace_id, size_t id) 
        { return Token{TokenType::ENUM, std::pair{namespace_id, id}}; }
    static inline Token String(std::string&& str) { return Token{TokenType::STRING, std::move(str)}; }
    static inline Token StringCopy(std::string_view str) { return Token{TokenType::STRING, std::string(str)}; }
    static inline Token Integer(long n) { return Token{TokenType::INTEGER, n}; }
    static inline Token Real(double r) { return Token{TokenType::REAL, r}; }
    static inline Token BeginStruct() { return Token{TokenType::BEGIN_STRUCT}; }
    static inline Token EndStruct() { return Token{TokenType::END_STRUCT}; }
    static inline Token BeginSeqWithSize(size_t count) { return Token{TokenType::BEGIN_SEQ, count}; }
    static inline Token BeginSeq() { return Token{TokenType::BEGIN_SEQ}; }
    static inline Token EndSeq() { return Token{TokenType::END_SEQ}; }
};

class TokenType_Debugable : public Debugable {
private:
    TokenType t;
public:
    TokenType_Debugable(TokenType t) : t(t) {}

    std::ostream& Debug(std::string_view, std::string_view, std::ostream& out) const override;
};
inline TokenType_Debugable MakeDebugable(TokenType t) { return TokenType_Debugable(t); }

class SymbolNamespace {
private:
    std::unordered_map<std::string_view, size_t> str_to_id;
    std::vector<std::string> id_to_str;
public:
    // Register invalidates 'str_to_id'
    size_t Register(std::string_view symbol);
    // CommitRegistrations reconstructs valid 'str_to_id'
    void CommitRegistrations();

    size_t IdForString(std::string_view symbol) const;
    std::string_view StringForId(size_t id) const;
};

class SymbolMap {
private:
    std::vector<std::unique_ptr<SymbolNamespace>> namespaces;

    const SymbolNamespace& GetNamespace(size_t namespace_id) const;
public:
    size_t DeclareNamespace(); 
    size_t Register(size_t namespace_id, std::string_view symbol);
    void CommitRegistrations();

    size_t IdForString(size_t namespace_id, std::string_view symbol) const {
        return GetNamespace(namespace_id).IdForString(symbol);
    }
    std::string_view StringForId(size_t namespace_id, size_t id) const {
        return GetNamespace(namespace_id).StringForId(id);
    }
};

class Token_Debugable : public Debugable {
private:
    const Token& token;
    const SymbolMap& token_map;
public:
    Token_Debugable(const SymbolMap& token_map, const Token& token) : token_map(token_map), token(token) {}

    std::ostream& Debug(std::string_view, std::string_view, std::ostream& out) const override;
};
inline Token_Debugable MakeDebugable(const SymbolMap& token_map, const Token& token) { return Token_Debugable(token_map, token); }

class TokenReader {
protected:    
    virtual void RaiseUnexpectedSymbol(size_t namespace_id, const std::vector<size_t>& expected_symbols, size_t actual_symbol) =  0;
    virtual void RaiseAtLocation(std::string_view message) = 0;
    virtual Token ReadToken(size_t namespace_id) = 0;
public:
    virtual ~TokenReader() = default;

    virtual bool HasMore() = 0;
    Token NextToken(size_t namespace_id, TokenType a);
    Token NextToken(size_t namespace_id, TokenType a, TokenType b);
    Token NextKey(size_t namespace_id, const std::vector<size_t>& valid_key_ids);
    Token NextKey(size_t namespace_id, const std::vector<size_t>& valid_key_ids, TokenType b);
    Token NextEnum(size_t namespace_id, const std::vector<size_t>& valid_enum_ids);
    Token NextEnum(size_t namespace_id, const std::vector<size_t>& valid_enum_ids, TokenType b);
};

class TokenWriter {
public:
    virtual ~TokenWriter() = default;

    virtual void Complete() = 0;
    virtual void Write(const Token& token) = 0;
};

class TextTokenWriter : public TokenWriter {
private:
    const SymbolMap& tokens;
    std::ostream& out;
    std::vector<TokenType> scope;
    bool first_token = true;
    bool pending_begin_seq = false;
public:
    TextTokenWriter(const SymbolMap& tokens, std::ostream& out);

    void Complete() override;
    void Write(const Token& token) override;
};

class TextTokenReader : public TokenReader {
private:
    const SymbolMap& symbol_map;
    std::istream& in;
    long line = 1;
    long col = 1;
    long next_line = 1;
    long next_col = 1;
    char next = 0;
    bool has_more;
    bool expect_value = false;

    void NextChar(); // updates next, next_line, next_col, has_more.
    void ConsumeWhitespace(bool advance_one);
    size_t ReadSymbol(size_t namespace_id);
    
    void RaiseUnexpectedSymbol(size_t namespace_id, const std::vector<size_t>& expected_symbols, size_t actual_symbol) override;
    void RaiseAtLocation(std::string_view message) override;
    Token ReadToken(size_t namespace_id) override;
public:
    TextTokenReader(const SymbolMap& symbol_map, std::istream& in);
    
    bool HasMore() override { return has_more; }
};

class ByteTokenWriter : public TokenWriter {
private:
    const SymbolMap& tokens;
    std::ostream& out;
public:
    ByteTokenWriter(const SymbolMap& tokens, std::ostream& out);

    void Complete() override;
    void Write(const Token& token) override;
};

class ByteTokenReader : public TokenReader {
private:
    const SymbolMap& tokens;
    std::istream& in;
    size_t offset;

    void RaiseUnexpectedSymbol(size_t namespace_id, const std::vector<size_t>& expected_symbols, size_t actual_symbol) override;
    void RaiseAtLocation(std::string_view message) override;
    Token ReadToken(size_t namespace_id) override;
public:
    ByteTokenReader(const SymbolMap& tokens, std::istream& in);
    
    bool HasMore() override;
};

} // namespace