#pragma once

#include <ostream>
#include <string_view>

namespace abbey {

class Debugable {
public:
    virtual ~Debugable() = default;

    // All lines after the first must write 'prefix' before anything else.
    // If additional nesting is desired, prepend 'indent' to 'prefix'.
    virtual std::ostream& Debug(std::string_view prefix, std::string_view indent, std::ostream& out) const = 0;
};

class OwnedDebugable : public Debugable {
public:
    virtual ~OwnedDebugable() = default;

    virtual std::unique_ptr<OwnedDebugable> Clone() const = 0;

};

inline std::ostream& operator<<(std::ostream& out, const Debugable& Debug) {
    return Debug.Debug("  ", "", out);
}

}
