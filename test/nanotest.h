/*
 * This is a tiny header-only unit testing framework for
 * C and C++. It is compatible with GNU-based compilers
 * and has not been tested in anything else.
 *
 * Features:
 * --  Macros for float comparison check and condition 
 *     truth/false check.
 * --  Report of successful checks.
 * --  Error report of failed assertion with valiable names
 *     and their values.
 *
 * Examples:
 *    float engineering_pi = 3;
 *    float actual_pi = 3.14159;
 *    NTEST_ASSERT_FLOAT_CLOSE(engineering_pi, actual_pi);
 *    float tolerance = 0.1;
 *    NTEST_ASSERT_FLOAT_CLOSE(3.1, actual_pi. tolerance);
 *    NTEST_ASSERT(3 == 3);
 *    // ...you can return the status of the tests; != 0
 *    // if at least one fails
 *    return ntest_result;
 *
 * Author: Leontios Mavropalias 2023
 * License: MIT
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#define NTEST_PASS 0
#define NTEST_FAIL 1
int ntest_result = NTEST_PASS;

//------------------------------------------------------------//
// Helper (internal) macros                                   //
//------------------------------------------------------------//
// credits @red1ynx (https://stackoverflow.com/a/8488201)
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// stringification macros
#define STR_VAL(s) STR_NAME(s)
#define STR_NAME(s) #s

#define GET_FIRST(args) GET_FIRST_(args)
#define GET_FIRST_(N, ...) N

// ## pastes the comma-separated variable arguments when present,
// but removes the preceeding comma when they are not present
// based on @user1187902's: https://stackoverflow.com/a/16926582
#define COUNT_ARGS(...) COUNT_ARGS_(,##__VA_ARGS__, 6, 5, 4, 3, 2, 1, 0)
// for example with input (1, 3, 37) -> (empty is before , so)
// empty="", a=1, b=3, c=37, d=6, e=5, f=4, --> cnt=3
#define COUNT_ARGS_(empty, a, b, c, d, e, f, cnt, ...) cnt

// CHecks if parameter is empty; if so it substitutes it with a
// default value so the compiler doesn't complain #args is fixed.
// credits @p00ya (https://stackoverflow.com/a/62650578)
#define CHECK_EMPTY(default_val, ...) (default_val CHECK_EMPTY_(__VA_ARGS__))
#define CHECK_EMPTY_(...) ,##__VA_ARGS__

#define NTEST_ASSERT_FLOAT_CLOSE_(actual, expected, ...)                     \
    do {                                                                     \
        const float actual_value = (actual);                                 \
        const float expected_value = (expected);                             \
        float epsilon = 0.0001;                                              \
        if (COUNT_ARGS(__VA_ARGS__) == 1)                                    \
           epsilon = CHECK_EMPTY(epsilon, GET_FIRST(__VA_ARGS__));           \
        if (fabs(actual_value - expected_value) < epsilon)                   \
            printf("[v] Assertion at line %d of file %s passed.\n",          \
				   __LINE__, __FILENAME__);                                  \
        else {                                                               \
            fprintf(stderr, "[x] Assertion at line %d (%s) failed:\n"        \
                            "    %s = %.4f and %s = %.4f are not close\n",   \
                    __LINE__, __FILE__, STR_NAME(actual), actual_value,      \
                    STR_NAME(expected), expected_value);                     \
            ntest_result = NTEST_FAIL;                                       \
        }                                                                    \
    } while(0)


//------------------------------------------------------------//
// External use macros                                        //
//------------------------------------------------------------//
#define NTEST_ASSERT_FLOAT_CLOSE(...) NTEST_ASSERT_FLOAT_CLOSE_(__VA_ARGS__)

#define NTEST_ASSERT(condition)                                               \
    do {                                                                      \
        if (condition)                                                        \
            printf("[v] Assertion at line %d of file %s passed.\n",           \
				   __LINE__, __FILENAME__);                                   \
        else {                                                                \
            fprintf(stderr, "[x] Assertion at line %d (%s) failed:\n"         \
                            "    %s\n", __LINE__, __FILE__, #condition);      \
            ntest_result = NTEST_FAIL;                                        \
        }                                                                     \
    } while(0)
