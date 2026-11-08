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
    } catch(const SystemError& e) {
        std::cerr << MakeDebugable(e) << "\n";
        return 1;
    }

    return result;
}