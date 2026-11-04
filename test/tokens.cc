#include "tokens.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include "test.h"
#include "abbey\tokens.h"

namespace {

using abbey::Token;
using abbey::TokenType;
using abbey::SymbolMap;
using abbey::TextTokenWriter;


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
    writer.Write(Token::SepSeq());
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
    writer.Write(Token::SepSeq());
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


} // namespace

int RunTokensTests(int argc, const char *argv[]) {
    BEGIN_SUITE("tokens");

    int result = 0;

    SymbolHolder symbols;
    symbols.PrepareSymbols();
    result = RunTokensTextWriter(symbols, argc, argv) == 0 ? result : 1;
    result = RunTokensEmptyCollection(symbols, argc, argv) == 0 ? result : 1;

    return result;
}