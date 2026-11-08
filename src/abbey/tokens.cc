#include "abbey/tokens.h"

#include <iomanip>
#include <sstream>
#include <cctype>

#include "abbey/error.h"

namespace abbey {

size_t SymbolNamespace::Register(std::string_view token) {
    size_t id = id_to_str.size();
    id_to_str.emplace_back(token);
    return id;
}

void SymbolNamespace::CommitRegistrations() {
    str_to_id.clear();
    str_to_id.reserve(id_to_str.size());
    for (size_t id = 0; id < id_to_str.size(); id++) {
        std::string_view str = id_to_str[id];
        if (str_to_id.find(str) != str_to_id.end()) {
            THROW_SYSTEM_ERROR(std::string("Duplicated token \"") + std::string(str) + "\" in namespace.");
        }
        str_to_id[id_to_str[id]] = id;
    }
}

size_t SymbolNamespace::IdForString(std::string_view token) const {
    auto found = str_to_id.find(std::string(token));
    if (found == str_to_id.end()) {
        THROW_SYSTEM_ERROR(std::string("Symbol \"") + std::string(token) + "\" not registered.");
    }
    return found->second;
}

std::string_view SymbolNamespace::StringForId(size_t id) const {
    if (id >= id_to_str.size()) {
        THROW_SYSTEM_ERROR(std::string("Symbol Id (") + std::to_string(id) + ") is not valid.");
    }
    return id_to_str[id];
}

size_t SymbolMap::DeclareNamespace() {
    size_t namespace_id = namespaces.size();
    namespaces.emplace_back(std::make_unique<SymbolNamespace>());
    return namespace_id;
}

size_t SymbolMap::Register(size_t namespace_id, std::string_view token) {
    if (namespace_id >= namespaces.size()) {
        THROW_SYSTEM_ERROR(std::string("Register called with undeclared namespace id."));
    }
    return namespaces[namespace_id]->Register(token);
}

void SymbolMap::CommitRegistrations() {
    for (auto& token_namespace : namespaces) {
        token_namespace->CommitRegistrations();
    }
}

const SymbolNamespace& SymbolMap::GetNamespace(size_t namespace_id) const {
    if (namespace_id >= namespaces.size()) {
        THROW_SYSTEM_ERROR(std::string("Register called with undeclared namespace id."));
    }
    return *namespaces[namespace_id];
}

std::ostream& TokenType_Debugable::Debug(std::string_view, std::string_view, std::ostream& out) const {
    switch(t) {
        case TokenType::KEY: return out << "KEY";
        case TokenType::ENUM: return out << "ENUM";
        case TokenType::INTEGER: return out << "INTEGER";
        case TokenType::REAL: return out << "REAL";
        case TokenType::STRING: return out << "STRING";
        case TokenType::BEGIN_STRUCT: return out << "BEGIN_STRUCT";
        case TokenType::END_STRUCT: return out << "END_STRUCT";
        case TokenType::BEGIN_SEQ: return out << "BEGIN_SEQ";
        case TokenType::END_SEQ: return out << "END_SEQ";
    }
    return out;
}

std::ostream& Token_Debugable::Debug(std::string_view, std::string_view, std::ostream& out) const {
    out << MakeDebugable(token.type);
    switch(token.type) {
        case TokenType::KEY: 
        case TokenType::ENUM: {
            auto namespace_symbol = std::get<std::pair<size_t, size_t>>(token.token);
            return out << "(" << token_map.StringForId(namespace_symbol.first, namespace_symbol.second) << ")";
        }
        case TokenType::INTEGER: 
            return out << "(" << token.GetInteger() << ")";
        case TokenType::REAL: 
            return out << "(" << token.GetReal() << ")";
        case TokenType::STRING:
            return out << "(" << token.GetString() << ")";
        case TokenType::BEGIN_STRUCT: return out;
        case TokenType::END_STRUCT: return out;
        case TokenType::BEGIN_SEQ: {
            std::optional<size_t> count = token.GetCount();
            if (count.has_value()) {
                out << "(" << *count << ")";
            }
            return out;
        }
        case TokenType::END_SEQ: return out;
    }
    return out;
}

namespace {

bool id_in_set(size_t id, const std::vector<size_t>& valid_set) {
    for (size_t v : valid_set) {
        if (id == v) {
            return true;
        }
    }
    return false;
}

} // namespace

Token TokenReader::NextToken(size_t namespace_id, TokenType a) {
    Token result = ReadToken(namespace_id);
    if (result.type == TokenType::INTEGER && a == TokenType::REAL) {
        // allow integer to implicitly cast to real.
        result.type = TokenType::REAL;
        result.token = static_cast<double>(std::get<long>(result.token));
    }
    if (result.type != a) {
        std::stringstream message;
        message << "Expected token type " << MakeDebugable(a) << ", found " << MakeDebugable(result.type);
        RaiseAtLocation(message.str());
    }
    return result;
}

Token TokenReader::NextToken(size_t namespace_id, TokenType a, TokenType b) {
    Token result = ReadToken(namespace_id);
    if (result.type == TokenType::INTEGER && (a == TokenType::REAL && b != TokenType::INTEGER || b == TokenType::REAL && a != TokenType::INTEGER)) {
        // allow integer to implicitly cast to real.
        result.type = TokenType::REAL;
        result.token = static_cast<double>(std::get<long>(result.token));
    }
    if (result.type != a && result.type != b) {
        std::stringstream message;
        message << "Expected token type " << MakeDebugable(a) << " or " << MakeDebugable(b) << ", found " << MakeDebugable(result.type);
        RaiseAtLocation(message.str());
    }
    return result;
}

Token TokenReader::NextKey(size_t namespace_id, const std::vector<size_t>& valid_key_ids) {
    Token t = NextToken(namespace_id, TokenType::KEY);
    if (!id_in_set(t.GetKey(), valid_key_ids)) {
        RaiseUnexpectedSymbol(namespace_id, valid_key_ids, t.GetKey());
    }
    return t;
}

Token TokenReader::NextKey(size_t namespace_id, const std::vector<size_t>& valid_key_ids, TokenType b) {
    Token t = NextToken(namespace_id, TokenType::KEY, b);
    if (t.type == b) {
        return t;
    }
    if (!id_in_set(t.GetKey(), valid_key_ids)) {
        RaiseUnexpectedSymbol(namespace_id, valid_key_ids, t.GetKey());
    }
    return t;
}

Token TokenReader::NextEnum(size_t namespace_id, const std::vector<size_t>& valid_enum_ids) {
    Token t = NextToken(namespace_id, TokenType::ENUM);
    if (!id_in_set(t.GetEnum(), valid_enum_ids)) {
        RaiseUnexpectedSymbol(namespace_id, valid_enum_ids, t.GetEnum());
    }
    return t;
}

Token TokenReader::NextEnum(size_t namespace_id, const std::vector<size_t>& valid_enum_ids, TokenType b) {
    Token t = NextToken(namespace_id, TokenType::ENUM, b);
    if (t.type == b) {
        return t;
    } 
    if (!id_in_set(t.GetEnum(), valid_enum_ids)) {
        RaiseUnexpectedSymbol(namespace_id, valid_enum_ids, t.GetEnum());
    }
    return t; 
}

TextTokenWriter::TextTokenWriter(const SymbolMap& tokens, std::ostream& out)
: tokens(tokens)
, out(out)
{
    out.imbue(std::locale("en_US.UTF-8"));
}

void TextTokenWriter::Complete() {
    out.flush();
}

namespace {
void scope_prefix(const std::vector<TokenType>& scope, std::ostream& out) {
    out << "\n" << std::string(scope.size() * 2, ' ');
}
void require_scope(const std::vector<TokenType>& scope, TokenType required, TokenType desired, bool or_top_level = false) {
    if (scope.empty()) {
        if (or_top_level) {
            return;
        }
        std::stringstream error_message;
        error_message << "Cannot write " << MakeDebugable(desired) << " at top level.";
        THROW_SYSTEM_ERROR(error_message.str());
    }

    if (scope.back() != required) {
        std::stringstream error_message;
        error_message << "Cannot write " << MakeDebugable(desired) << " in scope " << MakeDebugable(scope.back())
            << ", only valid in scope " << MakeDebugable(required) << ".";
        THROW_SYSTEM_ERROR(error_message.str());
    }
}

bool needs_comma(const std::vector<TokenType>& scope, bool pending_begin_seq) {
    if (scope.empty() || scope.back() != TokenType::BEGIN_SEQ) return false;
    return !pending_begin_seq;
}

void write_comma(const std::vector<TokenType>& scope, std::ostream& out) {
    out << ",";
    scope_prefix(scope, out);
}
}

void TextTokenWriter::Write(const Token& token) {
    bool enable_comma = needs_comma(scope, pending_begin_seq);
    if (pending_begin_seq) {
        pending_begin_seq = false;
        if (token.type == TokenType::END_SEQ) {
            // handle empty collection cleanly.
            out << "[]";
            return;
        }
        scope.emplace_back(TokenType::BEGIN_SEQ);
        out.put('[');
        scope_prefix(scope, out);
    }

    switch(token.type) {
        case TokenType::KEY: {
            require_scope(scope, TokenType::BEGIN_STRUCT, token.type, true);
            if (!first_token) {
                scope_prefix(scope, out);
            }
            auto namespace_symbol = std::get<std::pair<size_t, size_t>>(token.token);
            out << tokens.StringForId(namespace_symbol.first, namespace_symbol.second);
            out << ":";
        } break; 
        case TokenType::ENUM: {
            if (enable_comma) {
                write_comma(scope, out);
            }
            auto namespace_symbol = std::get<std::pair<size_t, size_t>>(token.token);
            out << tokens.StringForId(namespace_symbol.first, namespace_symbol.second);
        } break;
        case TokenType::INTEGER: {
            if (enable_comma) {
                write_comma(scope, out);
            }
            out << token.GetInteger();
        } break;
        case TokenType::REAL: {
            if (enable_comma) {
                write_comma(scope, out);
            }
            // This will be lossy, we're just going to eat that risk.
            out << std::scientific << std::setprecision(12) << token.GetReal();
        } break;
        case TokenType::STRING: {
            if (enable_comma) {
                write_comma(scope, out);
            }
            std::string_view cs = token.GetString();
            out.put('"');
            for (char c : cs) {
                // handling of multi-line strings will be ugly, but should still work.
                if (c == '\\' || c == '"') {
                    out.put('\\');
                }
                out.put(c);
            }
            out.put('"');
        } break;
        case TokenType::BEGIN_STRUCT: {
            if (enable_comma) {
                write_comma(scope, out);
            }
            out << "{";
            scope.emplace_back(TokenType::BEGIN_STRUCT);
        } break;
        case TokenType::END_STRUCT: {
            require_scope(scope, TokenType::BEGIN_STRUCT, token.type);
            scope.pop_back();
            scope_prefix(scope, out);
            out << '}';
        } break;
        case TokenType::BEGIN_SEQ: {
            if (enable_comma) {
                write_comma(scope, out);
            }
            pending_begin_seq = true;
        } break;
        case TokenType::END_SEQ: {
            require_scope(scope, TokenType::BEGIN_SEQ, token.type);
            scope.pop_back();
            scope_prefix(scope, out);
            out << ']';
        } break;
    }
    first_token = false; 
}

TextTokenReader::TextTokenReader(const SymbolMap& symbol_map, std::istream& in) 
: symbol_map(symbol_map)
, in(in)
, has_more(in.good())
{
    in.imbue(std::locale("en_US.UTF-8"));
    ConsumeWhitespace(true);
}

void TextTokenReader::NextChar() {
    in.get(next);
    if (next == '\n') {
        next_col = 0;
        ++next_line;
    } else {
        ++next_col;
    }
    has_more = in.good();
}

void TextTokenReader::ConsumeWhitespace(bool advance_one) {
    if (has_more && advance_one) {
        NextChar();
    }
    while (has_more && std::isspace(next)) {
        NextChar();
    }
}

void TextTokenReader::RaiseAtLocation(std::string_view message) {
    std::stringstream location;
    location << " At line: " << line << " col: " << col << ".";
    THROW_SYSTEM_ERROR(std::string(message) + location.str());
}

void TextTokenReader::RaiseUnexpectedSymbol(size_t namespace_id, const std::vector<size_t>& expected_symbols, size_t actual_symbol) {
    std::stringstream message;

    message << ": found symbol '" << symbol_map.StringForId(namespace_id, actual_symbol) << "', expected one of (";

    size_t count = 0;
    for (size_t expected : expected_symbols) {
        message << "'" << symbol_map.StringForId(namespace_id, expected) << "'";
        ++count;
        if (count != expected_symbols.size()) {
            message << ", ";
        }
    }
    message << ")";
    RaiseAtLocation(message.str());
}

namespace {
bool IsSpecial(char c) {
    return c == ':' || c == '{' || c == '}' || c == '[' || c == ']' || c == ',' || c == '"';
}
}

size_t TextTokenReader::ReadSymbol(size_t namespace_id) {
    std::string buffer;
    buffer.reserve(30);
    while (!IsSpecial(next) && !std::isspace(next)) {
        buffer.push_back(next);
        if (!has_more) break;
        NextChar();
    }
    try {  
        return symbol_map.IdForString(namespace_id, buffer); 
    } catch (const SystemError&) {
        std::stringstream msg;
        msg << "Failed to read symbol: \"" << buffer << "\" is not a registered symbol";
        RaiseAtLocation(msg.str());
        return 0;
    }
}

Token TextTokenReader::ReadToken(size_t namespace_id) {
    if (!has_more) {
        RaiseAtLocation("Unexpected end of stream");
    }
    col = next_col;
    line = next_line;

    if (expect_value) {
        expect_value = false;
        if (std::isdigit(next) || next == '-' || next == '+') {
            std::string buffer;
            bool is_real = false;
            buffer.reserve(30);
            while (std::isdigit(next) || next == '-' || next == '+' || next == 'e' || next == 'E' || next == '.') {
                if (next == '.' || next == 'e' || next == 'E') {
                    is_real = true;
                }
                buffer.push_back(next);
                if (!has_more) break;
                NextChar();
            }
            ConsumeWhitespace(false);
            try {
                if (is_real) {
                    size_t end;
                    double value = std::stod(buffer, &end);
                    if (end != buffer.size()) {
                        std::stringstream message;
                        message << "Failed to parse real number '" << buffer << "'";
                        THROW_SYSTEM_ERROR(message.str());
                    }
                    return Token::Real(value);
                }
                size_t end;
                long value = std::stol(buffer, &end);
                if (end != buffer.size()) {
                    std::stringstream message;
                    message << "Failed to parse integer '" << buffer << "'";
                    THROW_SYSTEM_ERROR(message.str());
                }
                return Token::Integer(value);
            } catch (const std::invalid_argument&) {
                std::stringstream message;
                message << "Failed to parse as number: '" << buffer << "'";
                RaiseAtLocation(message.str());
            } catch (const std::out_of_range&) {
                std::stringstream message;
                message << "Number out of representable range: '" << buffer << "'";
                RaiseAtLocation(message.str());
            }
        } else if (next == '[') {
            expect_value = true;
            ConsumeWhitespace(true);
            return Token::BeginSeq();
        } else if (next == ']') {
            // Would typically occur after reading a ',',
            // But can occur immediately after '['.
            ConsumeWhitespace(true);
            return Token::EndSeq();
        } else if (next == '{') {
            ConsumeWhitespace(true);
            return Token::BeginStruct();
        } else if (next == '"') {
            std::string buffer;
            bool escape = false;
            buffer.reserve(50);
            while (has_more) {
                NextChar();
                if (escape) {
                    escape = false;
                    buffer.push_back(next);
                    continue;
                } else if (next == '"') {
                    break;
                } else if (next == '\\') {
                    escape = true;
                    continue;
                }
                buffer.push_back(next);
            }
            ConsumeWhitespace(true); // eat the closing "
            return Token::String(std::move(buffer));
        } else {
            size_t symbol = ReadSymbol(namespace_id);     
            ConsumeWhitespace(false);
            return Token::Enum(namespace_id, symbol);
        }
    }
    if (next == ':') {
        expect_value = true;
        ConsumeWhitespace(true);
        return ReadToken(namespace_id);
    } else if (next == '}') {
        ConsumeWhitespace(true);
        return Token::EndStruct();
    } else if (next == ',') {
        expect_value = true;
        ConsumeWhitespace(true);
        return ReadToken(namespace_id);
    } else if (next == ']') {
        ConsumeWhitespace(true);
        return Token::EndSeq();
    } 
    size_t symbol = ReadSymbol(namespace_id);     
    ConsumeWhitespace(false);
    return Token::Key(namespace_id, symbol);

}

}  // abbey