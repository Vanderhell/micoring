/*
 * micoring test suite.
 *
 * Build: gcc -std=c99 -Wall -Wextra -I../include ../src/mring.c test_all.c -o test_all
 * Run:   ./test_all
 */

#include "mring.h"
#include <stdio.h>
#include <string.h>

/* ── Minimal test framework ────────────────────────────────────────────── */

static int tests_run = 0, tests_passed = 0, tests_failed = 0;

#define TEST(name) static void name(void)
#define RUN_TEST(name) do {                                     \
    tests_run++;                                                \
    printf("  %-55s ", #name);                                  \
    name();                                                     \
    printf("PASS\n");                                           \
    tests_passed++;                                             \
} while (0)

#define ASSERT_EQ(expected, actual) do {                        \
    if ((expected) != (actual)) {                               \
        printf("FAIL\n    %s:%d: expected %d, got %d\n",       \
               __FILE__, __LINE__, (int)(expected), (int)(actual)); \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_TRUE(expr) do {                                  \
    if (!(expr)) {                                              \
        printf("FAIL\n    %s:%d: expected true\n",              \
               __FILE__, __LINE__);                             \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_FALSE(expr) do {                                 \
    if ((expr)) {                                               \
        printf("FAIL\n    %s:%d: expected false\n",             \
               __FILE__, __LINE__);                             \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_STR_EQ(expected, actual) do {                    \
    if (strcmp((expected), (actual)) != 0) {                     \
        printf("FAIL\n    %s:%d: expected \"%s\", got \"%s\"\n",\
               __FILE__, __LINE__, (expected), (actual));       \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

/* ── Typed wrappers for tests ──────────────────────────────────────────── */

MRING_DEFINE_TYPED(u32ring, uint32_t)
MRING_DEFINE_TYPED(u8ring, uint8_t)

/* ── Helpers ───────────────────────────────────────────────────────────── */

#define BUF_CAP 8

static uint8_t storage_u32[BUF_CAP * sizeof(uint32_t)];
static uint8_t storage_u8[BUF_CAP * sizeof(uint8_t)];
static mring_t ring;

static void setup_u32(void) {
    memset(storage_u32, 0, sizeof(storage_u32));
    mring_init(&ring, storage_u32, BUF_CAP, sizeof(uint32_t));
}

static void setup_u8(void) {
    memset(storage_u8, 0, sizeof(storage_u8));
    mring_init(&ring, storage_u8, BUF_CAP, sizeof(uint8_t));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Init
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_init) {
    setup_u32();
    ASSERT_EQ(0, (int)mring_count(&ring));
    ASSERT_EQ(BUF_CAP, (int)mring_capacity(&ring));
    ASSERT_EQ(BUF_CAP, (int)mring_free(&ring));
    ASSERT_TRUE(mring_is_empty(&ring));
    ASSERT_FALSE(mring_is_full(&ring));
}

TEST(test_init_null) {
    ASSERT_EQ(MRING_ERR_NULL, mring_init(NULL, storage_u32, BUF_CAP, 4));
    ASSERT_EQ(MRING_ERR_NULL, mring_init(&ring, NULL, BUF_CAP, 4));
}

TEST(test_init_not_power_of_2) {
    ASSERT_EQ(MRING_ERR_INVALID, mring_init(&ring, storage_u32, 5, 4));
    ASSERT_EQ(MRING_ERR_INVALID, mring_init(&ring, storage_u32, 0, 4));
}

TEST(test_init_zero_elem_size) {
    ASSERT_EQ(MRING_ERR_INVALID, mring_init(&ring, storage_u32, BUF_CAP, 0));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Push and Pop
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_push_pop_single) {
    setup_u32();
    uint32_t val = 42;
    ASSERT_EQ(MRING_OK, mring_push(&ring, &val));
    ASSERT_EQ(1, (int)mring_count(&ring));
    ASSERT_EQ(BUF_CAP - 1, (int)mring_free(&ring));

    uint32_t out = 0;
    ASSERT_EQ(MRING_OK, mring_pop(&ring, &out));
    ASSERT_EQ(42, (int)out);
    ASSERT_EQ(0, (int)mring_count(&ring));
    ASSERT_TRUE(mring_is_empty(&ring));
}

TEST(test_push_pop_fifo_order) {
    setup_u32();
    for (uint32_t i = 0; i < 5; i++) {
        mring_push(&ring, &i);
    }
    ASSERT_EQ(5, (int)mring_count(&ring));

    for (uint32_t i = 0; i < 5; i++) {
        uint32_t out;
        mring_pop(&ring, &out);
        ASSERT_EQ((int)i, (int)out);
    }
}

TEST(test_push_full) {
    setup_u32();
    uint32_t val = 99;
    for (int i = 0; i < BUF_CAP; i++) {
        ASSERT_EQ(MRING_OK, mring_push(&ring, &val));
    }
    ASSERT_TRUE(mring_is_full(&ring));
    ASSERT_EQ(MRING_ERR_FULL, mring_push(&ring, &val));
}

TEST(test_pop_empty) {
    setup_u32();
    uint32_t out;
    ASSERT_EQ(MRING_ERR_EMPTY, mring_pop(&ring, &out));
}

TEST(test_pop_discard) {
    setup_u32();
    uint32_t val = 7;
    mring_push(&ring, &val);
    /* Pop with NULL destination — discard */
    ASSERT_EQ(MRING_OK, mring_pop(&ring, NULL));
    ASSERT_TRUE(mring_is_empty(&ring));
}

TEST(test_push_pop_null) {
    setup_u32();
    ASSERT_EQ(MRING_ERR_NULL, mring_push(NULL, &(uint32_t){0}));
    ASSERT_EQ(MRING_ERR_NULL, mring_push(&ring, NULL));
    ASSERT_EQ(MRING_ERR_NULL, mring_pop(NULL, &(uint32_t){0}));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Wraparound
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_wraparound) {
    setup_u32();
    /* Fill and drain multiple times to force wraparound */
    for (int cycle = 0; cycle < 4; cycle++) {
        for (uint32_t i = 0; i < BUF_CAP; i++) {
            uint32_t val = (uint32_t)(cycle * 100 + i);
            ASSERT_EQ(MRING_OK, mring_push(&ring, &val));
        }
        ASSERT_TRUE(mring_is_full(&ring));

        for (uint32_t i = 0; i < BUF_CAP; i++) {
            uint32_t out;
            ASSERT_EQ(MRING_OK, mring_pop(&ring, &out));
            ASSERT_EQ((int)(cycle * 100 + i), (int)out);
        }
        ASSERT_TRUE(mring_is_empty(&ring));
    }
}

TEST(test_interleaved_push_pop) {
    setup_u32();
    /* Push 3, pop 2, push 3, pop 2 ... forces index to wrap */
    uint32_t push_val = 0;
    uint32_t expected = 0;

    for (int round = 0; round < 10; round++) {
        for (int i = 0; i < 3; i++) {
            mring_push(&ring, &push_val);
            push_val++;
        }
        for (int i = 0; i < 2; i++) {
            uint32_t out;
            mring_pop(&ring, &out);
            ASSERT_EQ((int)expected, (int)out);
            expected++;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Peek
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_peek) {
    setup_u32();
    uint32_t val = 55;
    mring_push(&ring, &val);

    uint32_t out = 0;
    ASSERT_EQ(MRING_OK, mring_peek(&ring, &out));
    ASSERT_EQ(55, (int)out);
    /* Still there */
    ASSERT_EQ(1, (int)mring_count(&ring));
}

TEST(test_peek_empty) {
    setup_u32();
    uint32_t out;
    ASSERT_EQ(MRING_ERR_EMPTY, mring_peek(&ring, &out));
}

TEST(test_peek_at) {
    setup_u32();
    for (uint32_t i = 0; i < 5; i++) {
        mring_push(&ring, &i);
    }

    uint32_t out;
    ASSERT_EQ(MRING_OK, mring_peek_at(&ring, 0, &out));
    ASSERT_EQ(0, (int)out);
    ASSERT_EQ(MRING_OK, mring_peek_at(&ring, 2, &out));
    ASSERT_EQ(2, (int)out);
    ASSERT_EQ(MRING_OK, mring_peek_at(&ring, 4, &out));
    ASSERT_EQ(4, (int)out);
}

TEST(test_peek_at_out_of_range) {
    setup_u32();
    uint32_t val = 1;
    mring_push(&ring, &val);

    uint32_t out;
    ASSERT_EQ(MRING_ERR_SIZE, mring_peek_at(&ring, 1, &out));
    ASSERT_EQ(MRING_ERR_SIZE, mring_peek_at(&ring, 99, &out));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Batch operations
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_push_many) {
    setup_u32();
    uint32_t data[] = { 10, 20, 30, 40, 50 };
    uint32_t pushed = mring_push_many(&ring, data, 5);
    ASSERT_EQ(5, (int)pushed);
    ASSERT_EQ(5, (int)mring_count(&ring));
}

TEST(test_push_many_partial) {
    setup_u32();
    /* Fill with 6 first */
    uint32_t fill[6] = {0};
    mring_push_many(&ring, fill, 6);
    ASSERT_EQ(6, (int)mring_count(&ring));

    /* Only 2 slots free */
    uint32_t data[] = { 100, 200, 300 };
    uint32_t pushed = mring_push_many(&ring, data, 3);
    ASSERT_EQ(2, (int)pushed);
    ASSERT_TRUE(mring_is_full(&ring));
}

TEST(test_pop_many) {
    setup_u32();
    for (uint32_t i = 0; i < 5; i++) {
        mring_push(&ring, &i);
    }

    uint32_t out[5] = {0};
    uint32_t popped = mring_pop_many(&ring, out, 5);
    ASSERT_EQ(5, (int)popped);
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(i, (int)out[i]);
    }
    ASSERT_TRUE(mring_is_empty(&ring));
}

TEST(test_pop_many_partial) {
    setup_u32();
    uint32_t val = 7;
    mring_push(&ring, &val);
    mring_push(&ring, &val);

    uint32_t out[4] = {0};
    uint32_t popped = mring_pop_many(&ring, out, 4);
    ASSERT_EQ(2, (int)popped);
}

TEST(test_pop_many_discard) {
    setup_u32();
    for (uint32_t i = 0; i < 5; i++) {
        mring_push(&ring, &i);
    }
    uint32_t popped = mring_pop_many(&ring, NULL, 3);
    ASSERT_EQ(3, (int)popped);
    ASSERT_EQ(2, (int)mring_count(&ring));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Overwrite mode
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_push_overwrite_not_full) {
    setup_u32();
    uint32_t val = 42;
    ASSERT_EQ(MRING_OK, mring_push_overwrite(&ring, &val));
    ASSERT_EQ(1, (int)mring_count(&ring));
}

TEST(test_push_overwrite_full) {
    setup_u32();
    /* Fill with 0..7 */
    for (uint32_t i = 0; i < BUF_CAP; i++) {
        mring_push(&ring, &i);
    }
    ASSERT_TRUE(mring_is_full(&ring));

    /* Overwrite with 99 — should drop oldest (0) */
    uint32_t val = 99;
    ASSERT_EQ(MRING_OK, mring_push_overwrite(&ring, &val));
    ASSERT_EQ(BUF_CAP, (int)mring_count(&ring));  /* still full */

    /* Front should now be 1 (0 was dropped) */
    uint32_t out;
    mring_peek(&ring, &out);
    ASSERT_EQ(1, (int)out);

    /* Pop all, last should be 99 */
    for (int i = 0; i < (int)BUF_CAP - 1; i++) {
        mring_pop(&ring, &out);
    }
    mring_pop(&ring, &out);
    ASSERT_EQ(99, (int)out);
}

TEST(test_push_overwrite_continuous) {
    setup_u32();
    /* Write 20 values into a size-8 ring with overwrite */
    for (uint32_t i = 0; i < 20; i++) {
        mring_push_overwrite(&ring, &i);
    }
    /* Should contain the last 8: 12, 13, 14, 15, 16, 17, 18, 19 */
    ASSERT_EQ(BUF_CAP, (int)mring_count(&ring));
    for (uint32_t i = 0; i < BUF_CAP; i++) {
        uint32_t out;
        mring_pop(&ring, &out);
        ASSERT_EQ((int)(12 + i), (int)out);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Clear
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_clear) {
    setup_u32();
    for (uint32_t i = 0; i < 5; i++) {
        mring_push(&ring, &i);
    }
    ASSERT_EQ(5, (int)mring_count(&ring));

    ASSERT_EQ(MRING_OK, mring_clear(&ring));
    ASSERT_EQ(0, (int)mring_count(&ring));
    ASSERT_TRUE(mring_is_empty(&ring));
    ASSERT_EQ(BUF_CAP, (int)mring_free(&ring));
}

TEST(test_clear_null) {
    ASSERT_EQ(MRING_ERR_NULL, mring_clear(NULL));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Direct access
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_ptr_at) {
    setup_u32();
    for (uint32_t i = 0; i < 4; i++) {
        mring_push(&ring, &i);
    }

    const uint32_t *p = (const uint32_t *)mring_ptr_at(&ring, 0);
    ASSERT_TRUE(p != NULL);
    ASSERT_EQ(0, (int)*p);

    p = (const uint32_t *)mring_ptr_at(&ring, 3);
    ASSERT_TRUE(p != NULL);
    ASSERT_EQ(3, (int)*p);

    ASSERT_TRUE(mring_ptr_at(&ring, 4) == NULL);
    ASSERT_TRUE(mring_ptr_at(NULL, 0) == NULL);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Typed wrappers
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_typed_u32) {
    setup_u32();
    ASSERT_EQ(MRING_OK, u32ring_push(&ring, 123));
    ASSERT_EQ(MRING_OK, u32ring_push(&ring, 456));

    uint32_t v;
    ASSERT_EQ(MRING_OK, u32ring_peek(&ring, &v));
    ASSERT_EQ(123, (int)v);

    ASSERT_EQ(MRING_OK, u32ring_pop(&ring, &v));
    ASSERT_EQ(123, (int)v);
    ASSERT_EQ(MRING_OK, u32ring_pop(&ring, &v));
    ASSERT_EQ(456, (int)v);
}

TEST(test_typed_u8) {
    setup_u8();
    ASSERT_EQ(MRING_OK, u8ring_push(&ring, 0xAA));
    ASSERT_EQ(MRING_OK, u8ring_push(&ring, 0xBB));

    uint8_t v;
    ASSERT_EQ(MRING_OK, u8ring_pop(&ring, &v));
    ASSERT_EQ(0xAA, (int)v);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Struct elements
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    uint16_t sensor_id;
    float    value;
    uint32_t timestamp;
} sample_t;

TEST(test_struct_elements) {
    static uint8_t sbuf[4 * sizeof(sample_t)];  /* capacity = 4 */
    mring_t sr;
    mring_init(&sr, sbuf, 4, sizeof(sample_t));

    sample_t s1 = { .sensor_id = 1, .value = 23.5f, .timestamp = 1000 };
    sample_t s2 = { .sensor_id = 2, .value = -10.0f, .timestamp = 2000 };

    mring_push(&sr, &s1);
    mring_push(&sr, &s2);
    ASSERT_EQ(2, (int)mring_count(&sr));

    sample_t out;
    mring_pop(&sr, &out);
    ASSERT_EQ(1, out.sensor_id);
    ASSERT_EQ(1000, (int)out.timestamp);

    mring_pop(&sr, &out);
    ASSERT_EQ(2, out.sensor_id);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Null safety and edge cases
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_query_null) {
    ASSERT_EQ(0, (int)mring_count(NULL));
    ASSERT_EQ(0, (int)mring_free(NULL));
    ASSERT_EQ(0, (int)mring_capacity(NULL));
    ASSERT_TRUE(mring_is_empty(NULL));
    ASSERT_TRUE(mring_is_full(NULL));
}

TEST(test_err_str) {
    ASSERT_STR_EQ("ok",            mring_err_str(MRING_OK));
    ASSERT_STR_EQ("buffer full",   mring_err_str(MRING_ERR_FULL));
    ASSERT_STR_EQ("buffer empty",  mring_err_str(MRING_ERR_EMPTY));
    ASSERT_STR_EQ("invalid config",mring_err_str(MRING_ERR_INVALID));
    ASSERT_STR_EQ("unknown error", mring_err_str((mring_err_t)99));
}

TEST(test_capacity_1) {
    uint8_t tiny[sizeof(uint32_t)];
    mring_t tr;
    mring_init(&tr, tiny, 1, sizeof(uint32_t));
    ASSERT_EQ(1, (int)mring_capacity(&tr));

    uint32_t val = 77;
    ASSERT_EQ(MRING_OK, mring_push(&tr, &val));
    ASSERT_TRUE(mring_is_full(&tr));
    ASSERT_EQ(MRING_ERR_FULL, mring_push(&tr, &val));

    uint32_t out;
    ASSERT_EQ(MRING_OK, mring_pop(&tr, &out));
    ASSERT_EQ(77, (int)out);
    ASSERT_TRUE(mring_is_empty(&tr));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Main
 * ═══════════════════════════════════════════════════════════════════════════ */

int main(void) {
    printf("\n=== micoring test suite ===\n\n");

    printf("[Init]\n");
    RUN_TEST(test_init);
    RUN_TEST(test_init_null);
    RUN_TEST(test_init_not_power_of_2);
    RUN_TEST(test_init_zero_elem_size);

    printf("\n[Push & Pop]\n");
    RUN_TEST(test_push_pop_single);
    RUN_TEST(test_push_pop_fifo_order);
    RUN_TEST(test_push_full);
    RUN_TEST(test_pop_empty);
    RUN_TEST(test_pop_discard);
    RUN_TEST(test_push_pop_null);

    printf("\n[Wraparound]\n");
    RUN_TEST(test_wraparound);
    RUN_TEST(test_interleaved_push_pop);

    printf("\n[Peek]\n");
    RUN_TEST(test_peek);
    RUN_TEST(test_peek_empty);
    RUN_TEST(test_peek_at);
    RUN_TEST(test_peek_at_out_of_range);

    printf("\n[Batch Operations]\n");
    RUN_TEST(test_push_many);
    RUN_TEST(test_push_many_partial);
    RUN_TEST(test_pop_many);
    RUN_TEST(test_pop_many_partial);
    RUN_TEST(test_pop_many_discard);

    printf("\n[Overwrite Mode]\n");
    RUN_TEST(test_push_overwrite_not_full);
    RUN_TEST(test_push_overwrite_full);
    RUN_TEST(test_push_overwrite_continuous);

    printf("\n[Clear]\n");
    RUN_TEST(test_clear);
    RUN_TEST(test_clear_null);

    printf("\n[Direct Access]\n");
    RUN_TEST(test_ptr_at);

    printf("\n[Typed Wrappers]\n");
    RUN_TEST(test_typed_u32);
    RUN_TEST(test_typed_u8);

    printf("\n[Struct Elements]\n");
    RUN_TEST(test_struct_elements);

    printf("\n[Edge Cases]\n");
    RUN_TEST(test_query_null);
    RUN_TEST(test_err_str);
    RUN_TEST(test_capacity_1);

    printf("\n=== Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) printf(", %d FAILED", tests_failed);
    printf(" ===\n\n");

    return tests_failed > 0 ? 1 : 0;
}
