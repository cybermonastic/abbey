#include "print.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include "test.h"
#include "abbey\print.h"

namespace {

using abbey::LocalizableMessage;
using abbey::Locale;

int RunPrintSystem0(int argc, const char *argv[]) {
    BEGIN_TEST("system0");

    std::stringstream buf;
    LocalizableMessage("test.message_id").Localize(Locale::SYSTEM(), buf);
    EXPECT(buf.str(), "\"test.message_id.0\"");
    return 0;
}

int RunPrintSystem2(int argc, const char *argv[]) {
    BEGIN_TEST("system2");

    std::stringstream buf;
    LocalizableMessage("test.message_id")
        .WithInt(10).WithInt(20)
        .Localize(Locale::SYSTEM(), buf);
    EXPECT(buf.str(), "\"test.message_id.2\" {10} {20}");
    return 0;
}

int RunPrintSystemInt(int argc, const char *argv[]) {
    BEGIN_TEST("system_int");

    std::stringstream buf;
    LocalizableMessage("test.message_id")
        .WithInt(10)
        .Localize(Locale::SYSTEM(), buf);
    EXPECT(buf.str(), "\"test.message_id.1\" {10}");
    return 0;
}

int RunPrintSystemReal(int argc, const char *argv[]) {
    BEGIN_TEST("system_real");

    std::stringstream buf;
    LocalizableMessage("test.message_id")
        .WithReal(10.4, 0, false)
        .Localize(Locale::SYSTEM(), buf);
    EXPECT(buf.str(), "\"test.message_id.1\" {10}");
    return 0;
}

int RunPrintSystemDecimal(int argc, const char *argv[]) {
    BEGIN_TEST("system_decimal");

    std::stringstream buf;
    LocalizableMessage("test.message_id")
        .WithReal(10.4, 2, false)
        .Localize(Locale::SYSTEM(), buf);
    EXPECT(buf.str(), "\"test.message_id.1\" {10.40}");
    return 0;
}

int RunPrintSystemScientific(int argc, const char *argv[]) {
    BEGIN_TEST("system_scientific");

    std::stringstream buf;
    LocalizableMessage("test.message_id")
        .WithReal(10.46, 2, true)
        .Localize(Locale::SYSTEM(), buf);

    // C++ implementations do not agree on how to write the exponent in scientific notation,
    // so check for whatever the current implementation uses.
    std::stringstream expected;
    expected << "\"test.message_id.1\" {" << std::scientific << std::setprecision(2) << 10.5 << "}";
    
    EXPECT(buf.str(), expected.str());
    return 0;
}

int RunPrintSystemString(int argc, const char *argv[]) {
    BEGIN_TEST("system_string");

    std::stringstream buf;
    LocalizableMessage("test.message_id")
        .WithSystemString("my_test_file.txt")
        .Localize(Locale::SYSTEM(), buf);
    EXPECT(buf.str(), "\"test.message_id.1\" {\"my_test_file.txt\"}");
    return 0;
}

int RunPrintSystemMessage(int argc, const char *argv[]) {
    BEGIN_TEST("system_message");

    std::stringstream buf;
    LocalizableMessage("test.message_id")
        .WithArg(abbey::MakePrintable(
            LocalizableMessage("inner_message_id")))
        .Localize(Locale::SYSTEM(), buf);
    EXPECT(buf.str(), "\"test.message_id.1\" {\"inner_message_id.0\"}");
    return 0;
}

int RunPrintDebug(int argc, const char *argv[]) {
    BEGIN_TEST("debug");

    std::stringstream buf;
    MakeDebugable(
        LocalizableMessage("test.message_id")
            .WithInt(10))
        .Debug("----", "", buf);
    EXPECT(buf.str(), "----\"test.message_id.1\" {10}");
    return 0;
}

} // namespace

int RunPrintTests(int argc, const char *argv[]) {
    BEGIN_SUITE("print");

    int result = 0;

    result = RunPrintSystem0(argc, argv) == 0 ? result : 1;
    result = RunPrintSystem2(argc, argv) == 0 ? result : 1;
    result = RunPrintSystemInt(argc, argv) == 0 ? result : 1;
    result = RunPrintSystemReal(argc, argv) == 0 ? result : 1;
    result = RunPrintSystemDecimal(argc, argv) == 0 ? result : 1;
    result = RunPrintSystemScientific(argc, argv) == 0 ? result : 1;
    result = RunPrintSystemString(argc, argv) == 0 ? result : 1;
    result = RunPrintSystemMessage(argc, argv) == 0 ? result : 1;
    result = RunPrintDebug(argc, argv) == 0 ? result : 1;

    return result;
}