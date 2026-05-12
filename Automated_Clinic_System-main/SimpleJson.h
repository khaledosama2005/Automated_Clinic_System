#ifndef SIMPLEJSON_H
#define SIMPLEJSON_H

#include <cctype>
#include <map>
#include <string>
#include <variant>
#include <vector>

// Small JSON parser/writer for this project (objects/arrays/strings/numbers/bool/null).
// Not fully RFC-complete, but enough for our stdin command protocol + state.json.
namespace SimpleJson {

struct Value;
using Object = std::map<std::string, Value>;
using Array  = std::vector<Value>;

struct Value {
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;
    Storage v;

    Value() : v(nullptr) {}
    Value(std::nullptr_t) : v(nullptr) {}
    Value(bool b) : v(b) {}
    Value(int n) : v((double)n) {}
    Value(double n) : v(n) {}
    Value(const char* s) : v(std::string(s)) {}
    Value(std::string s) : v(std::move(s)) {}
    Value(Array a) : v(std::move(a)) {}
    Value(Object o) : v(std::move(o)) {}

    bool isNull()   const { return std::holds_alternative<std::nullptr_t>(v); }
    bool isBool()   const { return std::holds_alternative<bool>(v); }
    bool isNumber() const { return std::holds_alternative<double>(v); }
    bool isString() const { return std::holds_alternative<std::string>(v); }
    bool isArray()  const { return std::holds_alternative<Array>(v); }
    bool isObject() const { return std::holds_alternative<Object>(v); }

    const Object& asObject() const { return std::get<Object>(v); }
    const Array&  asArray()  const { return std::get<Array>(v); }
    const std::string& asString() const { return std::get<std::string>(v); }
    double asNumber(double def = 0.0) const { return isNumber() ? std::get<double>(v) : def; }
    bool asBool(bool def = false) const { return isBool() ? std::get<bool>(v) : def; }

    int asInt(int def = 0) const {
        if (!isNumber()) return def;
        return (int)std::get<double>(v);
    }

    // Safe field access helper
    const Value* get(const std::string& key) const {
        if (!isObject()) return nullptr;
        auto it = asObject().find(key);
        if (it == asObject().end()) return nullptr;
        return &it->second;
    }
};

struct ParseResult {
    bool ok;
    Value value;
    std::string error;
};

inline void skipWs(const std::string& s, size_t& i) {
    while (i < s.size() && std::isspace((unsigned char)s[i])) i++;
}

inline bool match(const std::string& s, size_t& i, const char* lit) {
    size_t j = i;
    while (*lit) {
        if (j >= s.size() || s[j] != *lit) return false;
        ++j; ++lit;
    }
    i = j;
    return true;
}

inline std::string parseStringRaw(const std::string& s, size_t& i, std::string& err) {
    if (i >= s.size() || s[i] != '"') { err = "Expected string"; return {}; }
    i++; // "
    std::string out;
    while (i < s.size()) {
        char c = s[i++];
        if (c == '"') return out;
        if (c == '\\') {
            if (i >= s.size()) { err = "Bad escape"; return {}; }
            char e = s[i++];
            switch (e) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                default: err = "Unsupported escape"; return {};
            }
        } else {
            out.push_back(c);
        }
    }
    err = "Unterminated string";
    return {};
}

inline Value parseValue(const std::string& s, size_t& i, std::string& err);

inline Value parseArray(const std::string& s, size_t& i, std::string& err) {
    Array arr;
    if (s[i] != '[') { err = "Expected ["; return {}; }
    i++;
    skipWs(s, i);
    if (i < s.size() && s[i] == ']') { i++; return Value(std::move(arr)); }
    while (i < s.size()) {
        skipWs(s, i);
        arr.push_back(parseValue(s, i, err));
        if (!err.empty()) return {};
        skipWs(s, i);
        if (i < s.size() && s[i] == ',') { i++; continue; }
        if (i < s.size() && s[i] == ']') { i++; return Value(std::move(arr)); }
        err = "Expected , or ]";
        return {};
    }
    err = "Unterminated array";
    return {};
}

inline Value parseObject(const std::string& s, size_t& i, std::string& err) {
    Object obj;
    if (s[i] != '{') { err = "Expected {"; return {}; }
    i++;
    skipWs(s, i);
    if (i < s.size() && s[i] == '}') { i++; return Value(std::move(obj)); }
    while (i < s.size()) {
        skipWs(s, i);
        std::string key = parseStringRaw(s, i, err);
        if (!err.empty()) return {};
        skipWs(s, i);
        if (i >= s.size() || s[i] != ':') { err = "Expected :"; return {}; }
        i++;
        skipWs(s, i);
        Value val = parseValue(s, i, err);
        if (!err.empty()) return {};
        obj.emplace(std::move(key), std::move(val));
        skipWs(s, i);
        if (i < s.size() && s[i] == ',') { i++; continue; }
        if (i < s.size() && s[i] == '}') { i++; return Value(std::move(obj)); }
        err = "Expected , or }";
        return {};
    }
    err = "Unterminated object";
    return {};
}

inline Value parseNumber(const std::string& s, size_t& i, std::string& err) {
    size_t start = i;
    if (s[i] == '-') i++;
    bool any = false;
    while (i < s.size() && std::isdigit((unsigned char)s[i])) { i++; any = true; }
    if (i < s.size() && s[i] == '.') {
        i++;
        while (i < s.size() && std::isdigit((unsigned char)s[i])) { i++; any = true; }
    }
    if (!any) { err = "Bad number"; return {}; }
    try {
        double d = std::stod(s.substr(start, i - start));
        return Value(d);
    } catch (...) {
        err = "Bad number";
        return {};
    }
}

inline Value parseValue(const std::string& s, size_t& i, std::string& err) {
    skipWs(s, i);
    if (i >= s.size()) { err = "Unexpected end"; return {}; }
    char c = s[i];
    if (c == '"') return Value(parseStringRaw(s, i, err));
    if (c == '{') return parseObject(s, i, err);
    if (c == '[') return parseArray(s, i, err);
    if (c == '-' || std::isdigit((unsigned char)c)) return parseNumber(s, i, err);
    if (match(s, i, "true")) return Value(true);
    if (match(s, i, "false")) return Value(false);
    if (match(s, i, "null")) return Value(nullptr);
    err = "Unexpected token";
    return {};
}

inline ParseResult parse(const std::string& s) {
    size_t i = 0;
    std::string err;
    Value v = parseValue(s, i, err);
    if (!err.empty()) return {false, Value(), err};
    skipWs(s, i);
    if (i != s.size()) return {false, Value(), "Trailing characters"};
    return {true, std::move(v), ""};
}

inline std::string escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

inline std::string stringify(const Value& v);

inline std::string stringifyObject(const Object& o) {
    std::string out = "{";
    bool first = true;
    for (const auto& kv : o) {
        if (!first) out += ",";
        first = false;
        out += "\"";
        out += escape(kv.first);
        out += "\":";
        out += stringify(kv.second);
    }
    out += "}";
    return out;
}

inline std::string stringifyArray(const Array& a) {
    std::string out = "[";
    for (size_t i = 0; i < a.size(); ++i) {
        if (i) out += ",";
        out += stringify(a[i]);
    }
    out += "]";
    return out;
}

inline std::string stringify(const Value& v) {
    if (v.isNull()) return "null";
    if (v.isBool()) return v.asBool() ? "true" : "false";
    if (v.isNumber()) {
        // avoid scientific notation in simple cases
        double d = v.asNumber();
        long long asInt = (long long)d;
        if ((double)asInt == d) return std::to_string(asInt);
        return std::to_string(d);
    }
    if (v.isString()) return "\"" + escape(v.asString()) + "\"";
    if (v.isArray()) return stringifyArray(v.asArray());
    return stringifyObject(v.asObject());
}

inline Value object(std::initializer_list<std::pair<std::string, Value>> fields) {
    Object o;
    for (auto& f : fields) o.emplace(f.first, f.second);
    return Value(std::move(o));
}

inline Value array(std::initializer_list<Value> items) {
    Array a(items.begin(), items.end());
    return Value(std::move(a));
}

} // namespace SimpleJson

#endif

