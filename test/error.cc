#include "error.h"

#include <iostream>
#include <sstream>

#include "test.h"
#include "abbey\error.h"
#include "abbey\print.h"

#define GET_FILE_STRING() __FILE__

// Disable unreachable code warning.
#pragma warning(disable : 4702)

namespace {

using abbey::MakeDebugable;
using abbey::MakePrintable;
using abbey::Locale;
using abbey::LocalizableMessage;
using abbey::UserError;

int RunErrorSystem(int argc, const char *argv[]) {
    BEGIN_TEST("system");

    try {
        THROW_SYSTEM_ERROR("Test failure");
    } catch (const abbey::SystemError &error) {
        std::stringstream debug;
        MakeDebugable(error).Debug("----", "", debug);
        std::stringstream expected;
        expected << "SystemError: \"" << GET_FILE_STRING() << ":27: Test failure\"";
        EXPECT(debug.str(), expected.str());

        std::stringstream printed;
        MakePrintable(error).Print()->Localize(Locale::SYSTEM(), printed);
        EXPECT(printed.str(), "\"abbey.system_error.0\"");
        return 0;
    }
    std::cout << "Error not caught.";
    return 1;
}

int RunErrorSystemWithCause(int argc, const char *argv[]) {
    BEGIN_TEST("system_with_cause");

    try {
        try {
            THROW_SYSTEM_ERROR("Inner test failure");
        } catch (const abbey::SystemError& e) {
            THROW_SYSTEM_ERROR_BECAUSE("Outer test failure", e);
        }
    } catch (const abbey::SystemError &error) {
        std::stringstream debug;
        MakeDebugable(error).Debug("----", "", debug);
        std::stringstream expected;
        expected << "SystemError: \"" << GET_FILE_STRING() << ":51: Outer test failure\" caused by\n----"
            << "SystemError: \"" << GET_FILE_STRING() << ":49: Inner test failure\"";
        EXPECT(debug.str(), expected.str());

        std::stringstream printed;
        MakePrintable(error).Print()->Localize(Locale::SYSTEM(), printed);
        EXPECT(printed.str(), "\"abbey.system_error.0\"");
        return 0;
    }
    std::cout << "Error not caught.";
    return 1;
}

int RunErrorUser(int argc, const char *argv[]) {
    BEGIN_TEST("user");

    try {
        throw UserError(LocalizableMessage("test.error").WithInt(15));
    } catch (const UserError &error) {
        std::stringstream debug;
        MakeDebugable(error).Debug("----", "", debug);
        std::stringstream expected;
        expected << "UserError: \"test.error.1\" {15}";
        EXPECT(debug.str(), expected.str());

        std::stringstream printed;
        MakePrintable(error).Print()->Localize(Locale::SYSTEM(), printed);
        EXPECT(printed.str(), "\"test.error.1\" {15}");
        return 0;
    }
    std::cout << "Error not caught.";
    return 1;
}

int RunErrorUserWithCause(int argc, const char *argv[]) {
    BEGIN_TEST("user_with_cause");

    try {
        try {
            throw UserError(LocalizableMessage("test.error").WithInt(15));
        } catch (const UserError& e) {
            throw UserError(LocalizableMessage("test.error"), abbey::MakeOwnedDebugable(e));
        }
    } catch (const UserError& error) {
        std::stringstream debug;
        MakeDebugable(error).Debug("----", "", debug);
        std::stringstream expected;
        expected << "UserError: \"test.error.0\" caused by\n"
                << "----UserError: \"test.error.1\" {15}";
        EXPECT(debug.str(), expected.str());

        std::stringstream printed;
        MakePrintable(error).Print()->Localize(Locale::SYSTEM(), printed);
        EXPECT(printed.str(), "\"test.error.0\"");
        return 0;
    }
    std::cout << "Error not caught.";
    return 1;
}

} // namespace

int RunErrorTests(int argc, const char *argv[]) {
    BEGIN_SUITE("error");

    int result = 0;

    result = RunErrorSystem(argc, argv) == 0 ? result : 1;
    result = RunErrorSystemWithCause(argc, argv) == 0 ? result : 1;
    result = RunErrorUser(argc, argv) == 0 ? result : 1;
    result = RunErrorUserWithCause(argc, argv) == 0 ? result : 1;

    return result;
}