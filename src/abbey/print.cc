#include "abbey/print.h"

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <locale>
#include <iomanip>
#include <optional>

namespace abbey {

namespace {

struct Span {
    std::string_view text;
    std::optional<size_t> argument_id;
};

struct MessageInfo {
    std::string message; // original owned text.
    std::vector<Span> spans;
};

struct LocaleInfo {
    std::locale c_locale;
    // key is "<message_id>.<arg_count>".
    std::unordered_map<std::string, MessageInfo> messages;
};

struct LocalizationOwner {
    std::unordered_map<std::string, LocaleInfo> locale_infos;

    LocaleInfo &GetLocaleInfo(const Locale& locale) {
        auto itr = locale_infos.find(locale.id);
        if (itr != locale_infos.end()) {
            return itr->second;
        }
        if (locale.id == Locale::SYSTEM().id) {
            locale_infos[locale.id].c_locale = std::locale::classic();
        } else {
            locale_infos[locale.id].c_locale = std::locale(locale.id);
        }
        
        return locale_infos.find(locale.id)->second; 
    }
};

LocalizationOwner& GetMutableSingletonLocalizationOwner() {
    static LocalizationOwner singleton;
    return singleton;
}

const LocaleInfo& GetLocale(const Locale& locale) {
    return GetMutableSingletonLocalizationOwner().GetLocaleInfo(locale);
}

struct LocalizableInteger : public Localizable {
    long value;
    LocalizableInteger(long value) : value(value) {}

    std::ostream& Localize(const Locale&, std::ostream& out) const override {
        // Assume caller has set the locale.
        return out << value;
    }

    std::unique_ptr<Localizable> Clone() const override {
        return std::make_unique<LocalizableInteger>(value);
    }
};

struct LocalizableReal : public Localizable {
    double value;
    int precision;
    bool scientific;

    LocalizableReal(double value, int precision, bool scientific) : value(value), precision(precision), scientific(scientific) {}

    std::ostream& Localize(const Locale&, std::ostream& out) const override {
        // Assume caller has set the locale.
        if (scientific) {
            out << std::scientific << std::showpoint << std::setprecision(precision);
        } else if (precision > 0) {
            out << std::fixed << std::showpoint << std::setprecision(precision);
        } else {
            out << std::fixed << std::setprecision(precision);
        }
        return out << value;
    }

    std::unique_ptr<Localizable> Clone() const override {
        return std::make_unique<LocalizableReal>(value, precision, scientific);
    }
};

// Helper for ensuring locales are reset correctly.
struct SetLocale {
private:
    std::locale original;
    std::ostream& out;
public:
    SetLocale(std::ostream& out, std::locale locale) : original(out.imbue(locale)), out(out) {}
    ~SetLocale() { out.imbue(original); }
};

struct SystemString : public Localizable {
    std::string value;
    SystemString(std::string_view value) : value(value) {}

    std::ostream& Localize(const Locale&, std::ostream& out) const override {
        SetLocale set_locale(out, std::locale::classic());
        out << "\"" << value << "\"";
        return out;
    }
    std::unique_ptr<Localizable> Clone() const override {
        return std::make_unique<SystemString>(value);
    }
};

std::string GetMessageKey(std::string_view message_id, size_t arg_count) {
    std::string result(message_id);
    result.reserve(message_id.size() + 3);
    result +=  "." + std::to_string(arg_count);
    return result;
}

}  // namespace

const Locale& Locale::SYSTEM() {
    static Locale system("C");
    return system;
}
  
void LoadUserMessages(std::string_view filename, bool ignore_failures) {
    // TODO: implement.
    (void)filename;
    (void)ignore_failures;
}

bool ValidateUserMessages(const Locale& reference, std::ostream& out) {
    // TODO: implement.
    (void)reference;
    (void)out;
    return true;
}

LocalizableMessage& LocalizableMessage::WithArg(const Printable& arg) {
    args.push_back(arg.Print());
    return *this;
}

LocalizableMessage& LocalizableMessage::WithInt(long value) {
    args.push_back(std::make_unique<LocalizableInteger>(value));
    return *this;
}

LocalizableMessage& LocalizableMessage::WithReal(double value, int precision, bool scientific){
    args.push_back(std::make_unique<LocalizableReal>(value, precision, scientific));
    return *this;
}

LocalizableMessage& LocalizableMessage::WithSystemString(std::string_view str) {
    args.push_back(std::make_unique<SystemString>(str));
    return *this;
}

// Don't use a message string and append arguments at the end. Do localize the arguments.
std::ostream& LocalizableMessage::LocalizeWithoutMessageString(const Locale& locale, std::ostream& out) const {
    {
        SetLocale set_locale(out, std::locale::classic());
        out << "\"" << GetMessageKey(message_id, args.size()) << "\"";
    }
    for (const auto& arg : args) {
        out << " {";
        arg->Localize(locale, out);
        out << "}";
    }
    return out;
}

std::ostream& LocalizableMessage::Localize(const Locale& locale, std::ostream& out) const {
    const LocaleInfo& info = GetLocale(locale);
    SetLocale set_locale(out, info.c_locale);

    if (locale.id == Locale::SYSTEM().id) {
        return LocalizeWithoutMessageString(locale, out);
    }

    auto found = info.messages.find(GetMessageKey(message_id, args.size()));
    if (found == info.messages.end()) {
        return LocalizeWithoutMessageString(locale, out);
    }

    const MessageInfo& message_info = found->second;
    for (const auto& span : message_info.spans) {
        if (span.argument_id != std::nullopt && span.argument_id < args.size()) {
            args[span.argument_id.value()]->Localize(locale, out);
            continue;
        }
        out << span.text;
    }

    return out;
}

std::unique_ptr<Localizable> LocalizableMessage::Clone() const {
    auto clone = std::make_unique<LocalizableMessage>(message_id);
    clone->args.reserve(args.size());
    for (auto& arg : args) {
        clone->args.push_back(arg->Clone());
    }
    return clone;
}

} // namespace abbey