#include "clavata.h"

#include <iostream>
using std::make_shared;
using std::move;
#include <cassert>
#define CLVT_ASSERT(expr) assert(expr)

namespace clavata {

template <JSON::Type ty, typename T>
class Value : public JSONValue {
   protected:
    const T m_value;
    // constructors
    explicit Value(const T &t) : m_value(t) {}
    explicit Value(T &&t) : m_value(move(t)) {}

    // type
    JSON::Type type() const override {
        // std::cout << "Value type is " << (int)ty << std::endl;
        return ty;
    }
};

// for JSON null
struct NullStruct {};
class ClvtNull final : public Value<JSON::kNULL, NullStruct> {
   public:
    explicit ClvtNull() : Value({}) {}
};

class ClvtNumber final : public Value<JSON::kNUMBER, double> {
    double number_value() const override { return m_value; }

   public:
    explicit ClvtNumber(double d) : Value(d) {}
};

class ClvtBool final : public Value<JSON::kBOOL, bool> {
    bool bool_value() const override { return m_value; }

   public:
    explicit ClvtBool(bool b) : Value(b) {
        // std::cout << "ClvtBool Type is " << (int)type() << std::endl;
    }
};

class ClvtString final : public Value<JSON::kSTRING, string> {
    const string &string_value() const override { return m_value; }

   public:
    explicit ClvtString(const string &s) : Value(s) {}
    explicit ClvtString(string &&s) : Value(move(s)) {}
};

class ClvtArray final : public Value<JSON::kARRAY, JSON::array> {
    const JSON::array &array_items() const override { return m_value; }
    const JSON &operator[](size_t) const override;

   public:
    explicit ClvtArray(const JSON::array &a) : Value(a) {}
    explicit ClvtArray(JSON::array &&a) : Value(move(a)) {}
};

class ClvtObject final : public Value<JSON::kOBJECT, JSON::object> {
    const JSON::object &object_items() const override { return m_value; }
    const JSON &operator[](const string &) const override;

   public:
    explicit ClvtObject(const JSON::object &o) : Value(o) {}
    explicit ClvtObject(JSON::object &&o) : Value(move(o)) {}
};

struct Statics {
    const shared_ptr<JSONValue> null = make_shared<ClvtNull>();
    const shared_ptr<JSONValue> f = make_shared<ClvtBool>(false);
    const shared_ptr<JSONValue> t = make_shared<ClvtBool>(true);
    const string empty_string;
    const vector<JSON> empty_array;
    const map<string, JSON> empty_map;
    Statics() {}
};

static const Statics &statics() {
    static const Statics s{};
    return s;
}

static const JSON &statics_null() {
    static const JSON s;
    return s;
}

JSON::JSON() noexcept : m(statics().null) {}
JSON::JSON(std::nullptr_t) noexcept : m(statics().null) {}
JSON::JSON(bool b) : m(b ? statics().t : statics().f) {}
JSON::JSON(int i) : m(make_shared<ClvtNumber>(i)) {}
JSON::JSON(double d) : m(make_shared<ClvtNumber>(d)) {}
JSON::JSON(const string &s) : m(make_shared<ClvtString>(s)) {}
JSON::JSON(const char *s) : m(make_shared<ClvtString>(s)) {}
JSON::JSON(string &&s) : m(make_shared<ClvtString>(move(s))) {}
JSON::JSON(const array &a) : m(make_shared<ClvtArray>(a)) {}
JSON::JSON(array &&a) : m(make_shared<ClvtArray>(move(a))) {}
JSON::JSON(const object &o) : m(make_shared<ClvtObject>(o)) {}
JSON::JSON(object &&o) : m(make_shared<ClvtObject>(move(o))) {}

JSON::Type JSON::type() const { return m->type(); }

double JSON::number_value() const { return m->number_value(); }
bool JSON::bool_value() const { return m->bool_value(); }
const string &JSON::string_value() const { return m->string_value(); }
const JSON::array &JSON::array_items() const { return m->array_items(); }
const JSON::object &JSON::object_items() const { return m->object_items(); }

const JSON &JSON::operator[](size_t i) const { return (*m)[i]; }
const JSON &JSON::operator[](const string &key) const { return (*m)[key]; }

double JSONValue::number_value() const { return 0; }
bool JSONValue::bool_value() const { return false; }
const string &JSONValue::string_value() const { return statics().empty_string; }
const JSON::array &JSONValue::array_items() const {
    return statics().empty_array;
}
const JSON &JSONValue::operator[](size_t) const { return statics_null(); }
const JSON::object &JSONValue::object_items() const {
    return statics().empty_map;
}
const JSON &JSONValue::operator[](const string &) const {
    return statics_null();
}

const JSON &ClvtArray::operator[](size_t i) const {
    if (i >= m_value.size()) return statics_null();
    return m_value[i];
}
const JSON &ClvtObject::operator[](const string &key) const {
    auto iter = m_value.find(key);
    return iter == m_value.end() ? statics_null() : iter->second;
}

#define in_range(c, s, e) (s <= c && c <= e)
#define IS_DIGIT(c) in_range(c, '0', '9')
#define IS_INTEGER(c) in_range(c, '1', '9')

/**
 * from json11
 * format char c suitable for printing an error message
 */

static inline string format_char(char c) {
    char buf[12];
    if (static_cast<uint8_t>(c) >= 0x20 && static_cast<uint8_t>(c) <= 0x7f)
        snprintf(buf, sizeof buf, "(%c), %d", c, c);
    else
        snprintf(buf, sizeof buf, "(%d)", c);
    return string(buf);
}
#define is_hex(ch) \
    (in_range(ch, '0', '9') || in_range(ch, 'A', 'F') || in_range(ch, 'a', 'f'))

struct ClvtParser final {
    const string &src;
    size_t i;
    bool failed;
    string &err;
    void fail(string msg) {
        failed = true;
        err = msg;
    }
    JSON parse_literal(const string &expect, JSON res) {
        auto iter = expect.begin();
        while (iter != expect.end())
            if (*(iter++) != src[i++]) break;
        if (iter == expect.end()) return res;
        fail("syntax error in parsing `" + expect + "`.");
        return JSON();
    }
    long encode_hex4() {
        assert(src[i] == 'u');
        if (src.length() - i < 4) {
            fail("no enough hex charactor for encode.");
            return -1;
        }
        i++;
        for (unsigned a = 0; a < 4; a++) {
            if (!is_hex(src[i + a])) {
                fail("invalid hex charactor " + format_char(src[i + a]) + ".");
                return -1;
            }
        }
        return strtol(src.substr(i, 4).data(), nullptr, 16);
    }
    /**
     * from json11
     */
    void encode_utf8(long cp, string &out) {
        if (cp < 0) return;

        if (cp < 0x80) {
            out += static_cast<char>(cp);
        } else if (cp < 0x800) {
            out += static_cast<char>((cp >> 6) | 0xC0);
            out += static_cast<char>((cp & 0x3F) | 0x80);
        } else if (cp < 0x10000) {
            out += static_cast<char>((cp >> 12) | 0xE0);
            out += static_cast<char>(((cp >> 6) & 0x3F) | 0x80);
            out += static_cast<char>((cp & 0x3F) | 0x80);
        } else {
            out += static_cast<char>((cp >> 18) | 0xF0);
            out += static_cast<char>(((cp >> 12) & 0x3F) | 0x80);
            out += static_cast<char>(((cp >> 6) & 0x3F) | 0x80);
            out += static_cast<char>((cp & 0x3F) | 0x80);
        }
    }

    JSON parse_object() {
        assert(src[i] == '{');
        i++;
        map<string, JSON> o;
        while (true) {
            skip_whitespace();
            if (src[i] == '}') {
                i++;
                return o;
            }
            if (src[i] != '"') {
                fail("expect a string as key in JSON object.");
                return JSON();
            }
            JSON k = parse_string();
            if (failed) return JSON();
            skip_whitespace();
            if (src[i] != ':') {
                fail(
                    "expect a `:` to separate key and value in JSON object, "
                    "but got a " +
                    format_char(src[i]) + ".");
                return JSON();
            }
            i++;
            skip_whitespace();
            JSON v = parse_json();
            if (failed) return JSON();
            o.insert({std::move(k.string_value()), v});
            skip_whitespace();
            if (src[i] != ',') {
                if (src[i] != '}') {
                    fail("expect a comma in object, but got a " +
                         format_char(src[i]) + ".");
                    return JSON();
                }
                i++;
                return o;
            }
            // ignore trailing comma
            i++;
        }
        assert(0);
        return JSON();
    }

    JSON parse_array() {
        assert(src[i] == '[');
        i++;
        vector<JSON> vec;
        while (true) {
            skip_whitespace();
            if (src[i] == ']') {
                i++;
                return vec;
            }
            JSON j = parse_json();
            vec.push_back(j);
            skip_whitespace();
            if (src[i] == ',')
                i++;
            else {
                if (src[i] != ']') {
                    fail("need a comma.");
                    return JSON();
                }
                i++;
                return vec;
            }
        }
        assert(0);
        return JSON();
    }

    JSON parse_string() {
        assert(src[i] == '"');
        i++;
        string res = "";
        while (true) {
            if (i == src.size()) {
                fail("unexpected end of input string.");
                return JSON();
            }
            char ch = src[i++];
            if (ch == '"') {
                return res;
            } else if (ch == '\\') {
                switch (src[i]) {
                    case '"':
                    case '\\':
                    case '/':
                        res += src[i++];
                        break;
                    case 'b':
                        i++;
                        res += '\b';
                        break;
                    case 'f':
                        i++;
                        res += '\f';
                        break;
                    case 'n':
                        i++;
                        res += '\n';
                        break;
                    case 'r':
                        i++;
                        res += '\r';
                        break;
                    case 't':
                        i++;
                        res += '\t';
                        break;
                    case 'u':
                        long cp = encode_hex4();
                        if (cp < 0) {
                            return JSON();
                        }
                        i += 4;
                        if (cp >= 0xD800 && cp <= 0xDBFF) {
                            if (src[i] != '\\' || src[i + 1] != 'u') {
                                fail("expect a surrogate pair but not get.");
                                return JSON();
                            }
                            i++;
                            long cp2 = encode_hex4();
                            if (cp2 < 0xDC00 || cp2 > 0xDFFF) {
                                fail("invalid surrogate pair with " +
                                     std::to_string(cp) + " and " +
                                     std::to_string(cp2));
                                return JSON();
                            }
                            cp = (((cp - 0xD800) << 10) | (cp2 - 0xDC00)) +
                                 0x10000;
                        }
                        encode_utf8(cp, res);
                        break;
                }
            } else {
                res += ch;
            }
        }
    }

    JSON parse_number() {
        // start position
        size_t sp = i;
        // prefix +/-
        if (src[i] == '-' || src[i] == '+') i++;
        // integer part
        if (src[i] == '0') {
            i++;
            if (IS_DIGIT(src[i])) {
                fail("leading 0(s) not permitted in numbers.");
                return JSON();
            }
        } else if (IS_INTEGER(src[i])) {
            i++;
            while (IS_DIGIT(src[i])) i++;
        } else {
            fail("invalid charactor " + format_char(src[i]) + " in numbers.");
            return JSON();
        }
        // fraction part
        if (src[i] == '.') {
            i++;
            if (!IS_DIGIT(src[i])) {
                fail("at least a decimal required in fractional part.");
                return JSON();
            }
            while (IS_DIGIT(src[i])) i++;
        }

        // exponent part
        if (src[i] == 'e' || src[i] == 'E') {
            i++;
            if (src[i] == '-' || src[i] == '+') i++;
            if (!IS_DIGIT(src[i])) {
                fail("at least a decimal required in exponential part.");
                return JSON();
            }
            while (IS_DIGIT(src[i])) i++;
        }
        return strtod(src.c_str() + sp, nullptr);
    }

    JSON parse_json() {
        skip_whitespace();
        while (i != src.size()) {
            switch (src[i]) {
                case 'n':
                    return parse_literal("null", JSON());
                case 'f':
                    return parse_literal("false", false);
                case 't':
                    return parse_literal("true", true);
                case '"':
                    return parse_string();
                case '[':
                    return parse_array();
                case '{':
                    // std::cout << src << std::endl;
                    return parse_object();
                default:
                    return parse_number();
            }
        }
        CLVT_ASSERT(0);
        return JSON();
    }
    void skip_whitespace() {
        while (src[i] == ' ' || src[i] == '\n' || src[i] == '\r' ||
               src[i] == '\t')
            i++;
    }
};

JSON JSON::parse(const string &in, string &err) {
    ClvtParser cp{in, 0, false, err};
    JSON res = cp.parse_json();
    cp.skip_whitespace();
    if (cp.failed) return JSON();
    if (cp.i != in.size()) return JSON();
    return res;
}

}  // namespace clavata