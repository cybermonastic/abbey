#pragma once

#include <ostream>
#include <string>
#include <string_view>
#include <memory>

#include "abbey/debug.h"
#include "abbey/print.h"

#define LINE_AS_STRING(n) #n
#define SYSTEM_ERROR_MESSAGE(file, line, msg) std::string(file ":" LINE_AS_STRING(line) ": ") + msg
#define CREATE_SYSTEM_ERROR(msg) abbey::SystemError( SYSTEM_ERROR_MESSAGE(__FILE__, __LINE__, msg) )
#define CREATE_SYSTEM_ERROR_BECAUSE(msg, cause) \
        abbey::SystemError( SYSTEM_ERROR_MESSAGE(__FILE__, __LINE__, msg), abbey::MakeOwnedDebugable(cause) );
#define THROW_SYSTEM_ERROR(msg) throw CREATE_SYSTEM_ERROR(msg);
#define THROW_SYSTEM_ERROR_BECAUSE(msg, cause) throw CREATE_SYSTEM_ERROR_BECAUSE(msg, cause);

/* Sample Usage:
// try {
//    try {
//        THROW_SYSTEM_ERROR("Inner test failure");
//    } catch (const abbey::SystemError& e) {
//        THROW_SYSTEM_ERROR_BECAUSE("Outer test failure", e);
//    }
// } catch (const abbey::SystemError &error) {
//    std::cerr << MakeDebugable(error) << "\n";
//    return 1;
// }
// try {
//    try {
//        throw UserError(LocalizableMessage("test.error").WithInt(15));
//    } catch (const UserError& e) {
//        throw UserError(LocalizableMessage("test.error"), abbey::MakeOwnedDebugable(e));
//    }
// } catch (const UserError& error) {
//    MakePrintable(error).Print()->Localize(Locale::SYSTEM(), std::cerr);
//    return 1;
// }
*/

namespace abbey {

// For errors intending to be seen by users.
struct UserError {
    // What the user should see.
    std::unique_ptr<Localizable> message;
    // Collected for debugging purposes only.
    std::unique_ptr<OwnedDebugable> cause;

    UserError(UserError&&) = default;
    UserError(const UserError& e);
    UserError(const Localizable& message) : message(message.Clone()) {}
    UserError(const Localizable& message, std::unique_ptr<OwnedDebugable> cause) 
        : message(MakePrintable(message).Print()), cause(std::move(cause)) {}
    ~UserError() = default;

    std::unique_ptr<UserError> Clone() const;
};

class UserError_OwnedDebugable : public OwnedDebugable {
private:
    std::unique_ptr<UserError> self;
public:
    UserError_OwnedDebugable(const UserError& self) : self(self.Clone()) {}

    std::ostream& Debug(std::string_view indent, std::string_view prefix, std::ostream& out) const override;
    std::unique_ptr<OwnedDebugable> Clone() const override {
        return std::make_unique<UserError_OwnedDebugable>(*self);
    }
};
inline std::unique_ptr<UserError_OwnedDebugable> MakeOwnedDebugable(const UserError& e) {
    return std::make_unique<UserError_OwnedDebugable>(std::move(e));
}  

class UserError_Debugable : public Debugable {
private:
    const UserError& self;
public:
    UserError_Debugable(const UserError& self) : self(self) {}

    std::ostream& Debug(std::string_view indent, std::string_view prefix, std::ostream& out) const override;
};
inline UserError_Debugable MakeDebugable(const UserError& e) {
    return UserError_Debugable(e);
}

class UserError_Printable : public Printable {
private:
    const UserError& self;
public:
    UserError_Printable(const UserError& self) : self(self) {}

    // Don't leak debug information to the end user.
    std::unique_ptr<Localizable> Print() const override;
};
inline UserError_Printable MakePrintable(const UserError& e) { 
    return UserError_Printable(e);
}

// For errors that are intended to produce debug information,
// not for end-user consumption.
struct SystemError {
    std::string message;
    std::unique_ptr<OwnedDebugable> cause;

    SystemError(SystemError&&) = default;
    SystemError(const SystemError& e);
    SystemError(std::string_view message) : message(message) {}
    SystemError(std::string_view message, std::unique_ptr<OwnedDebugable> cause) : message(message), cause(std::move(cause)) {}
    ~SystemError() = default;

    std::unique_ptr<SystemError> Clone() const;
};

class SystemError_OwnedDebugable : public OwnedDebugable {
private:
    std::unique_ptr<SystemError> self;
public:
    SystemError_OwnedDebugable(const SystemError& self) : self(self.Clone()) {}

    std::ostream& Debug(std::string_view indent, std::string_view prefix, std::ostream& out) const override;
    std::unique_ptr<OwnedDebugable> Clone() const override {
        return std::make_unique<SystemError_OwnedDebugable>(*self);
    }
};
inline std::unique_ptr<SystemError_OwnedDebugable> MakeOwnedDebugable(const SystemError& e) {
    return std::make_unique<SystemError_OwnedDebugable>(e);
}  

class SystemError_Debugable : public Debugable {
private:
    const SystemError& self;
public:
    SystemError_Debugable(const SystemError& self) : self(self) {}

    std::ostream& Debug(std::string_view indent, std::string_view prefix, std::ostream& out) const override;
};
inline SystemError_Debugable MakeDebugable(const SystemError& e) {
    return SystemError_Debugable(e);
}

class SystemError_Printable : public Printable {
private:
    const SystemError& self;
public:
    SystemError_Printable(const SystemError& self) : self(self) {}

    // Don't leak debug information to the end user.
    std::unique_ptr<Localizable> Print() const override;
};
inline SystemError_Printable MakePrintable(const SystemError& e) { 
    return SystemError_Printable(e); 
}

} // abbey