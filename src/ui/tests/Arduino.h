#pragma once
// Small host adapter for the String operations used by journal_ui.cpp. Hardware
// builds use the real Arduino String. Network/task behavior is mocked separately.
#include <string>
#include <cstdio>
#include <cstdint>
#include <cmath>
class String : public std::string {
    // Arduino String is not an STL iterator container.
    using const_iterator = std::string::const_iterator;
public:
    String() = default;
    String(const char *s) : std::string(s ? s : "") {}
    String(const char *s, size_t n) : std::string(s, n) {}
    String(const std::string &s) : std::string(s) {}
    String(double value, unsigned digits) {
        char text[80]; std::snprintf(text, sizeof(text), "%.*f", digits, value); assign(text);
    }
    String &operator=(const char *s) { assign(s ? s : ""); return *this; }
    bool concat(const char *s) { append(s); return true; }
    void replace(const char *from, const char *to) {
        size_t pos = 0;
        while ((pos = find(from, pos)) != npos) {
            std::string::replace(pos, std::char_traits<char>::length(from), to);
            pos += std::char_traits<char>::length(to);
        }
    }
};
unsigned long millis();
