/*
 * micoring runtime test suite.
 *
 * This file is intentionally repository-local and C99-only.
 */

#define MRING_TEST 1

#include "mring.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

typedef int (*test_fn_t)(void);

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static int name(void)

#define ASSERT_EQ_INT(expected, actual)                                               \
    do {                                                                              \
        int assert_eq_expected_value = (expected);                                    \
        int assert_eq_actual_value = (actual);                                        \
        if (assert_eq_expected_value != assert_eq_actual_value) {                     \
            fprintf(stderr, "%s:%d expected %d got %d\n", __FILE__, __LINE__,         \
                assert_eq_expected_value, assert_eq_actual_value);                    \
            return 1;                                                                 \
        }                                                                             \
    } while (0)

#define ASSERT_EQ_SIZE(expected, actual)                                              \
    do {                                                                              \
        size_t assert_eq_expected_value = (expected);                                 \
        size_t assert_eq_actual_value = (actual);                                     \
        if (assert_eq_expected_value != assert_eq_actual_value) {                     \
            fprintf(stderr, "%s:%d expected %lu got %lu\n", __FILE__, __LINE__,       \
                (unsigned long)assert_eq_expected_value,                              \
                (unsigned long)assert_eq_actual_value);                               \
            return 1;                                                                 \
        }                                                                             \
    } while (0)

#define ASSERT_TRUE(expr)                                                             \
    do {                                                                              \
        int assert_true_value = !!(expr);                                             \
        if (!assert_true_value) {                                                     \
            fprintf(stderr, "%s:%d expected true: %s\n", __FILE__, __LINE__, #expr); \
            return 1;                                                                 \
        }                                                                             \
    } while (0)

#define ASSERT_FALSE(expr)                                                            \
    do {                                                                              \
        int assert_false_value = !!(expr);                                            \
        if (assert_false_value) {                                                     \
            fprintf(stderr, "%s:%d expected false: %s\n", __FILE__, __LINE__, #expr);\
            return 1;                                                                 \
        }                                                                             \
    } while (0)

#define ASSERT_STR_EQ(expected, actual)                                               \
    do {                                                                              \
        const char *assert_str_expected_value = (expected);                           \
        const char *assert_str_actual_value = (actual);                               \
        if (strcmp(assert_str_expected_value, assert_str_actual_value) != 0) {        \
            fprintf(stderr, "%s:%d expected %s got %s\n", __FILE__, __LINE__,        \
                assert_str_expected_value, assert_str_actual_value);                  \
            return 1;                                                                 \
        }                                                                             \
    } while (0)

static int run_test(const char *name, test_fn_t fn)
{
    int status = 0;

    tests_run += 1;
    status = fn();
    if (status == 0) {
        tests_passed += 1;
        printf("PASS %s\n", name);
    } else {
        tests_failed += 1;
        printf("FAIL %s\n", name);
    }
    return status;
}

#define RUN_TEST(name) run_test(#name, name)

MRING_DEFINE_TYPED(u8ring, uint8_t); MRING_DEFINE_TYPED(u32ring, uint32_t)

typedef struct {
    uint16_t sensor_id;
    uint16_t state;
    uint32_t reading;
} sample_t;

MRING_DEFINE_TYPED(sample_ring, sample_t)

static mring_t ring;
static uint8_t storage_u32[8U * sizeof(uint32_t)];
static uint8_t storage_u8[8U * sizeof(uint8_t)];
static uint8_t storage_sample[4U * sizeof(sample_t)];

static void setup_ring_u32(void)
{
    memset(&ring, 0, sizeof(ring));
    memset(storage_u32, 0, sizeof(storage_u32));
    (void)mring_init(&ring, storage_u32, sizeof(storage_u32), 8U, sizeof(uint32_t));
}

static void setup_ring_u8(void)
{
    memset(&ring, 0, sizeof(ring));
    memset(storage_u8, 0, sizeof(storage_u8));
    (void)mring_init(&ring, storage_u8, sizeof(storage_u8), 8U, sizeof(uint8_t));
}

TEST(test_init_exact_storage_size)
{
    mring_t local_ring;
    uint8_t local_storage[8U * sizeof(uint32_t)];
    ASSERT_EQ_INT(MRING_OK, mring_init(&local_ring, local_storage, sizeof(local_storage), 8U, sizeof(uint32_t)));
    ASSERT_EQ_SIZE(sizeof(local_storage), local_ring.storage_size);
    return 0;
}

TEST(test_init_rejects_zero_capacity)
{
    mring_t local_ring;
    uint8_t local_storage[8U];
    ASSERT_EQ_INT(MRING_ERR_INVALID, mring_init(&local_ring, local_storage, sizeof(local_storage), 0U, 1U));
    return 0;
}

TEST(test_init_rejects_non_power_of_two_capacity)
{
    mring_t local_ring;
    uint8_t local_storage[6U];
    ASSERT_EQ_INT(MRING_ERR_INVALID, mring_init(&local_ring, local_storage, sizeof(local_storage), 6U, 1U));
    return 0;
}

TEST(test_init_rejects_zero_element_size)
{
    mring_t local_ring;
    uint8_t local_storage[8U];
    ASSERT_EQ_INT(MRING_ERR_SIZE, mring_init(&local_ring, local_storage, sizeof(local_storage), 8U, 0U));
    return 0;
}

TEST(test_init_rejects_insufficient_storage)
{
    mring_t local_ring;
    uint8_t local_storage[15U];
    ASSERT_EQ_INT(MRING_ERR_SIZE, mring_init(&local_ring, local_storage, sizeof(local_storage), 8U, 2U));
    return 0;
}

TEST(test_init_rejects_overflow)
{
    mring_t local_ring;
    uint8_t local_storage[8U];
    ASSERT_EQ_INT(MRING_ERR_SIZE, mring_init(&local_ring, local_storage, sizeof(local_storage), 8U, (SIZE_MAX / 4U) + 1U));
    return 0;
}

TEST(test_init_rejects_large_capacity_combo)
{
    mring_t local_ring;
    uint8_t local_storage[8U];
    ASSERT_EQ_INT(MRING_ERR_SIZE, mring_init(&local_ring, local_storage, sizeof(local_storage), 0x80000000U, 2U));
    return 0;
}

TEST(test_init_rejects_overlapping_ring_and_storage)
{
    union {
        mring_t ring_value;
        uint8_t bytes[sizeof(mring_t) + 64U];
    } overlap;
    ASSERT_EQ_INT(MRING_ERR_INVALID, mring_init(&overlap.ring_value, overlap.bytes, sizeof(overlap.bytes), 8U, 4U));
    return 0;
}

TEST(test_capacity_one)
{
    mring_t local_ring;
    uint32_t value = 11U;
    uint32_t out = 0U;
    uint8_t local_storage[sizeof(uint32_t)];
    bool full = false;

    ASSERT_EQ_INT(MRING_OK, mring_init(&local_ring, local_storage, sizeof(local_storage), 1U, sizeof(uint32_t)));
    ASSERT_EQ_INT(MRING_OK, mring_push(&local_ring, &value));
    ASSERT_EQ_INT(MRING_OK, mring_is_full(&local_ring, &full));
    ASSERT_TRUE(full);
    ASSERT_EQ_INT(MRING_ERR_FULL, mring_push(&local_ring, &value));
    ASSERT_EQ_INT(MRING_OK, mring_pop(&local_ring, &out));
    ASSERT_EQ_INT(11, (int)out);
    return 0;
}

TEST(test_push_pop_fifo)
{
    uint32_t value = 0U;
    uint32_t out = 0U;
    setup_ring_u32();
    for (value = 0U; value < 8U; ++value) {
        ASSERT_EQ_INT(MRING_OK, mring_push(&ring, &value));
    }
    for (value = 0U; value < 8U; ++value) {
        ASSERT_EQ_INT(MRING_OK, mring_pop(&ring, &out));
        ASSERT_EQ_INT((int)value, (int)out);
    }
    return 0;
}

TEST(test_single_element_same_address_copy)
{
    uint32_t value = 7U;
    setup_ring_u32();
    ASSERT_EQ_INT(MRING_OK, mring_push(&ring, &value));
    ASSERT_EQ_INT(MRING_OK, mring_peek(&ring, storage_u32));
    ASSERT_EQ_INT(MRING_OK, mring_pop(&ring, storage_u32));
    return 0;
}

TEST(test_push_pop_null_arguments)
{
    uint32_t value = 0U;
    setup_ring_u32();
    ASSERT_EQ_INT(MRING_ERR_NULL, mring_push(NULL, &value));
    ASSERT_EQ_INT(MRING_ERR_NULL, mring_push(&ring, NULL));
    ASSERT_EQ_INT(MRING_ERR_NULL, mring_pop(NULL, &value));
    ASSERT_EQ_INT(MRING_ERR_NULL, mring_peek(NULL, &value));
    ASSERT_EQ_INT(MRING_ERR_NULL, mring_peek(&ring, NULL));
    return 0;
}

TEST(test_peek_at_contracts)
{
    uint32_t value = 17U;
    uint32_t out = 0U;
    setup_ring_u32();
    ASSERT_EQ_INT(MRING_ERR_EMPTY, mring_peek_at(&ring, 0U, &out));
    ASSERT_EQ_INT(MRING_ERR_NULL, mring_peek_at(NULL, 0U, &out));
    ASSERT_EQ_INT(MRING_ERR_NULL, mring_peek_at(&ring, 0U, NULL));
    ASSERT_EQ_INT(MRING_OK, mring_push(&ring, &value));
    ASSERT_EQ_INT(MRING_ERR_SIZE, mring_peek_at(&ring, 1U, &out));
    return 0;
}

TEST(test_typed_wrapper_mismatch_u8_vs_u32)
{
    uint8_t out = 0U;
    setup_ring_u32();
    ASSERT_EQ_INT(MRING_ERR_TYPE, u8ring_push(&ring, 1U));
    ASSERT_EQ_INT(MRING_ERR_TYPE, u8ring_pop(&ring, &out));
    ASSERT_EQ_INT(MRING_ERR_TYPE, u8ring_peek(&ring, &out));
    return 0;
}

TEST(test_typed_wrapper_mismatch_u32_vs_u8)
{
    uint32_t out = 0U;
    setup_ring_u8();
    ASSERT_EQ_INT(MRING_ERR_TYPE, u32ring_push(&ring, 1U));
    ASSERT_EQ_INT(MRING_ERR_TYPE, u32ring_pop(&ring, &out));
    ASSERT_EQ_INT(MRING_ERR_TYPE, u32ring_peek(&ring, &out));
    return 0;
}

TEST(test_typed_wrapper_scalar_match)
{
    uint32_t out = 0U;
    setup_ring_u32();
    ASSERT_EQ_INT(MRING_OK, u32ring_push(&ring, 3U));
    ASSERT_EQ_INT(MRING_OK, u32ring_pop(&ring, &out));
    ASSERT_EQ_INT(3, (int)out);
    return 0;
}

TEST(test_typed_wrapper_struct_match)
{
    mring_t local_ring;
    sample_t sample = { 1U, 2U, 3U };
    sample_t out;

    memset(storage_sample, 0, sizeof(storage_sample));
    ASSERT_EQ_INT(MRING_OK, mring_init(&local_ring, storage_sample, sizeof(storage_sample), 4U, sizeof(sample_t)));
    ASSERT_EQ_INT(MRING_OK, sample_ring_push(&local_ring, sample));
    ASSERT_EQ_INT(MRING_OK, sample_ring_pop(&local_ring, &out));
    ASSERT_EQ_INT(1, (int)out.sensor_id);
    ASSERT_EQ_INT(3, (int)out.reading);
    return 0;
}

TEST(test_overwrite_supported_in_single_context)
{
    uint32_t value = 0U;
    uint32_t out = 0U;
    setup_ring_u32();
    for (value = 0U; value < 8U; ++value) {
        ASSERT_EQ_INT(MRING_OK, mring_push(&ring, &value));
    }
    value = 99U;
    ASSERT_EQ_INT(MRING_OK, mring_push_overwrite(&ring, &value));
    ASSERT_EQ_INT(MRING_OK, mring_pop(&ring, &out));
    ASSERT_EQ_INT(1, (int)out);
    return 0;
}

TEST(test_batch_zero_partial_full_and_empty)
{
    uint32_t input[5] = { 10U, 20U, 30U, 40U, 50U };
    uint32_t output[5] = { 0U, 0U, 0U, 0U, 0U };
    size_t transferred = 99U;

    setup_ring_u32();
    ASSERT_EQ_INT(MRING_OK, mring_push_many(&ring, input, 0U, &transferred));
    ASSERT_EQ_SIZE(0U, transferred);
    ASSERT_EQ_INT(MRING_OK, mring_push_many(&ring, input, 5U, &transferred));
    ASSERT_EQ_SIZE(5U, transferred);
    ASSERT_EQ_INT(MRING_OK, mring_push_many(&ring, input, 5U, &transferred));
    ASSERT_EQ_SIZE(3U, transferred);
    ASSERT_EQ_INT(MRING_OK, mring_pop_many(&ring, output, 5U, &transferred));
    ASSERT_EQ_SIZE(5U, transferred);
    ASSERT_EQ_INT(10, (int)output[0]);
    ASSERT_EQ_INT(MRING_OK, mring_pop_many(&ring, output, 5U, &transferred));
    ASSERT_EQ_SIZE(3U, transferred);
    ASSERT_EQ_INT(MRING_OK, mring_pop_many(&ring, output, 5U, &transferred));
    ASSERT_EQ_SIZE(0U, transferred);
    return 0;
}

TEST(test_batch_discard_and_overlap_rejection)
{
    uint32_t input[4] = { 1U, 2U, 3U, 4U };
    size_t transferred = 0U;

    setup_ring_u32();
    ASSERT_EQ_INT(MRING_OK, mring_push_many(&ring, input, 4U, &transferred));
    ASSERT_EQ_SIZE(4U, transferred);
    ASSERT_EQ_INT(MRING_OK, mring_pop_many(&ring, NULL, 2U, &transferred));
    ASSERT_EQ_SIZE(2U, transferred);
    ASSERT_EQ_INT(MRING_ERR_INVALID, mring_push_many(&ring, storage_u32, 1U, &transferred));
    ASSERT_EQ_INT(MRING_ERR_INVALID, mring_pop_many(&ring, storage_u32, 1U, &transferred));
    return 0;
}

TEST(test_query_null_handling)
{
    uint32_t count = 0U;
    size_t elem_size = 0U;
    bool flag = false;

    setup_ring_u32();
    ASSERT_EQ_INT(MRING_ERR_NULL, mring_capacity(NULL, &count));
    ASSERT_EQ_INT(MRING_ERR_NULL, mring_capacity(&ring, NULL));
    ASSERT_EQ_INT(MRING_ERR_NULL, mring_element_size(NULL, &elem_size));
    ASSERT_EQ_INT(MRING_ERR_NULL, mring_count(NULL, &count));
    ASSERT_EQ_INT(MRING_ERR_NULL, mring_free(NULL, &count));
    ASSERT_EQ_INT(MRING_ERR_NULL, mring_is_empty(NULL, &flag));
    ASSERT_EQ_INT(MRING_ERR_NULL, mring_is_full(NULL, &flag));
    return 0;
}

TEST(test_query_values)
{
    uint32_t count = 0U;
    uint32_t free_slots = 0U;
    uint32_t capacity = 0U;
    size_t elem_size = 0U;
    bool flag = false;
    uint32_t value = 7U;

    setup_ring_u32();
    ASSERT_EQ_INT(MRING_OK, mring_push(&ring, &value));
    ASSERT_EQ_INT(MRING_OK, mring_count(&ring, &count));
    ASSERT_EQ_INT(MRING_OK, mring_free(&ring, &free_slots));
    ASSERT_EQ_INT(MRING_OK, mring_capacity(&ring, &capacity));
    ASSERT_EQ_INT(MRING_OK, mring_element_size(&ring, &elem_size));
    ASSERT_EQ_INT(MRING_OK, mring_is_empty(&ring, &flag));
    ASSERT_EQ_INT(1, (int)count);
    ASSERT_EQ_INT(7, (int)free_slots);
    ASSERT_EQ_INT(8, (int)capacity);
    ASSERT_EQ_SIZE(sizeof(uint32_t), elem_size);
    ASSERT_FALSE(flag);
    return 0;
}

TEST(test_wrap_push_pop_near_uint32_max)
{
    uint32_t value = 0U;
    uint32_t out = 0U;
    setup_ring_u32();
    ASSERT_EQ_INT(MRING_OK, mring_test_set_counters(&ring, UINT32_MAX - 3U, UINT32_MAX - 3U));
    for (value = 0U; value < 4U; ++value) {
        ASSERT_EQ_INT(MRING_OK, mring_push(&ring, &value));
    }
    for (value = 0U; value < 4U; ++value) {
        ASSERT_EQ_INT(MRING_OK, mring_pop(&ring, &out));
        ASSERT_EQ_INT((int)value, (int)out);
    }
    return 0;
}

TEST(test_wrap_full_empty_and_batch)
{
    uint32_t input[4] = { 8U, 9U, 10U, 11U };
    uint32_t output[4] = { 0U, 0U, 0U, 0U };
    size_t transferred = 0U;
    bool full = false;
    bool empty = false;

    setup_ring_u32();
    ASSERT_EQ_INT(MRING_OK, mring_test_set_counters(&ring, UINT32_MAX - 1U, UINT32_MAX - 1U));
    ASSERT_EQ_INT(MRING_OK, mring_push_many(&ring, input, 4U, &transferred));
    ASSERT_EQ_SIZE(4U, transferred);
    ASSERT_EQ_INT(MRING_OK, mring_is_full(&ring, &full));
    ASSERT_TRUE(full);
    ASSERT_EQ_INT(MRING_OK, mring_pop_many(&ring, output, 4U, &transferred));
    ASSERT_EQ_SIZE(4U, transferred);
    ASSERT_EQ_INT(MRING_OK, mring_is_empty(&ring, &empty));
    ASSERT_TRUE(empty);
    ASSERT_EQ_INT(8, (int)output[0]);
    ASSERT_EQ_INT(11, (int)output[3]);
    return 0;
}

TEST(test_repeated_fill_drain)
{
    uint32_t cycle = 0U;
    uint32_t value = 0U;
    uint32_t out = 0U;

    setup_ring_u32();
    for (cycle = 0U; cycle < 32U; ++cycle) {
        for (value = 0U; value < 8U; ++value) {
            uint32_t payload = (cycle * 100U) + value;
            ASSERT_EQ_INT(MRING_OK, mring_push(&ring, &payload));
        }
        for (value = 0U; value < 8U; ++value) {
            ASSERT_EQ_INT(MRING_OK, mring_pop(&ring, &out));
            ASSERT_EQ_INT((int)((cycle * 100U) + value), (int)out);
        }
    }
    return 0;
}

TEST(test_err_strings)
{
    ASSERT_STR_EQ("ok", mring_err_str(MRING_OK));
    ASSERT_STR_EQ("element type mismatch", mring_err_str(MRING_ERR_TYPE));
    ASSERT_STR_EQ("unsupported operation", mring_err_str(MRING_ERR_UNSUPPORTED));
    ASSERT_STR_EQ("unknown error", mring_err_str((mring_err_t)1234));
    return 0;
}

int main(void)
{
    int failed = 0;

    failed |= RUN_TEST(test_init_exact_storage_size);
    failed |= RUN_TEST(test_init_rejects_zero_capacity);
    failed |= RUN_TEST(test_init_rejects_non_power_of_two_capacity);
    failed |= RUN_TEST(test_init_rejects_zero_element_size);
    failed |= RUN_TEST(test_init_rejects_insufficient_storage);
    failed |= RUN_TEST(test_init_rejects_overflow);
    failed |= RUN_TEST(test_init_rejects_large_capacity_combo);
    failed |= RUN_TEST(test_init_rejects_overlapping_ring_and_storage);
    failed |= RUN_TEST(test_capacity_one);
    failed |= RUN_TEST(test_push_pop_fifo);
    failed |= RUN_TEST(test_single_element_same_address_copy);
    failed |= RUN_TEST(test_push_pop_null_arguments);
    failed |= RUN_TEST(test_peek_at_contracts);
    failed |= RUN_TEST(test_typed_wrapper_mismatch_u8_vs_u32);
    failed |= RUN_TEST(test_typed_wrapper_mismatch_u32_vs_u8);
    failed |= RUN_TEST(test_typed_wrapper_scalar_match);
    failed |= RUN_TEST(test_typed_wrapper_struct_match);
    failed |= RUN_TEST(test_overwrite_supported_in_single_context);
    failed |= RUN_TEST(test_batch_zero_partial_full_and_empty);
    failed |= RUN_TEST(test_batch_discard_and_overlap_rejection);
    failed |= RUN_TEST(test_query_null_handling);
    failed |= RUN_TEST(test_query_values);
    failed |= RUN_TEST(test_wrap_push_pop_near_uint32_max);
    failed |= RUN_TEST(test_wrap_full_empty_and_batch);
    failed |= RUN_TEST(test_repeated_fill_drain);
    failed |= RUN_TEST(test_err_strings);

    printf("SUMMARY %d run %d passed %d failed\n", tests_run, tests_passed, tests_failed);
    return failed ? 1 : 0;
}
