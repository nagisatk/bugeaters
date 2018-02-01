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

struct ClvtParser final {
    const string &src;
    size_t i;
    bool fail;
    string &err;
    JSON parse_literal(const string &expect, JSON res) {
        auto iter = expect.begin();
        while (iter != expect.end())
            if (*(iter++) != src[i++]) break;
        if (iter == expect.end()) return res;
        return JSON();
    }
    JSON parse_object() { return JSON(); }
    JSON parse_array() { return JSON(); }
    JSON parse_string() { return JSON(); }
    JSON parse_number() { return JSON(); }
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
    if (cp.fail) return JSON();
    if (cp.i != in.size()) return JSON();
    return res;
}

}  // namespace clavata