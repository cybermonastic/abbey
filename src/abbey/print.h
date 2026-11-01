#pragma once

#include <ostream>
#include <string>
#include <vector>
#include <memory>

#include "abbey/debug.h"

namespace abbey {

// Adds user messages loaded from file.
// The first line of the file should be of the form:
// lang_COUNTRY.codepage
// e.g. en_US.UTF-8
//
// Every subsequent line should be of the form:
// <message_id.N> <whitespace> <message>
//
// <message> may include {n} arguments, where n is an integer, 0 <= n < N.
// {n} arguments do not need to appear in sequence, but all values
// from 0 to N-1 should be present.
//
// NOT THREADSAFE 
void LoadUserMessages(std::string_view filename, bool ignore_failures);

struct Locale {
    std::string id;

    static const Locale& SYSTEM(); 
};

// Returns true if all messages currently loaded are valid and argument
// counts are consistent with one another.
// Otherwise returns false, and prints discrepancies to "out".
// All locales are compared against the 'reference' locale, which
// should have all desired strings loaded.
bool ValidateUserMessages(const Locale& reference, std::ostream& out);

// Wraps all of the information needed to write a localized string,
// owning all data upon which it depends.
class Localizable {
public:
    virtual ~Localizable() = default;

    virtual std::ostream& Localize(const Locale& locale, std::ostream& out) const = 0;
    virtual std::unique_ptr<Localizable> Clone() const = 0;
};

// Wraps all information needed to write a localized string
// into a self-contained Localizable which owns all of the
// data required to produce that string.
class Printable {
public:
    virtual ~Printable() = default;

    virtual std::unique_ptr<Localizable> Print() const = 0;
};

// Canonical type for holding message intended for
// end-user consumption. Typical actual return type
// for implementations of Printable.
class LocalizableMessage : public Localizable {
private:
    std::string message_id;
    std::vector<std::unique_ptr<Localizable>> args;

    std::ostream& LocalizeWithoutMessageString(const Locale& locale, std::ostream& out) const;
public:
    LocalizableMessage(std::string_view message_id) : message_id(message_id) {}
    ~LocalizableMessage() = default;

    LocalizableMessage& WithArg(const Printable& arg);
    LocalizableMessage& WithInt(long value);
    // 'precision' is number of digits after the decimal point.
    // 'scientific' enables shifting the decimal point through scientific notation.
    LocalizableMessage& WithReal(double value, int precision, bool scientific);
    LocalizableMessage& WithSystemString(std::string_view str);

    std::ostream& Localize(const Locale& locale, std::ostream& out) const override;
    std::unique_ptr<Localizable> Clone() const override;
};

class Localizable_Printable : public Printable {
private:
    const Localizable& self;
public:
    Localizable_Printable(const Localizable& self): self(self) {}

    std::unique_ptr<Localizable> Print() const override {
        return self.Clone();
    }
};
inline Localizable_Printable MakePrintable(const Localizable& m) { return Localizable_Printable(m); }

// Given a Localizable instance, produce debug output by Localizing it to the SYSTEM() locale.
class Localizable_Debugable : public Debugable {
private:
    const Localizable& self;
public:
    Localizable_Debugable(const Localizable& self): self(self) {}

    std::ostream& Debug(std::string_view indent, std::string_view, std::ostream& out) const override {
        out << indent;
        return self.Localize(Locale::SYSTEM(), out);
    }
};
inline Localizable_Debugable MakeDebugable(const Localizable& m) { return Localizable_Debugable(m); } 

}