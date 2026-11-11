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
            return out << "(" << symbol_map.StringForId(namespace_symbol.first, namespace_symbol.second) << ")";
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

namespace {

// context determines if seq, struct, (not used with SEQ_SUFFIX_WITH_SIZE)
constexpr uint8_t BEGIN_PREFIX = 0x00; 
// context determines if seq, struct
constexpr uint8_t END_PREFIX = 0xE0; 
// 0xC0
// rest of the byte is the integer value.
constexpr uint8_t TINY_INTEGER_PREFIX = 0xA0;
// rest of the byte is the symbol value.
constexpr uint8_t TINY_SYMBOL_PREFIX = 0x80;
// next two bytes are the integer value.
constexpr uint8_t SMALL_PREFIX = 0x60;
// next four bytes hold an integer, or next eight bytes hold a double.
constexpr uint8_t FULL_PREFIX = 0x40;
// 0x20

// 0x01
constexpr uint8_t SEQ_SUFFIX = 0x02;
constexpr uint8_t STRUCT_SUFFIX = 0x03;
constexpr uint8_t SYMBOL_SUFFIX = 0x04;
constexpr uint8_t INTEGER_SUFFIX = 0x05;
constexpr uint8_t REAL_SUFFIX = 0x06;
// 0x07
// 0x08
constexpr uint8_t STRING_SUFFIX_WITH_SIZE = 0x09;
constexpr uint8_t SEQ_SUFFIX_WITH_SIZE = 0x0A;

uint8_t GetPrefix(uint8_t byte) { return 0xE0 & byte; }
uint8_t GetSuffix(uint8_t byte) { return 0x1F & byte; }

TokenType GetTokenType(uint8_t byte, bool expect_key) {
    uint8_t suffix = GetSuffix(byte);
    if (suffix == STRING_SUFFIX_WITH_SIZE) {
        return TokenType::STRING;
    }
    if (suffix == REAL_SUFFIX) {
        return TokenType::REAL;
    }
    uint8_t prefix = GetPrefix(byte);
    if (prefix == TINY_INTEGER_PREFIX) {
        return TokenType::INTEGER;
    }
    if (prefix == TINY_SYMBOL_PREFIX) {
        return expect_key ? TokenType::KEY : TokenType::ENUM;
    }
    if (prefix == BEGIN_PREFIX) {
        if (suffix == STRUCT_SUFFIX) {
            return TokenType::BEGIN_STRUCT;
        } else if (suffix == SEQ_SUFFIX) {
            return TokenType::BEGIN_SEQ;
        }
    }
    if (prefix == END_PREFIX) {
        if (suffix == STRUCT_SUFFIX) {
            return TokenType::END_STRUCT;
        } else if (suffix == SEQ_SUFFIX) {
            return TokenType::END_SEQ;
        }
    }
    if (suffix == SEQ_SUFFIX_WITH_SIZE) {
        return TokenType::BEGIN_SEQ;
    }
    if (suffix == INTEGER_SUFFIX) {
        return TokenType::INTEGER;
    }
    if (suffix == SYMBOL_SUFFIX) {
        return expect_key ? TokenType::KEY : TokenType::ENUM;
    }
    std::stringstream message;
    message << "Could not parse byte '" << std::hex << static_cast<int>(byte) << "'";
    THROW_SYSTEM_ERROR(message.str());
}

bool HasSize(uint8_t byte) {  
    return (GetSuffix(byte) & 0x08) > 0;
}

void WriteInteger(TokenType type, long value, std::ostream& out) {
    if (value >= 0 && value <= 0x1F && type != TokenType::STRING && type != TokenType::BEGIN_SEQ) {
        // Can use tiny representation.
        uint8_t byte = static_cast<uint8_t>(value);
        if (type == TokenType::ENUM || type == TokenType::KEY) {
            byte += TINY_SYMBOL_PREFIX;
        } else {
            byte += TINY_INTEGER_PREFIX;
        }
        out.put(byte);
        return;
    }
    
    if (value == static_cast<int16_t>(value)) {
        // Can use short representation.
        uint8_t control = SMALL_PREFIX;
        if (type == TokenType::ENUM || type == TokenType::KEY) {
            control += SYMBOL_SUFFIX;
        } else if (type == TokenType::BEGIN_SEQ) {
            control += SEQ_SUFFIX_WITH_SIZE;
        } else if (type == TokenType::STRING) {
            control += STRING_SUFFIX_WITH_SIZE;
        } else {
            control += INTEGER_SUFFIX;
        }
        out.put(control);
        out.put(static_cast<uint8_t>((static_cast<uint16_t>(value) >> 8) & 0x00FF));
        out.put(static_cast<uint8_t>(static_cast<uint16_t>(value) & 0x00FF));
        return;
    }

    // Otherwise use long representation.
    uint8_t control = FULL_PREFIX;
    if (type == TokenType::ENUM || type == TokenType::KEY) {
        control += SYMBOL_SUFFIX;
    }  else if (type == TokenType::BEGIN_SEQ) {
        control += SEQ_SUFFIX_WITH_SIZE;
    } else if (type == TokenType::STRING) {
        control += STRING_SUFFIX_WITH_SIZE;
    } else {
        control += INTEGER_SUFFIX;
    }
    out.put(control);
    out.put(static_cast<uint8_t>((static_cast<uint32_t>(value) >> 24) & 0x00FF));
    out.put(static_cast<uint8_t>((static_cast<uint32_t>(value) >> 16) & 0x00FF));
    out.put(static_cast<uint8_t>((static_cast<uint32_t>(value) >> 8) & 0x00FF));
    out.put(static_cast<uint8_t>(static_cast<uint32_t>(value) & 0x00FF));
}

void WriteReal(double value, std::ostream& out) {
    out.put(FULL_PREFIX + REAL_SUFFIX);
    std::vector<uint8_t> bytes(sizeof(double));
    std::memcpy(bytes.data(), &value, sizeof(double));
    out.write(reinterpret_cast<const char *>(bytes.data()), sizeof(double));
}

} // namespace

ByteTokenWriter::ByteTokenWriter(std::ostream& out) : out(out) {}

void ByteTokenWriter::Complete() {
    out.flush();
}

void ByteTokenWriter::Write(const Token& token) {
    switch(token.type) {
        case TokenType::KEY: {
            WriteInteger(token.type, static_cast<long>(token.GetKey()), out);
        } return;
        case TokenType::ENUM: {
            WriteInteger(token.type, static_cast<long>(token.GetEnum()), out);
        } return;
        case TokenType::INTEGER: {
            WriteInteger(token.type, token.GetInteger(), out);
        } return;
        case TokenType::REAL: {
            WriteReal(token.GetReal(), out);
        } return;
        case TokenType::STRING: {
            std::string_view value = token.GetString();
            WriteInteger(token.type, static_cast<long>(value.size()), out);
            out.write(value.data(), value.size());
        } return;
        case TokenType::BEGIN_STRUCT: {
            out.put(static_cast<char>(BEGIN_PREFIX + STRUCT_SUFFIX));
        } return;
        case TokenType::END_STRUCT: {
            out.put(static_cast<char>(END_PREFIX + STRUCT_SUFFIX));
        } return;
        case TokenType::BEGIN_SEQ: {
            auto count = token.GetCount();
            if (count == std::nullopt) {
                out.put(static_cast<char>(BEGIN_PREFIX + SEQ_SUFFIX));
            } else {
                WriteInteger(token.type, static_cast<long>(*count), out); 
            }
        } return;
        case TokenType::END_SEQ: {
            out.put(static_cast<char>(END_PREFIX + SEQ_SUFFIX));
        } return;
    }
}

ByteTokenReader::ByteTokenReader(const SymbolMap& symbols, std::istream& in) : symbols(symbols), in(in) {
    GetByte();
}

uint8_t ByteTokenReader::GetByte() {
    if (!in.good()) {
        std::stringstream msg;
        msg << "Unexpected end of file at offset " << offset;
        THROW_SYSTEM_ERROR(msg.str());
    }
    ++offset;
    uint8_t byte = next_byte;

    // read one byte in advance, to find out if we've reached end of stream.
    next_byte = static_cast<uint8_t>(in.get());
    has_more = in.good();
    return byte;
}

long ByteTokenReader::ReadInteger(uint8_t control) {
    uint8_t prefix = GetPrefix(control);

    if (prefix == TINY_INTEGER_PREFIX || prefix == TINY_SYMBOL_PREFIX) {
        return static_cast<int8_t>(control & 0x1F);
    }
    if (prefix == SMALL_PREFIX) {
        uint16_t result = GetByte();
        result <<= 8;
        result += GetByte();
        return static_cast<int16_t>(result);
    }
    uint32_t result = GetByte();
    result <<= 8;
    result += GetByte();
    result <<= 8;
    result += GetByte();
    result <<= 8;
    result += GetByte();
    return static_cast<long>(result);
}

double ByteTokenReader::ReadReal() {
    std::vector<uint8_t> bytes(sizeof(double));
    for (size_t i = 0; i < sizeof(double); i++) {
        bytes[i] = GetByte();
    }
    double value;
    std::memcpy(&value, bytes.data(), sizeof(double));
    return value;
}

std::string ByteTokenReader::ReadString(size_t count) {
    std::string result;
    result.resize(count);
    for (size_t i = 0; i < count; i++) {
        result[i] = GetByte();
    }
    return result;
}

void ByteTokenReader::RaiseUnexpectedSymbol(size_t namespace_id, const std::vector<size_t>& expected_symbols, size_t actual_symbol) {
    std::stringstream message;

    message << ": found symbol '" << symbols.StringForId(namespace_id, actual_symbol) << "', expected one of (";

    size_t count = 0;
    for (size_t expected : expected_symbols) {
        message << "'" << symbols.StringForId(namespace_id, expected) << "'";
        ++count;
        if (count != expected_symbols.size()) {
            message << ", ";
        }
    }
    message << ")";
    RaiseAtLocation(message.str());
}

void ByteTokenReader::RaiseAtLocation(std::string_view message) {
    std::stringstream location;
    location << " before offset: " << offset << ".";
    THROW_SYSTEM_ERROR(std::string(message) + location.str());
}

bool ByteTokenReader::InSeq() {
    if (scope.empty() || scope.back() != TokenType::BEGIN_SEQ) {
        return false;
    }
    return true;
}

Token ByteTokenReader::ReadToken(size_t namespace_id) {
    uint8_t control = GetByte();
    switch (GetTokenType(control, expect_key)) {
        case TokenType::KEY: {
            expect_key = false;
            size_t id = ReadInteger(control);
            if (!symbols.HasSymbol(namespace_id, id)) {
                std::stringstream msg;
                msg << "Invalid symbol id " << id;
                RaiseAtLocation(msg.str());
            }
            return Token::Key(namespace_id, id);
        }
        case TokenType::ENUM: {
            expect_key = !InSeq();
            size_t id = ReadInteger(control);
            if (!symbols.HasSymbol(namespace_id, id)) {
                std::stringstream msg;
                msg << "Invalid symbol id " << id;
                RaiseAtLocation(msg.str());
            }
            return Token::Enum(namespace_id, id);
        }
        case TokenType::INTEGER: {
            expect_key = !InSeq();
            long value = ReadInteger(control);
            return Token::Integer(value);
        }
        case TokenType::REAL: {
            expect_key = !InSeq();
            double value = ReadReal();
            return Token::Real(value);
        }
        case TokenType::STRING: {
            expect_key = !InSeq();
            size_t byte_count = ReadInteger(control);
            return Token::String(ReadString(byte_count));
        }
        case TokenType::BEGIN_STRUCT: {
            expect_key = true;
            scope.push_back(TokenType::BEGIN_STRUCT);
            return Token::BeginStruct();
        }
        case TokenType::END_STRUCT: {
            if (scope.empty() || scope.back() != TokenType::BEGIN_STRUCT) {
                RaiseAtLocation("Unexpected END_STRUCT control byte");
            }
            scope.pop_back();
            expect_key = !InSeq();
            return Token::EndStruct();
        }
        case TokenType::BEGIN_SEQ: {
            scope.push_back(TokenType::BEGIN_SEQ);
            expect_key = false;
            if (HasSize(control)) {
                size_t count = ReadInteger(control);
                return Token::BeginSeqWithSize(count);
            }
            return Token::BeginSeq();
        }
        case TokenType::END_SEQ: {
            if (scope.empty() || scope.back() != TokenType::BEGIN_SEQ) {
                RaiseAtLocation("Unexpected END_SEQ control byte");
            }
            scope.pop_back();
            expect_key = !InSeq();
            return Token::EndSeq();
        }
    }
    std::stringstream msg;
    msg << "Invalid control byte " << std::hex << control;
    RaiseAtLocation(msg.str());
    return Token::EndStruct(); // unreachable.
}

}  // abbey