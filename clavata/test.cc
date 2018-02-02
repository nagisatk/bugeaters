#include "clavata.h"
using namespace clavata;
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
using namespace std;

static size_t tcnt = 0;
static size_t pcnt = 0;

#define CLVT_EQ_BASE(equality, expect, actual, format)       \
    do {                                                     \
        tcnt++;                                              \
        if (equality)                                        \
            pcnt++;                                          \
        else {                                               \
            cout << err << endl;                             \
            printf("expected " format ", but actual " format \
                   ", at line %d.\n",                        \
                   expect, actual, __LINE__);                \
        }                                                    \
    } while (0)

#define CLVT_EQ_INT(expect, actual) \
    CLVT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define CLVT_EQ_TYPE(expect, res) CLVT_EQ_INT(expect, res.type())
#define CLVT_EQ_BOOL(expect, res) CLVT_EQ_INT(expect, res.bool_value())

void test_literal() {
    string err;
    auto res = JSON::parse("null", err);
    CLVT_EQ_TYPE(JSON::kNULL, res);
    res = JSON::parse("false", err);
    CLVT_EQ_TYPE(JSON::kBOOL, res);
    CLVT_EQ_BOOL(false, res);
    res = JSON::parse("true", err);
    CLVT_EQ_TYPE(JSON::kBOOL, res);
    CLVT_EQ_BOOL(true, res);
}

#define CLVT_EQ_DOUBLE(expect, actual) \
    CLVT_EQ_BASE((expect) == (actual), expect, actual, "%.6f")

#define TEST_NUMBER(expect, json)                   \
    do {                                            \
        res = JSON::parse(json, err);               \
        CLVT_EQ_TYPE(JSON::kNUMBER, res);           \
        CLVT_EQ_DOUBLE(expect, res.number_value()); \
    } while (0)

void test_number() {
    string err;
    JSON res;
    // test case form leptjson by Milo Yip
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_NUMBER(1.0000000000000002,
                "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER(4.9406564584124654e-324,
                "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER(2.2250738585072009e-308,
                "2.2250738585072009e-308"); /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER(2.2250738585072014e-308,
                "2.2250738585072014e-308"); /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER(1.7976931348623157e+308,
                "1.7976931348623157e+308"); /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

#define CLVT_EQ_STRING(expect, actual)                                  \
    do {                                                                \
        tcnt++;                                                         \
        if (std::memcmp(actual.data(), expect, sizeof expect - 1) == 0) \
            pcnt++;                                                     \
        else {                                                          \
            cout << "expected " << expect << ", but actual " << actual  \
                 << ", at line " << __LINE__ << endl;                   \
        }                                                               \
    } while (0)

#define TEST_STRING(expect, json)                   \
    do {                                            \
        res = JSON::parse(json, err);               \
        CLVT_EQ_TYPE(JSON::kSTRING, res);           \
        CLVT_EQ_STRING(expect, res.string_value()); \
    } while (0)
void test_string() {
    string err;
    JSON res;
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t",
                "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    // TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E",
                "\"\\uD834\\uDD1E\""); /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E",
                "\"\\ud834\\udd1e\""); /* G clef sign U+1D11E */
}

// test parse array
#define TEST_ARRAY(json)                 \
    do {                                 \
        res = JSON::parse(json, err);    \
        CLVT_EQ_TYPE(JSON::kARRAY, res); \
    } while (0)

void test_array() {
    string err;
    JSON res;
    TEST_ARRAY("[ ]");
    CLVT_EQ_INT(0, (int)res.array_items().size());
    TEST_ARRAY("[ null , false , true , 123 , \"abc\" ]");
    CLVT_EQ_TYPE(JSON::kNULL, res[0]);
    CLVT_EQ_TYPE(JSON::kBOOL, res[1]);
    CLVT_EQ_TYPE(JSON::kBOOL, res[2]);
    CLVT_EQ_TYPE(JSON::kNUMBER, res[3]);
    CLVT_EQ_DOUBLE(123.0, res[3].number_value());
    CLVT_EQ_TYPE(JSON::kSTRING, res[4]);
    CLVT_EQ_STRING("abc", res[4].string_value());
}

#define TEST_OBJECT(json)                 \
    do {                                  \
        res = JSON::parse(json, err);     \
        CLVT_EQ_TYPE(JSON::kOBJECT, res); \
    } while (0)

void test_object() {
    string err;
    JSON res;
    TEST_OBJECT("{ }");
    TEST_OBJECT(
        " { "
        "\"n\" : null , "
        "\"f\" : false , "
        "\"t\" : true , "
        "\"i\" : 123 , "
        "\"s\" : \"abc\", "
        "\"a\" : [ 1, 2, 3 ],"
        "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
        " } ");
}

void test() {
    test_literal();
    test_number();
    test_string();
    test_array();
    test_object();
}
int main() {
    test();
    cout << "total test case: " << tcnt << ", success: " << pcnt
         << ", passed: " << fixed << setprecision(3)
         << (double)pcnt / tcnt * 100 << "%." << endl;
    system("pause");
    return 0;
}