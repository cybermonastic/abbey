#include "tokens.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include "test.h"
#include "abbey\tokens.h"
#include "abbey\error.h"

namespace {

using abbey::Token;
using abbey::TokenType;
using abbey::SymbolMap;
using abbey::TextTokenWriter;
using abbey::TextTokenReader;
using abbey::SystemError;
using abbey::ByteTokenWriter;
using abbey::ByteTokenReader;

struct SymbolHolder {
    SymbolMap symbol_map;

    size_t outer_ns;
    size_t name;
    size_t count;
    size_t inner;
    size_t inner_ns;
    size_t experimental;
    size_t yes;
    size_t no;
    size_t real1;
    size_t real2;
    size_t collection;

    void PrepareSymbols() {
        outer_ns = symbol_map.DeclareNamespace();
        name = symbol_map.Register(outer_ns, "name");
        count = symbol_map.Register(outer_ns, "count");
        inner = symbol_map.Register(outer_ns, "inner");

        inner_ns = symbol_map.DeclareNamespace();
        experimental = symbol_map.Register(inner_ns, "experimental");
        yes = symbol_map.Register(inner_ns, "yes");
        no = symbol_map.Register(inner_ns, "no");
        real1 = symbol_map.Register(inner_ns, "real1");
        real2 = symbol_map.Register(inner_ns, "real2");
        collection = symbol_map.Register(inner_ns, "collection");

        symbol_map.CommitRegistrations();
    }   
};

int RunTokensTextWriter(const SymbolHolder &symbols, int argc, const char *argv[]) {
    BEGIN_TEST("text_writer");

    std::stringstream buf;
    TextTokenWriter writer(symbols.symbol_map, buf);
    writer.Write(Token::Key(symbols.outer_ns, symbols.name));
    writer.Write(Token::String("he said, \"Hello \\World!\""));
    writer.Write(Token::Key(symbols.outer_ns, symbols.count));
    writer.Write(Token::Integer(2));
    writer.Write(Token::Key(symbols.outer_ns, symbols.inner));

    writer.Write(Token::BeginStruct());
    writer.Write(Token::Key(symbols.inner_ns, symbols.experimental));
    writer.Write(Token::Enum(symbols.inner_ns, symbols.yes));
    writer.Write(Token::Key(symbols.inner_ns, symbols.collection));
    
    writer.Write(Token::BeginSeq());
    writer.Write(Token::Integer(100));
    writer.Write(Token::Integer(200));
    writer.Write(Token::EndSeq());

    writer.Write(Token::Key(symbols.inner_ns, symbols.real1));
    writer.Write(Token::Real(0.0));
    writer.Write(Token::Key(symbols.inner_ns, symbols.real2));
    writer.Write(Token::Real(7.8965e-9));
    writer.Write(Token::EndStruct());

    std::stringstream expected;
    expected << R"(name:"he said, \"Hello \\World!\""
count:2
inner:{
  experimental:yes
  collection:[
    100,
    200
  ]
  real1:)" << std::scientific << std::setprecision(12) << 0.00000000000
  << "\n  real2:" <<  7.8965e-9 << "\n}";

    EXPECT(buf.str(), expected.str());
    return 0;
}

int RunTokensEmptyCollection(const SymbolHolder &symbols, int argc, const char *argv[]) {
    BEGIN_TEST("empty_collection");

    std::stringstream buf;
    TextTokenWriter writer(symbols.symbol_map, buf);
    writer.Write(Token::Key(symbols.inner_ns, symbols.collection));
    
    writer.Write(Token::BeginSeq());
    writer.Write(Token::BeginSeq());
    writer.Write(Token::EndSeq());
    writer.Write(Token::BeginSeq());
    writer.Write(Token::EndSeq());
    writer.Write(Token::EndSeq());

    std::stringstream expected;
    expected << R"(collection:[
  [],
  []
])";

    EXPECT(buf.str(), expected.str());
    return 0;
}

int RunTokensBadStruct(const SymbolHolder &symbols, int argc, const char *argv[]) {
    BEGIN_TEST("bad_struct");

    try {
        std::stringstream buf;
        TextTokenWriter writer(symbols.symbol_map, buf);
        writer.Write(Token::Key(symbols.inner_ns, symbols.collection));
        
        writer.Write(Token::BeginSeq());
        writer.Write(Token::EndStruct());
    } catch (const SystemError&){
        return 0;
    }

    std::cout << "Expected exception, none thrown.\n";
    return 1;
}

int RunTokensBadSeq(const SymbolHolder &symbols, int argc, const char *argv[]) {
    BEGIN_TEST("bad_seq");

    try {
        std::stringstream buf;
        TextTokenWriter writer(symbols.symbol_map, buf);
        writer.Write(Token::Key(symbols.inner_ns, symbols.collection));
        
        writer.Write(Token::BeginStruct());
        writer.Write(Token::EndSeq());
    } catch (const SystemError&){
        return 0;
    }

    std::cout << "Expected exception, none thrown.\n";
    return 1;
}

int RunTokensRead(const SymbolHolder &symbols, int argc, const char *argv[]) {
    BEGIN_TEST("read");

    const char* const input = R"(name:"he said, \"Hello World!\""
count:2
inner:{
  experimental:yes
  collection:[
    100,
    200
  ]
  real1: 0.00
  real2: 7.8965e-9
})";
    std::istringstream in(input);
    TextTokenReader reader(symbols.symbol_map, in);
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextKey(symbols.outer_ns, {symbols.name, symbols.count, symbols.inner}).GetKey(), symbols.name);
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextToken(symbols.outer_ns, TokenType::STRING).ConsumeString(), "he said, \"Hello World!\"");
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextKey(symbols.outer_ns, {symbols.name, symbols.count, symbols.inner}).GetKey(), symbols.count);
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextToken(symbols.outer_ns, TokenType::INTEGER).GetInteger(), 2);
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextKey(symbols.outer_ns, {symbols.name, symbols.count, symbols.inner}).GetKey(), symbols.inner);
    EXPECT(reader.HasMore(), true);
    reader.NextToken(symbols.inner_ns, TokenType::BEGIN_STRUCT);

    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextKey(
        symbols.inner_ns, 
        {symbols.experimental, symbols.collection, symbols.real1, symbols.real2}, 
        TokenType::END_STRUCT).GetKey(), symbols.experimental);
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextEnum(
        symbols.inner_ns, 
        {symbols.yes, symbols.no}, 
        TokenType::END_STRUCT).GetEnum(), symbols.yes);

    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextKey(
        symbols.inner_ns, 
        {symbols.experimental, symbols.collection, symbols.real1, symbols.real2}, 
        TokenType::END_STRUCT).GetKey(), symbols.collection);
    EXPECT(reader.HasMore(), true);
    reader.NextToken(symbols.inner_ns, TokenType::BEGIN_SEQ);
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextToken(symbols.inner_ns, TokenType::INTEGER, TokenType::END_SEQ).GetInteger(), 100);
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextToken(symbols.inner_ns, TokenType::INTEGER, TokenType::END_SEQ).GetInteger(), 200);
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextToken(symbols.inner_ns, TokenType::INTEGER, TokenType::END_SEQ).type, TokenType::END_SEQ);
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextKey(
        symbols.inner_ns, 
        {symbols.experimental, symbols.collection, symbols.real1, symbols.real2}, 
        TokenType::END_STRUCT).GetKey(), symbols.real1);
    EXPECT(reader.HasMore(), true);
    EXPECT_WITHIN(reader.NextToken(symbols.inner_ns, TokenType::REAL).GetReal(), 0.0, 0.01);
    
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextKey(
        symbols.inner_ns, 
        {symbols.experimental, symbols.collection, symbols.real1, symbols.real2}, 
        TokenType::END_STRUCT).GetKey(), symbols.real2);
    EXPECT_WITHIN(reader.NextToken(symbols.inner_ns, TokenType::REAL).GetReal(), 7.8965e-9, 0.01);
    
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextKey(
        symbols.inner_ns, 
        {symbols.experimental, symbols.collection, symbols.real1, symbols.real2}, 
        TokenType::END_STRUCT).type, TokenType::END_STRUCT);
    EXPECT(reader.HasMore(), false);
    return 0;
}

int RunTokensReadString(const SymbolHolder &symbols, int argc, const char *argv[]) { 
    BEGIN_TEST("read_string");

    const char* const input = R"(name: [
        "he said, \"Hello World!\"",
        "\"I would love for
this to go on
forever\""])";
    std::istringstream in(input);
    TextTokenReader reader(symbols.symbol_map, in);
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextKey(symbols.outer_ns, {symbols.name, symbols.count, symbols.inner}).GetKey(), symbols.name);
    EXPECT(reader.HasMore(), true);
    reader.NextToken(symbols.inner_ns, TokenType::BEGIN_SEQ);
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextToken(symbols.inner_ns, TokenType::STRING, TokenType::END_SEQ).ConsumeString(), "he said, \"Hello World!\"");
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextToken(symbols.inner_ns, TokenType::STRING, TokenType::END_SEQ).ConsumeString(),
        "\"I would love for\nthis to go on\nforever\"");
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextToken(symbols.inner_ns, TokenType::STRING, TokenType::END_SEQ).type, TokenType::END_SEQ);
    
    EXPECT(reader.HasMore(), false);
    return 0;
}

int RunTokensReadEmpty(const SymbolHolder &symbols, int argc, const char *argv[]) {
    BEGIN_TEST("read_empty");

    const char* const input = R"(name:{} inner:[])";
    std::istringstream in(input);
    TextTokenReader reader(symbols.symbol_map, in);

    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextKey(symbols.outer_ns, {symbols.name, symbols.count, symbols.inner}).GetKey(), symbols.name);
    EXPECT(reader.HasMore(), true);
    reader.NextToken(symbols.inner_ns, TokenType::BEGIN_STRUCT);
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextToken(symbols.outer_ns, TokenType::STRING, TokenType::END_STRUCT).type, TokenType::END_STRUCT);
    
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextKey(symbols.outer_ns, {symbols.name, symbols.count, symbols.inner}).GetKey(), symbols.inner);
    EXPECT(reader.HasMore(), true);
    reader.NextToken(symbols.inner_ns, TokenType::BEGIN_SEQ);
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextToken(symbols.outer_ns, TokenType::STRING, TokenType::END_SEQ).type, TokenType::END_SEQ);

    EXPECT(reader.HasMore(), false);
    return 0;
}

int RunTokensReadMissingSymbol(const SymbolHolder &symbols, int argc, const char *argv[]) { 
    BEGIN_TEST("read_missing_symbol");
    bool caught_exception = false;

    {
         const char* const input = R"(experimental:true)";
        std::istringstream in(input);
        TextTokenReader reader(symbols.symbol_map, in);

        EXPECT(reader.HasMore(), true);
        try {
            reader.NextKey(symbols.inner_ns, {symbols.real2, symbols.real1});
        } catch (const SystemError&) {
            caught_exception = true;
        }
    }

    EXPECT(caught_exception, true);
    caught_exception = false;

     {
        const char* const input = R"(experimental:yes)";
        std::istringstream in(input);
        TextTokenReader reader(symbols.symbol_map, in);

        EXPECT(reader.HasMore(), true);
        EXPECT(reader.NextKey(symbols.inner_ns, {symbols.real2, symbols.real1, symbols.experimental}).GetKey(), symbols.experimental);

        EXPECT(reader.HasMore(), true);
        try {
            reader.NextEnum(symbols.inner_ns, {symbols.yes, symbols.no});
        } catch (const SystemError&) {
            caught_exception = true;
        }
     }

    EXPECT(caught_exception, true);
    return 0;
}

int RunTokensReadDouble(const SymbolHolder &symbols, int argc, const char *argv[]) {
    BEGIN_TEST("read_double");

    const char* const input = R"(count: 3 inner:56.78.9)";
    std::istringstream in(input);
    TextTokenReader reader(symbols.symbol_map, in);

    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextKey(symbols.outer_ns, {symbols.count}).GetKey(), symbols.count);
    EXPECT(reader.HasMore(), true);
    EXPECT_WITHIN(reader.NextToken(symbols.inner_ns, TokenType::REAL).GetReal(), 3.0, 0.01);
    EXPECT(reader.HasMore(), true);
    EXPECT(reader.NextKey(symbols.outer_ns, {symbols.inner}).GetKey(), symbols.inner);
    EXPECT(reader.HasMore(), true);

    try {
        reader.NextToken(symbols.inner_ns, TokenType::REAL).GetReal();
    } catch (const SystemError& e) {
        // Expected outcome.
        std::cerr << MakeDebugable(e) << "\n";
        return 0;
    }

    EXPECT(std::string("Fail on invalid double"), "succeeded");
    return 1;
}

int RunTokensReadWrongType(const SymbolHolder &symbols, int argc, const char *argv[]) { 
    BEGIN_TEST("read_wrong_type");
    bool caught_exception = false;

    {
        const char* const input = R"(experimental::yes)";
        std::istringstream in(input);
        TextTokenReader reader(symbols.symbol_map, in);

        EXPECT(reader.HasMore(), true);
        EXPECT(reader.NextKey(symbols.inner_ns, {symbols.experimental}).type, TokenType::KEY);
        EXPECT(reader.HasMore(), true);
        try {
            reader.NextEnum(symbols.inner_ns, {symbols.yes});
        } catch (const SystemError& e) {
            std::cerr << MakeDebugable(e) << "\n";
            caught_exception = true;
        }
    }

    EXPECT(caught_exception, true);
    caught_exception = false;

     {
        const char* const input = R"({)";
        std::istringstream in(input);
        TextTokenReader reader(symbols.symbol_map, in);

        EXPECT(reader.HasMore(), true);
        try {
            reader.NextKey(symbols.inner_ns, {symbols.experimental}, TokenType::BEGIN_STRUCT);
        } catch (const SystemError& e) {
            std::cerr << MakeDebugable(e) << "\n";
            caught_exception = true;
        }
     }

    EXPECT(caught_exception, true);
    return 0;
}

int RunTokensBinaryTinyInt(const SymbolHolder &symbols, int argc, const char *argv[]) { 
    BEGIN_TEST("binary_tiny_int");
    std::stringstream buffer;
    ByteTokenWriter out(buffer);

    out.Write(Token::Key(symbols.outer_ns, symbols.count));
    out.Write(Token::Integer(2));

    out.Complete();
    buffer.seekg(0);
    ByteTokenReader in(symbols.symbol_map, buffer);

    EXPECT(in.HasMore(), true);
    EXPECT(in.NextKey(symbols.outer_ns, {symbols.count}).type, TokenType::KEY);
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextToken(symbols.outer_ns, TokenType::INTEGER).GetInteger(), 2);
    EXPECT(in.HasMore(), false);
    return 0;
}

int RunTokensBinaryShortInt(const SymbolHolder &symbols, int argc, const char *argv[]) { 
    BEGIN_TEST("binary_short_int");

    std::stringstream buffer;
    ByteTokenWriter out(buffer);

    out.Write(Token::Key(symbols.outer_ns, symbols.count));
    out.Write(Token::Integer(-2));

    out.Complete();
    buffer.seekg(0);
    ByteTokenReader in(symbols.symbol_map, buffer);

    EXPECT(in.HasMore(), true);
    EXPECT(in.NextKey(symbols.outer_ns, {symbols.count}).type, TokenType::KEY);
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextToken(symbols.outer_ns, TokenType::INTEGER).GetInteger(), -2);
    EXPECT(in.HasMore(), false);

    return 0;
}

int RunTokensBinaryLongInt(const SymbolHolder &symbols, int argc, const char *argv[]) { 
    BEGIN_TEST("binary_long_int");
    std::stringstream buffer;
    ByteTokenWriter out(buffer);

    out.Write(Token::Key(symbols.outer_ns, symbols.count));
    out.Write(Token::Integer(0x8FFF)); // overflows 16 bits, but not 32.

    out.Complete();
    EXPECT(buffer.str().size() > 5, true);
    buffer.seekg(0);
    ByteTokenReader in(symbols.symbol_map, buffer);

    EXPECT(in.HasMore(), true);
    EXPECT(in.NextKey(symbols.outer_ns, {symbols.count}).type, TokenType::KEY);
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextToken(symbols.outer_ns, TokenType::INTEGER).GetInteger(), 0x8fff);
    EXPECT(in.HasMore(), false);
    return 0;
}

int RunTokensBinaryDouble(const SymbolHolder &symbols, int argc, const char *argv[]) { 
    BEGIN_TEST("binary_double");
    std::stringstream buffer;
    ByteTokenWriter out(buffer);

    out.Write(Token::Key(symbols.inner_ns, symbols.real1));
    out.Write(Token::Real(0.012));
    out.Write(Token::Key(symbols.inner_ns, symbols.real2));
    out.Write(Token::Real(-1.012e-12));

    out.Complete();
    buffer.seekg(0);
    ByteTokenReader in(symbols.symbol_map, buffer);

    EXPECT(in.HasMore(), true);
    EXPECT(in.NextKey(symbols.inner_ns, {symbols.real1}).type, TokenType::KEY);
    EXPECT(in.HasMore(), true);
    EXPECT_WITHIN(in.NextToken(symbols.inner_ns, TokenType::REAL).GetReal(), 0.012, 0.001);
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextKey(symbols.inner_ns, {symbols.real2}).type, TokenType::KEY);
    EXPECT(in.HasMore(), true);
    EXPECT_WITHIN(in.NextToken(symbols.inner_ns, TokenType::REAL).GetReal(), -1.012e-12, 0.001);
    EXPECT(in.HasMore(), false);
    return 0;
}

int RunTokensBinaryString(const SymbolHolder &symbols, int argc, const char *argv[]) { 
    BEGIN_TEST("binary_string");
    std::stringstream buffer;
    ByteTokenWriter out(buffer);

    out.Write(Token::Key(symbols.outer_ns, symbols.name));
    out.Write(Token::String("\"Hello \\World!\"")); // overflows 16 bits, but not 32.

    out.Complete();
    buffer.seekg(0);
    ByteTokenReader in(symbols.symbol_map, buffer);

    EXPECT(in.HasMore(), true);
    EXPECT(in.NextKey(symbols.outer_ns, {symbols.name}).type, TokenType::KEY);
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextToken(symbols.outer_ns, TokenType::STRING).ConsumeString(), "\"Hello \\World!\"");
    EXPECT(in.HasMore(), false);
    return 0;
}

int RunTokensBinaryEnum(const SymbolHolder &symbols, int argc, const char *argv[]) { 
    BEGIN_TEST("binary_enum");
    std::stringstream buffer;
    ByteTokenWriter out(buffer);

    out.Write(Token::Key(symbols.inner_ns, symbols.experimental));
    out.Write(Token::Enum(symbols.inner_ns, symbols.no));

    out.Complete();
    buffer.seekg(0);
    ByteTokenReader in(symbols.symbol_map, buffer);

    EXPECT(in.HasMore(), true);
    EXPECT(in.NextKey(symbols.inner_ns, {symbols.experimental}).type, TokenType::KEY);
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextEnum(symbols.inner_ns, {symbols.no}).type, TokenType::ENUM);
    EXPECT(in.HasMore(), false);
    return 0;
}

int RunTokensBinarySequence(const SymbolHolder &symbols, int argc, const char *argv[]) { 
    BEGIN_TEST("binary_sequence");
    std::stringstream buffer;
    ByteTokenWriter out(buffer);

    out.Write(Token::Key(symbols.inner_ns, symbols.collection));
    out.Write(Token::BeginSeqWithSize(2));
    out.Write(Token::String("first"));
    out.Write(Token::String("second"));
    out.Write(Token::EndSeq());

    out.Complete();
    buffer.seekg(0);
    ByteTokenReader in(symbols.symbol_map, buffer);

    EXPECT(in.HasMore(), true);
    EXPECT(in.NextKey(symbols.inner_ns, {symbols.collection}).type, TokenType::KEY);
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextToken(symbols.inner_ns, TokenType::BEGIN_SEQ).GetCount().value(), 2);
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextToken(symbols.inner_ns, TokenType::STRING).ConsumeString(), "first");
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextToken(symbols.inner_ns, TokenType::STRING).ConsumeString(), "second");
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextToken(symbols.inner_ns, TokenType::END_SEQ).type, TokenType::END_SEQ);
    EXPECT(in.HasMore(), false);
    return 0;
}

int RunTokensBinarySequenceNoSize(const SymbolHolder &symbols, int argc, const char *argv[]) { 
    BEGIN_TEST("binary_sequence_no_size");
    std::stringstream buffer;
    ByteTokenWriter out(buffer);

    out.Write(Token::Key(symbols.inner_ns, symbols.collection));
    out.Write(Token::BeginSeq());
    out.Write(Token::String("first"));
    out.Write(Token::String("second"));
    out.Write(Token::EndSeq());

    out.Complete();
    buffer.seekg(0);
    ByteTokenReader in(symbols.symbol_map, buffer);

    EXPECT(in.HasMore(), true);
    EXPECT(in.NextKey(symbols.inner_ns, {symbols.collection}).type, TokenType::KEY);
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextToken(symbols.inner_ns, TokenType::BEGIN_SEQ).GetCount().has_value(), false);
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextToken(symbols.inner_ns, TokenType::STRING).ConsumeString(), "first");
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextToken(symbols.inner_ns, TokenType::STRING).ConsumeString(), "second");
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextToken(symbols.inner_ns, TokenType::END_SEQ).type, TokenType::END_SEQ);
    EXPECT(in.HasMore(), false);
    return 0;
}

int RunTokensBinaryStruct(const SymbolHolder &symbols, int argc, const char *argv[]) { 
    BEGIN_TEST("binary_struct");
    std::stringstream buffer;
    ByteTokenWriter out(buffer);

    out.Write(Token::Key(symbols.outer_ns, symbols.inner));
    out.Write(Token::BeginStruct());
    out.Write(Token::Key(symbols.inner_ns, symbols.experimental));
    out.Write(Token::Enum(symbols.inner_ns, symbols.yes));
    out.Write(Token::EndStruct());

    out.Complete();
    buffer.seekg(0);
    ByteTokenReader in(symbols.symbol_map, buffer);

    EXPECT(in.HasMore(), true);
    EXPECT(in.NextKey(symbols.outer_ns, {symbols.inner}).type, TokenType::KEY);
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextToken(symbols.outer_ns, TokenType::BEGIN_STRUCT).type, TokenType::BEGIN_STRUCT);
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextKey(symbols.inner_ns, {symbols.experimental}).type, TokenType::KEY);
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextEnum(symbols.inner_ns, {symbols.yes}).type, TokenType::ENUM);
    EXPECT(in.HasMore(), true);
    EXPECT(in.NextToken(symbols.outer_ns, TokenType::END_STRUCT).type, TokenType::END_STRUCT);
    EXPECT(in.HasMore(), false);
    return 0;
}

} // namespace

int RunTokensTests(int argc, const char *argv[]) {
    BEGIN_SUITE("tokens");

    int result = 0;

    try {
        SymbolHolder symbols;
        symbols.PrepareSymbols();

        // text writer
        result = RunTokensTextWriter(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensEmptyCollection(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensBadStruct(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensBadSeq(symbols, argc, argv) == 0 ? result : 1;
        
        // text reader
        result = RunTokensRead(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensReadString(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensReadEmpty(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensReadMissingSymbol(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensReadDouble(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensReadWrongType(symbols, argc, argv) == 0 ? result : 1;

        // binary representation
        result = RunTokensBinaryTinyInt(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensBinaryShortInt(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensBinaryLongInt(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensBinaryDouble(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensBinaryString(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensBinaryEnum(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensBinarySequence(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensBinarySequenceNoSize(symbols, argc, argv) == 0 ? result : 1;
        result = RunTokensBinaryStruct(symbols, argc, argv) == 0 ? result : 1;

    } catch(const SystemError& e) {
        std::cerr << MakeDebugable(e) << "\n";
        return 1;
    }

    return result;
}