#ifndef _CLAVATA_H__
#define _CLAVATA_H__

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

using std::map;
using std::shared_ptr;
using std::string;
using std::vector;

namespace clavata {

class JSONValue;

class JSON final {
   public:
    enum Type { kNULL, kNUMBER, kBOOL, kSTRING, kARRAY, kOBJECT };
    typedef vector<JSON> array;
    typedef map<string, JSON> object;

    JSON() noexcept;                // null
    JSON(std::nullptr_t) noexcept;  // null
    JSON(bool);               // boolean
    JSON(int);                      // number
    JSON(double);                   // number
    JSON(const string &);           // string
    JSON(const char *);             // string
    JSON(string &&);                // string
    JSON(const array &);            // array
    JSON(array &&);                 // array
    JSON(const object &);           // object
    JSON(object &&);                // object
    // implicit constructor for types with a to_json() function
    template <class T, class = decltype(&T::to_json)>
    JSON(const T &t) : JSON(t.to_json()) {}

    JSON(void *) = delete;

    // type
    Type type() const;

    bool is_null() const { return type() == kNULL; }
    bool is_number() const { return type() == kNUMBER; }
    bool is_bool() const { return type() == kBOOL; }
    bool is_string() const { return type() == kSTRING; }
    bool is_array() const { return type() == kARRAY; }
    bool is_object() const { return type() == kOBJECT; }

    double number_value() const;
    bool bool_value() const;
    const string &string_value() const;
    const array &array_items() const;
    const object &object_items() const;

    const JSON &operator[](size_t) const;
    const JSON &operator[](const string &) const;
    // const JSON &operator[](const char *) const;

    static JSON parse(const string &, string &);
    static JSON parse(const char *in, string &err) {
        if (in) {
            return parse(string(in), err);
        } else {
            err = "null pointer.";
            return nullptr;
        }
    }

   private:
    shared_ptr<JSONValue> m;
};

class JSONValue {
    friend class JSON;

   protected:
    virtual JSON::Type type() const = 0;
    // virtual void dump(string &out) const = 0;
    virtual double number_value() const;
    virtual bool bool_value() const;
    virtual const string &string_value() const;
    virtual const JSON::array &array_items() const;
    virtual const JSON &operator[](size_t) const;
    virtual const JSON::object &object_items() const;
    virtual const JSON &operator[](const string &) const;
    // virtual const JSON &operator[](const char *) const;
    virtual ~JSONValue() {}
};

}  // namespace clavata

#endif  // _CLAVATA_H__