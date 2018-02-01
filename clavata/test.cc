#include "clavata.h"
using namespace clavata;
#include <cstdio>
#include <iomanip>
#include <iostream>
using namespace std;

static size_t tcnt = 0;
static size_t pcnt = 0;

#define CLVT_EQ_BASE(expect, actual, format)                 \
    do {                                                     \
        tcnt++;                                              \
        if (expect == actual)                                \
            pcnt++;                                          \
        else {                                               \
            printf("expected " format ", but actual " format \
                   ", at line %d.\n",                        \
                   expect, actual, __LINE__);                \
        }                                                    \
    } while (0)

#define CLVT_EQ_INT(expect, actual) CLVT_EQ_BASE(expect, actual, "%d")
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
void test() { test_literal(); }
int main() {
    test();
    cout << "total test case: " << tcnt << ", success: " << pcnt
         << ", passed: " << fixed << setprecision(3)
         << (double)pcnt / tcnt * 100 << "%." << endl;

    system("pause");
    return 0;
}