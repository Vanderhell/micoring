#include <stdio.h>

static int passed = 0;
static int failed = 0;
static int side_effect_counter = 0;

#define ASSERT_TRUE(expr)                                                         \
    do {                                                                          \
        int assert_value = !!(expr);                                              \
        if (!assert_value) {                                                      \
            fprintf(stderr, "assertion failed: %s\n", #expr);                     \
            return 1;                                                             \
        }                                                                         \
    } while (0)

static int increment_and_compare(int expected)
{
    side_effect_counter += 1;
    return side_effect_counter == expected;
}

static int passing_test(void)
{
    ASSERT_TRUE(increment_and_compare(1));
    return 0;
}

static int failing_test(void)
{
    ASSERT_TRUE(increment_and_compare(3));
    return 0;
}

static int run_test(const char *name, int (*fn)(void))
{
    int status = fn();
    if (status == 0) {
        passed += 1;
        printf("PASS %s\n", name);
    } else {
        failed += 1;
        printf("FAIL %s\n", name);
    }
    return status;
}

int main(void)
{
    int status = 0;

    status |= run_test("passing_test", passing_test);
    status |= run_test("failing_test", failing_test);

    if (passed != 1) {
        fprintf(stderr, "expected passed == 1 got %d\n", passed);
        return 1;
    }
    if (failed != 1) {
        fprintf(stderr, "expected failed == 1 got %d\n", failed);
        return 1;
    }
    if (side_effect_counter != 2) {
        fprintf(stderr, "expected side_effect_counter == 2 got %d\n", side_effect_counter);
        return 1;
    }
    return status ? 1 : 0;
}
