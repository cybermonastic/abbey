#include "abbey/tokens.h"

#include <iomanip>
#include <sstream>

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
        THROW_SYSTEM_ERROR(std::string("Token \"") + std::string(token) + "\" not registered.");
    }
    return found->second;
}

std::string_view SymbolNamespace::StringForId(size_t id) const {
    if (id >= id_to_str.size()) {
        THROW_SYSTEM_ERROR(std::string("Token Id (") + std::to_string(id) + ") is not valid.");
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
        case TokenType::SEP_SEQ: return out << "SEP_SEQ";
    }
    return out;
}

std::ostream& Token_Debugable::Debug(std::string_view, std::string_view, std::ostream& out) const {
    out << MakeDebugable(token.type);
    switch(token.type) {
        case TokenType::KEY: 
        case TokenType::ENUM: {
            auto namespace_token = std::get<std::pair<size_t, size_t>>(token.token);
            return out << "(" << token_map.StringForId(namespace_token.first, namespace_token.second) << ")";
        }
        case TokenType::INTEGER: 
            return out << "(" << token.GetInteger() << ")";
        case TokenType::REAL: 
            return out << "(" << token.GetReal() << ")";
        case TokenType::STRING:
            return out << "(" << token.GetString() << ")";
        case TokenType::BEGIN_STRUCT: return out;
        case TokenType::END_STRUCT: return out;
        case TokenType::BEGIN_SEQ: return out;
        case TokenType::END_SEQ: return out;
        case TokenType::SEP_SEQ: return out;
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

Token TokenReader::NextKey(size_t namespace_id, const std::vector<size_t>& valid_key_ids) {
    Token t = NextToken(namespace_id, TokenType::KEY);
    if (!id_in_set(t.GetKey(), valid_key_ids)) {
        RaiseUnexpectedToken(namespace_id, valid_key_ids, t.GetKey());
    }
    return t;
}

Token TokenReader::NextKey(size_t namespace_id, const std::vector<size_t>& valid_key_ids, TokenType b) {
    Token t = NextToken(namespace_id, TokenType::KEY, b);
    if (t.type == b) {
        return t;
    }
    if (!id_in_set(t.GetKey(), valid_key_ids)) {
        RaiseUnexpectedToken(namespace_id, valid_key_ids, t.GetKey());
    }
    return t;
}

Token TokenReader::NextEnum(size_t namespace_id, const std::vector<size_t>& valid_enum_ids) {
    Token t = NextToken(namespace_id, TokenType::ENUM);
    if (!id_in_set(t.GetEnum(), valid_enum_ids)) {
        RaiseUnexpectedToken(namespace_id, valid_enum_ids, t.GetEnum());
    }
    return t;
}

Token TokenReader::NextEnum(size_t namespace_id, const std::vector<size_t>& valid_enum_ids, TokenType b) {
    Token t = NextToken(namespace_id, TokenType::ENUM, b);
    if (t.type == b) {
        return t;
    } 
    if (!id_in_set(t.GetEnum(), valid_enum_ids)) {
        RaiseUnexpectedToken(namespace_id, valid_enum_ids, t.GetEnum());
    }
    return t; 
}

TextTokenWriter::TextTokenWriter(const SymbolMap& tokens, std::ostream& out)
: tokens(tokens)
, out(out)
{}

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
}

void TextTokenWriter::Write(const Token& token) {
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
            auto namespace_token = std::get<std::pair<size_t, size_t>>(token.token);
            out << tokens.StringForId(namespace_token.first, namespace_token.second);
            out << ":";
        } break; 
        case TokenType::ENUM: {
            auto namespace_token = std::get<std::pair<size_t, size_t>>(token.token);
            out << tokens.StringForId(namespace_token.first, namespace_token.second);
        } break;
        case TokenType::INTEGER: {
            out << token.GetInteger();
        } break;
        case TokenType::REAL: {
            // This will be lossy, we're just going to eat that risk.
            out << std::scientific << std::setprecision(12) << token.GetReal();
        } break;
        case TokenType::STRING: {
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
            scope.emplace_back(TokenType::BEGIN_STRUCT);
            out.put('{');
        } break;
        case TokenType::END_STRUCT: {
            require_scope(scope, TokenType::BEGIN_STRUCT, token.type);
            scope.pop_back();
            scope_prefix(scope, out);
            out << '}';
        } break;
        case TokenType::BEGIN_SEQ: {
            pending_begin_seq = true;
        } break;
        case TokenType::END_SEQ: {
            require_scope(scope, TokenType::BEGIN_SEQ, token.type);
            scope.pop_back();
            scope_prefix(scope, out);
            out << ']';
        } break;
        case TokenType::SEP_SEQ: {
            require_scope(scope, TokenType::BEGIN_SEQ, token.type);
            out << ',';
            scope_prefix(scope, out);
        } break;
    }
    first_token = false; 
}

// Test TextTokenWriter::Write before doing anything further.

}  // abbey