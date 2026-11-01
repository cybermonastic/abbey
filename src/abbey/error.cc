#include "abbey/error.h"

namespace abbey {

UserError::UserError(const UserError& e) 
: message(e.message->Clone())
, cause(e.cause ? e.cause->Clone() : nullptr)
{}

std::unique_ptr<UserError> UserError::Clone() const {
    return std::make_unique<UserError>(*this);
}

std::ostream& UserError_OwnedDebugable::Debug(std::string_view indent, std::string_view prefix, std::ostream& out) const {
    return MakeDebugable(*self).Debug(indent, prefix, out);
} 

std::ostream& UserError_Debugable::Debug(std::string_view indent, std::string_view prefix, std::ostream& out) const {
    out << "UserError: ";
    MakeDebugable(*self.message).Debug(indent, prefix, out);
    if (self.cause) {
        out << " caused by\n" << indent;
        return self.cause->Debug(indent, prefix, out);
    }
    return out;
} 

std::unique_ptr<Localizable> UserError_Printable::Print() const {
    return self.message->Clone();
}

SystemError::SystemError(const SystemError& e) 
: message(e.message)
, cause(e.cause ? e.cause->Clone() : nullptr)
{}

std::unique_ptr<SystemError> SystemError::Clone() const {
    return std::make_unique<SystemError>(*this);
}

std::ostream& SystemError_OwnedDebugable::Debug(std::string_view indent, std::string_view prefix, std::ostream& out) const {
    return MakeDebugable(*self).Debug(indent, prefix, out);
} 

std::ostream& SystemError_Debugable::Debug(std::string_view indent, std::string_view prefix, std::ostream& out) const {
    out << "SystemError: \"" << self.message << "\"";
    if (self.cause) {
        out << " caused by\n" << indent;
        return self.cause->Debug(indent, prefix, out);
    }
    return out;
}

// Don't leak debug information to the end user.
std::unique_ptr<Localizable> SystemError_Printable::Print() const {
    return std::make_unique<LocalizableMessage>("abbey.system_error");
}

} // abbey