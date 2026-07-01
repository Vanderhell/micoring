/*
 * micoring - implementation.
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/micoring
 */

#include "mring.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#if defined(MRING_TEST)
#include <assert.h>
#endif

MRING_STATIC_ASSERT(err_width, sizeof(mring_err_t) == sizeof(int32_t));

static int mring_is_power_of_two(uint32_t value)
{
    return (value != 0U) && ((value & (value - 1U)) == 0U);
}

static int mring_multiply_size(size_t left, size_t right, size_t *product_out)
{
    if (product_out == NULL) {
        return 0;
    }
    if ((left != 0U) && (right > (SIZE_MAX / left))) {
        return 0;
    }
    *product_out = left * right;
    return 1;
}

static int mring_ranges_overlap(const void *left, size_t left_size, const void *right, size_t right_size)
{
    const uintptr_t left_begin = (uintptr_t)left;
    const uintptr_t left_end = left_begin + left_size;
    const uintptr_t right_begin = (uintptr_t)right;
    const uintptr_t right_end = right_begin + right_size;

    if ((left == NULL) || (right == NULL) || (left_size == 0U) || (right_size == 0U)) {
        return 0;
    }

    if ((left_end < left_begin) || (right_end < right_begin)) {
        return 1;
    }

    return (left_begin < right_end) && (right_begin < left_end);
}

static size_t mring_slot_offset(const mring_t *ring, uint32_t counter)
{
    return ((size_t)(counter & ring->mask)) * ring->elem_size;
}

static void *mring_slot_ptr(const mring_t *ring, uint32_t counter)
{
    return ring->storage + mring_slot_offset(ring, counter);
}

#if MRING_CONCURRENCY_MODE == MRING_CONCURRENCY_SPSC_ATOMIC
static uint32_t mring_atomic_load_acquire32(const uint32_t *value)
{
#if MRING_HAVE_USER_ATOMICS
    return MRING_USER_ATOMIC_LOAD_ACQUIRE32(value);
#elif defined(MRING_TEST_FORCE_UNSUPPORTED_ATOMICS)
#error "MRING SPSC atomic mode requires GCC/Clang, MSVC, or user atomic hooks"
#elif defined(__clang__) || defined(__GNUC__)
    return __atomic_load_n(value, __ATOMIC_ACQUIRE);
#elif defined(_MSC_VER)
    _ReadBarrier();
    return (uint32_t)_InterlockedCompareExchange((volatile long *)value, 0L, 0L);
#else
#error "MRING SPSC atomic mode requires GCC/Clang, MSVC, or user atomic hooks"
#endif
}

static void mring_atomic_store_release32(uint32_t *value, uint32_t next_value)
{
#if MRING_HAVE_USER_ATOMICS
    MRING_USER_ATOMIC_STORE_RELEASE32(value, next_value);
#elif defined(MRING_TEST_FORCE_UNSUPPORTED_ATOMICS)
#error "MRING SPSC atomic mode requires GCC/Clang, MSVC, or user atomic hooks"
#elif defined(__clang__) || defined(__GNUC__)
    __atomic_store_n(value, next_value, __ATOMIC_RELEASE);
#elif defined(_MSC_VER)
    _WriteBarrier();
    _InterlockedExchange((volatile long *)value, (long)next_value);
#else
#error "MRING SPSC atomic mode requires GCC/Clang, MSVC, or user atomic hooks"
#endif
}
#endif

static uint32_t mring_load_head_snapshot(const mring_t *ring)
{
#if MRING_CONCURRENCY_MODE == MRING_CONCURRENCY_SPSC_ATOMIC
    return mring_atomic_load_acquire32(&ring->head);
#else
    return ring->head;
#endif
}

static uint32_t mring_load_tail_snapshot(const mring_t *ring)
{
#if MRING_CONCURRENCY_MODE == MRING_CONCURRENCY_SPSC_ATOMIC
    return mring_atomic_load_acquire32(&ring->tail);
#else
    return ring->tail;
#endif
}

static void mring_store_head_publish(mring_t *ring, uint32_t value)
{
#if MRING_CONCURRENCY_MODE == MRING_CONCURRENCY_SPSC_ATOMIC
    mring_atomic_store_release32(&ring->head, value);
#else
    ring->head = value;
#endif
}

static void mring_store_tail_publish(mring_t *ring, uint32_t value)
{
#if MRING_CONCURRENCY_MODE == MRING_CONCURRENCY_SPSC_ATOMIC
    mring_atomic_store_release32(&ring->tail, value);
#else
    ring->tail = value;
#endif
}

static uint32_t mring_producer_head(const mring_t *ring)
{
    return ring->head;
}

static uint32_t mring_consumer_tail(const mring_t *ring)
{
    return ring->tail;
}

static uint32_t mring_count_from(uint32_t head, uint32_t tail)
{
    return head - tail;
}

static int mring_snapshot_count(const mring_t *ring, uint32_t *count_out)
{
    uint32_t head = 0U;
    uint32_t tail = 0U;

    if ((ring == NULL) || (count_out == NULL)) {
        return 0;
    }

    head = mring_load_head_snapshot(ring);
    tail = mring_load_tail_snapshot(ring);
    *count_out = mring_count_from(head, tail);
    return 1;
}

#if defined(MRING_TEST)
static void mring_assert_invariants(const mring_t *ring)
{
    uint32_t count = 0U;

    if (ring == NULL) {
        return;
    }

    count = mring_count_from(ring->head, ring->tail);
    assert(count <= ring->capacity);
}
#else
static void mring_assert_invariants(const mring_t *ring)
{
    (void)ring;
}
#endif

const char *mring_err_str(mring_err_t err)
{
    switch (err) {
    case MRING_OK:
        return "ok";
    case MRING_ERR_NULL:
        return "null argument";
    case MRING_ERR_FULL:
        return "ring full";
    case MRING_ERR_EMPTY:
        return "ring empty";
    case MRING_ERR_INVALID:
        return "invalid argument";
    case MRING_ERR_SIZE:
        return "size error";
    case MRING_ERR_TYPE:
        return "element type mismatch";
    case MRING_ERR_UNSUPPORTED:
        return "unsupported operation";
    case MRING_ERR_BUSY:
        return "busy";
    default:
        return "unknown error";
    }
}

mring_err_t mring_init(
    mring_t *ring,
    void *storage,
    size_t storage_size,
    uint32_t capacity,
    size_t elem_size)
{
    mring_t candidate;
    size_t required_size = 0U;

    if ((ring == NULL) || (storage == NULL)) {
        return MRING_ERR_NULL;
    }
    if ((capacity == 0U) || !mring_is_power_of_two(capacity)) {
        return MRING_ERR_INVALID;
    }
    if (elem_size == 0U) {
        return MRING_ERR_SIZE;
    }
    if (capacity > 0x80000000U) {
        return MRING_ERR_SIZE;
    }
    if (!mring_multiply_size((size_t)capacity, elem_size, &required_size)) {
        return MRING_ERR_SIZE;
    }
    if (storage_size < required_size) {
        return MRING_ERR_SIZE;
    }
    if (mring_ranges_overlap(ring, sizeof(*ring), storage, required_size)) {
        return MRING_ERR_INVALID;
    }

    candidate.storage = (uint8_t *)storage;
    candidate.storage_size = required_size;
    candidate.elem_size = elem_size;
    candidate.capacity = capacity;
    candidate.mask = capacity - 1U;
    candidate.head = 0U;
    candidate.tail = 0U;

    *ring = candidate;
    mring_assert_invariants(ring);
    return MRING_OK;
}

mring_err_t mring_capacity(const mring_t *ring, uint32_t *capacity_out)
{
    if ((ring == NULL) || (capacity_out == NULL)) {
        return MRING_ERR_NULL;
    }
    *capacity_out = ring->capacity;
    return MRING_OK;
}

mring_err_t mring_element_size(const mring_t *ring, size_t *elem_size_out)
{
    if ((ring == NULL) || (elem_size_out == NULL)) {
        return MRING_ERR_NULL;
    }
    *elem_size_out = ring->elem_size;
    return MRING_OK;
}

mring_err_t mring_count(const mring_t *ring, uint32_t *count_out)
{
    if (!mring_snapshot_count(ring, count_out)) {
        return MRING_ERR_NULL;
    }
    return MRING_OK;
}

mring_err_t mring_free(const mring_t *ring, uint32_t *free_out)
{
    uint32_t count = 0U;

    if ((ring == NULL) || (free_out == NULL)) {
        return MRING_ERR_NULL;
    }
    if (!mring_snapshot_count(ring, &count)) {
        return MRING_ERR_NULL;
    }
    *free_out = ring->capacity - count;
    return MRING_OK;
}

mring_err_t mring_is_empty(const mring_t *ring, bool *is_empty_out)
{
    uint32_t count = 0U;

    if ((ring == NULL) || (is_empty_out == NULL)) {
        return MRING_ERR_NULL;
    }
    if (!mring_snapshot_count(ring, &count)) {
        return MRING_ERR_NULL;
    }
    *is_empty_out = (count == 0U);
    return MRING_OK;
}

mring_err_t mring_is_full(const mring_t *ring, bool *is_full_out)
{
    uint32_t count = 0U;

    if ((ring == NULL) || (is_full_out == NULL)) {
        return MRING_ERR_NULL;
    }
    if (!mring_snapshot_count(ring, &count)) {
        return MRING_ERR_NULL;
    }
    *is_full_out = (count == ring->capacity);
    return MRING_OK;
}

mring_err_t mring_push(mring_t *ring, const void *element)
{
    uint32_t head = 0U;
    uint32_t tail = 0U;

    if ((ring == NULL) || (element == NULL)) {
        return MRING_ERR_NULL;
    }

    head = mring_producer_head(ring);
    tail = mring_load_tail_snapshot(ring);
    if (mring_count_from(head, tail) == ring->capacity) {
        return MRING_ERR_FULL;
    }

    memmove(mring_slot_ptr(ring, head), element, ring->elem_size);
    mring_store_head_publish(ring, head + 1U);
    mring_assert_invariants(ring);
    return MRING_OK;
}

mring_err_t mring_pop(mring_t *ring, void *element)
{
    uint32_t head = 0U;
    uint32_t tail = 0U;

    if (ring == NULL) {
        return MRING_ERR_NULL;
    }

    tail = mring_consumer_tail(ring);
    head = mring_load_head_snapshot(ring);
    if (head == tail) {
        return MRING_ERR_EMPTY;
    }

    if (element != NULL) {
        memmove(element, mring_slot_ptr(ring, tail), ring->elem_size);
    }
    mring_store_tail_publish(ring, tail + 1U);
    mring_assert_invariants(ring);
    return MRING_OK;
}

mring_err_t mring_peek(const mring_t *ring, void *element)
{
    uint32_t head = 0U;
    uint32_t tail = 0U;

    if ((ring == NULL) || (element == NULL)) {
        return MRING_ERR_NULL;
    }

    tail = mring_consumer_tail(ring);
    head = mring_load_head_snapshot(ring);
    if (head == tail) {
        return MRING_ERR_EMPTY;
    }

    memmove(element, mring_slot_ptr(ring, tail), ring->elem_size);
    return MRING_OK;
}

mring_err_t mring_peek_at(const mring_t *ring, uint32_t offset, void *element)
{
    uint32_t head = 0U;
    uint32_t tail = 0U;
    uint32_t count = 0U;

    if ((ring == NULL) || (element == NULL)) {
        return MRING_ERR_NULL;
    }

    tail = mring_consumer_tail(ring);
    head = mring_load_head_snapshot(ring);
    count = mring_count_from(head, tail);
    if (count == 0U) {
        return MRING_ERR_EMPTY;
    }
    if (offset >= count) {
        return MRING_ERR_SIZE;
    }

    memmove(element, mring_slot_ptr(ring, tail + offset), ring->elem_size);
    return MRING_OK;
}

mring_err_t mring_push_many(
    mring_t *ring,
    const void *elements,
    size_t requested,
    size_t *pushed)
{
    const uint8_t *cursor = (const uint8_t *)elements;
    size_t bytes = 0U;
    size_t index = 0U;

    if ((ring == NULL) || (pushed == NULL)) {
        return MRING_ERR_NULL;
    }
    *pushed = 0U;
    if (requested == 0U) {
        return MRING_OK;
    }
    if (elements == NULL) {
        return MRING_ERR_NULL;
    }
    if (!mring_multiply_size(requested, ring->elem_size, &bytes)) {
        return MRING_ERR_SIZE;
    }
    if (mring_ranges_overlap(elements, bytes, ring->storage, ring->storage_size)) {
        return MRING_ERR_INVALID;
    }

    for (index = 0U; index < requested; ++index) {
        mring_err_t status = mring_push(ring, cursor + (index * ring->elem_size));
        if (status == MRING_ERR_FULL) {
            return MRING_OK;
        }
        if (status != MRING_OK) {
            return status;
        }
        *pushed = *pushed + 1U;
    }

    return MRING_OK;
}

mring_err_t mring_pop_many(
    mring_t *ring,
    void *elements,
    size_t requested,
    size_t *popped)
{
    uint8_t *cursor = (uint8_t *)elements;
    size_t bytes = 0U;
    size_t index = 0U;

    if ((ring == NULL) || (popped == NULL)) {
        return MRING_ERR_NULL;
    }
    *popped = 0U;
    if (requested == 0U) {
        return MRING_OK;
    }
    if ((elements != NULL) && !mring_multiply_size(requested, ring->elem_size, &bytes)) {
        return MRING_ERR_SIZE;
    }
    if ((elements != NULL) && mring_ranges_overlap(elements, bytes, ring->storage, ring->storage_size)) {
        return MRING_ERR_INVALID;
    }

    for (index = 0U; index < requested; ++index) {
        void *destination = (cursor == NULL) ? NULL : (void *)(cursor + (index * ring->elem_size));
        mring_err_t status = mring_pop(ring, destination);
        if (status == MRING_ERR_EMPTY) {
            return MRING_OK;
        }
        if (status != MRING_OK) {
            return status;
        }
        *popped = *popped + 1U;
    }

    return MRING_OK;
}

mring_err_t mring_push_overwrite(mring_t *ring, const void *element)
{
    uint32_t head = 0U;
    uint32_t tail = 0U;

    if ((ring == NULL) || (element == NULL)) {
        return MRING_ERR_NULL;
    }

#if MRING_CONCURRENCY_MODE == MRING_CONCURRENCY_SPSC_ATOMIC
    return MRING_ERR_UNSUPPORTED;
#else
    head = ring->head;
    tail = ring->tail;
    if (mring_count_from(head, tail) == ring->capacity) {
        ring->tail = tail + 1U;
    }
    memmove(mring_slot_ptr(ring, head), element, ring->elem_size);
    ring->head = head + 1U;
    mring_assert_invariants(ring);
    return MRING_OK;
#endif
}

mring_err_t mring_clear(mring_t *ring)
{
    if (ring == NULL) {
        return MRING_ERR_NULL;
    }
    ring->head = 0U;
    ring->tail = 0U;
    mring_assert_invariants(ring);
    return MRING_OK;
}

#ifdef MRING_TEST
mring_err_t mring_test_set_counters(mring_t *ring, uint32_t head, uint32_t tail)
{
    if (ring == NULL) {
        return MRING_ERR_NULL;
    }
    if ((head - tail) > ring->capacity) {
        return MRING_ERR_INVALID;
    }
    ring->head = head;
    ring->tail = tail;
    mring_assert_invariants(ring);
    return MRING_OK;
}
#endif
