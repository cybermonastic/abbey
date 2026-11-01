#pragma once

#include <ostream>
#include <string>
#include <string_view>
#include <memory>

#include "abbey/debug.h"

namespace abbey {
   
struct Exception {
    Exception(std::string_view message) : message(message) {}
    Exception(std::string_view message, std::unique_ptr<Debugable> cause) : message(message), cause(std::move(cause)) {}
    std::string message;
    std::unique_ptr<Debugable> cause;
};

class Exception_Debugable : public Debugable {
private:
    const Exception& self;
public:
    Exception_Debugable(const Exception& self): self(self) {}

    std::ostream& Debug(std::string_view indent, std::string_view prefix, std::ostream& out) const override {
        out << prefix << "Exception: \"" << self.message << "\"";
        if (self.cause) {
            out << " caused by\n";
            return self.cause->Debug(indent, prefix, out);
        }
        return out << "\n";
    }
};
inline Exception_Debugable MakeDebugable(const Exception& e) { return Exception_Debugable(e); }  
}