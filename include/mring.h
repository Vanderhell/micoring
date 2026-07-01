/*
 * micoring - fixed-capacity ring buffer.
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/micoring
 */

#ifndef MRING_H
#define MRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mring_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef int32_t mring_err_t;

#define MRING_OK ((mring_err_t)0)
#define MRING_ERR_NULL ((mring_err_t)-1)
#define MRING_ERR_FULL ((mring_err_t)-2)
#define MRING_ERR_EMPTY ((mring_err_t)-3)
#define MRING_ERR_INVALID ((mring_err_t)-4)
#define MRING_ERR_SIZE ((mring_err_t)-5)
#define MRING_ERR_TYPE ((mring_err_t)-6)
#define MRING_ERR_UNSUPPORTED ((mring_err_t)-7)
#define MRING_ERR_BUSY ((mring_err_t)-8)

typedef struct {
    uint8_t *storage;
    size_t storage_size;
    size_t elem_size;
    uint32_t capacity;
    uint32_t mask;
    uint32_t head;
    uint32_t tail;
} mring_t;

const char *mring_err_str(mring_err_t err);

mring_err_t mring_init(
    mring_t *ring,
    void *storage,
    size_t storage_size,
    uint32_t capacity,
    size_t elem_size);

mring_err_t mring_capacity(const mring_t *ring, uint32_t *capacity_out);
mring_err_t mring_element_size(const mring_t *ring, size_t *elem_size_out);
mring_err_t mring_count(const mring_t *ring, uint32_t *count_out);
mring_err_t mring_free(const mring_t *ring, uint32_t *free_out);
mring_err_t mring_is_empty(const mring_t *ring, bool *is_empty_out);
mring_err_t mring_is_full(const mring_t *ring, bool *is_full_out);

mring_err_t mring_push(mring_t *ring, const void *element);
mring_err_t mring_pop(mring_t *ring, void *element);
mring_err_t mring_peek(const mring_t *ring, void *element);
mring_err_t mring_peek_at(const mring_t *ring, uint32_t offset, void *element);

mring_err_t mring_push_many(
    mring_t *ring,
    const void *elements,
    size_t requested,
    size_t *pushed);

mring_err_t mring_pop_many(
    mring_t *ring,
    void *elements,
    size_t requested,
    size_t *popped);

mring_err_t mring_push_overwrite(mring_t *ring, const void *element);
mring_err_t mring_clear(mring_t *ring);

#ifdef MRING_TEST
mring_err_t mring_test_set_counters(mring_t *ring, uint32_t head, uint32_t tail);
#endif

#define MRING_DEFINE_TYPED(prefix, type)                                              \
    static inline mring_err_t prefix##_push(mring_t *ring, type value)                \
    {                                                                                 \
        size_t prefix##_elem_size_value = 0U;                                         \
        mring_err_t prefix##_status = mring_element_size(ring, &prefix##_elem_size_value); \
        if (prefix##_status != MRING_OK) {                                            \
            return prefix##_status;                                                   \
        }                                                                             \
        if (prefix##_elem_size_value != sizeof(type)) {                               \
            return MRING_ERR_TYPE;                                                    \
        }                                                                             \
        return mring_push(ring, &value);                                              \
    }                                                                                 \
    static inline mring_err_t prefix##_pop(mring_t *ring, type *value)                \
    {                                                                                 \
        size_t prefix##_elem_size_value = 0U;                                         \
        mring_err_t prefix##_status = mring_element_size(ring, &prefix##_elem_size_value); \
        if (prefix##_status != MRING_OK) {                                            \
            return prefix##_status;                                                   \
        }                                                                             \
        if (prefix##_elem_size_value != sizeof(type)) {                               \
            return MRING_ERR_TYPE;                                                    \
        }                                                                             \
        return mring_pop(ring, value);                                                \
    }                                                                                 \
    static inline mring_err_t prefix##_peek(const mring_t *ring, type *value)         \
    {                                                                                 \
        size_t prefix##_elem_size_value = 0U;                                         \
        mring_err_t prefix##_status = mring_element_size(ring, &prefix##_elem_size_value); \
        if (prefix##_status != MRING_OK) {                                            \
            return prefix##_status;                                                   \
        }                                                                             \
        if (prefix##_elem_size_value != sizeof(type)) {                               \
            return MRING_ERR_TYPE;                                                    \
        }                                                                             \
        return mring_peek(ring, value);                                               \
    }

#ifdef __cplusplus
}
#endif

#endif /* MRING_H */
