#pragma once

#include <iostream>
#include <variant>
#include <string>
#include <unordered_map>

#include "abbey/debug.h"

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
    SEP_SEQ,
};

struct Token {
    TokenType type;
    std::variant<
        std::monostate, // struct, seq
        std::pair<size_t, size_t>, // (namspace,token) key or enum
        std::string, // string
        long, // integer
        double // real
    > token;

    size_t GetKey() const { return std::get<std::pair<size_t, size_t>>(token).second; }
    size_t GetEnum() const { return std::get<std::pair<size_t, size_t>>(token).second; }
    std::string_view GetString() const { return std::get<std::string>(token); }
    long GetInteger() const { return std::get<long>(token); }
    double GetReal() const { return std::get<double>(token); }

    static inline Token Key(size_t namespace_id, size_t id) 
        { return Token{TokenType::KEY, std::pair{namespace_id, id}}; }
    static inline Token Enum(size_t namespace_id, size_t id) 
        { return Token{TokenType::ENUM, std::pair{namespace_id, id}}; }
    static inline Token String(std::string_view str) { return Token{TokenType::STRING, std::string(str)}; }
    static inline Token Integer(long n) { return Token{TokenType::INTEGER, n}; }
    static inline Token Real(double r) { return Token{TokenType::REAL, r}; }
    static inline Token BeginStruct() { return Token{TokenType::BEGIN_STRUCT}; }
    static inline Token EndStruct() { return Token{TokenType::END_STRUCT}; }
    static inline Token BeginSeq() { return Token{TokenType::BEGIN_SEQ}; }
    static inline Token EndSeq() { return Token{TokenType::END_SEQ}; }
    static inline Token SepSeq() { return Token{TokenType::SEP_SEQ}; }
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
public:
    virtual ~TokenReader() = default;

    virtual bool HasMore() = 0;
    virtual Token NextToken(size_t namespace_id, TokenType a) = 0;
    virtual Token NextToken(size_t namespace_id, TokenType a, TokenType b) = 0;
    virtual void RaiseUnexpectedToken(size_t namespace_id, const std::vector<size_t>& expected_tokens, size_t actual_token) =  0;
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
    const SymbolMap& tokens;
    std::istream& in;
    long line;
    long col;
public:
    TextTokenReader(const SymbolMap& tokens, std::istream& in);
    
    bool HasMore() override;
    Token NextToken(size_t namespace_id, TokenType a) override;
    Token NextToken(size_t namespace_id, TokenType a, TokenType b) override;
    void RaiseUnexpectedToken(size_t namespace_id, const std::vector<size_t>& expected_tokens, size_t actual_token) override;
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
public:
    ByteTokenReader(const SymbolMap& tokens, std::istream& in);
    
    bool HasMore() override;
    Token NextToken(size_t namespace_id, TokenType a) override;
    Token NextToken(size_t namespace_id, TokenType a, TokenType b) override;
    void RaiseUnexpectedToken(size_t namespace_id, const std::vector<size_t>& expected_tokens, size_t actual_token) override;
};

} // namespace