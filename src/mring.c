/*
 * micoring - Implementation.
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/micoring
 */

#include "mring.h"

#include <string.h>

/* Helpers */

/* Check if n is a power of 2 (and non-zero). */
static inline bool is_power_of_2(uint32_t n)
{
    return n > 0 && (n & (n - 1)) == 0;
}

/* Wrap index using bitmask (requires power-of-2 capacity). */
static inline uint32_t wrap(const mring_t *ring, uint32_t index)
{
    return index & (ring->capacity - 1);
}

/* Get pointer to element at raw index. */
static inline void *elem_at(const mring_t *ring, uint32_t index)
{
    return ring->buf + wrap(ring, index) * ring->elem_size;
}

/* Error strings */

const char *mring_err_str(mring_err_t err)
{
    switch (err) {
    case MRING_OK:          return "ok";
    case MRING_ERR_NULL:    return "null pointer";
    case MRING_ERR_FULL:    return "buffer full";
    case MRING_ERR_EMPTY:   return "buffer empty";
    case MRING_ERR_INVALID: return "invalid config";
    case MRING_ERR_SIZE:    return "size error";
    default:                return "unknown error";
    }
}

/* Init */

mring_err_t mring_init(mring_t *ring, void *buf, uint32_t capacity,
                       uint32_t elem_size)
{
    if (ring == NULL || buf == NULL) return MRING_ERR_NULL;
    if (elem_size == 0) return MRING_ERR_INVALID;
    if (!is_power_of_2(capacity)) return MRING_ERR_INVALID;

    ring->buf = (uint8_t *)buf;
    ring->capacity = capacity;
    ring->elem_size = elem_size;
    ring->head = 0;
    ring->tail = 0;

    return MRING_OK;
}

/* Query */

uint32_t mring_count(const mring_t *ring)
{
    if (ring == NULL) return 0;
    return ring->head - ring->tail;
}

uint32_t mring_free(const mring_t *ring)
{
    if (ring == NULL) return 0;
    return ring->capacity - mring_count(ring);
}

bool mring_is_empty(const mring_t *ring)
{
    if (ring == NULL) return true;
    return ring->head == ring->tail;
}

bool mring_is_full(const mring_t *ring)
{
    if (ring == NULL) return true;
    return mring_count(ring) >= ring->capacity;
}

uint32_t mring_capacity(const mring_t *ring)
{
    if (ring == NULL) return 0;
    return ring->capacity;
}

/* Single-element operations */

mring_err_t mring_push(mring_t *ring, const void *elem)
{
    if (ring == NULL || elem == NULL) return MRING_ERR_NULL;
    if (mring_is_full(ring)) return MRING_ERR_FULL;

    memcpy(elem_at(ring, ring->head), elem, ring->elem_size);
    ring->head++;

    return MRING_OK;
}

mring_err_t mring_pop(mring_t *ring, void *elem)
{
    if (ring == NULL) return MRING_ERR_NULL;
    if (mring_is_empty(ring)) return MRING_ERR_EMPTY;

    if (elem != NULL) {
        memcpy(elem, elem_at(ring, ring->tail), ring->elem_size);
    }
    ring->tail++;

    return MRING_OK;
}

mring_err_t mring_peek(const mring_t *ring, void *elem)
{
    if (ring == NULL || elem == NULL) return MRING_ERR_NULL;
    if (mring_is_empty(ring)) return MRING_ERR_EMPTY;

    memcpy(elem, elem_at(ring, ring->tail), ring->elem_size);
    return MRING_OK;
}

mring_err_t mring_peek_at(const mring_t *ring, uint32_t offset, void *elem)
{
    if (ring == NULL || elem == NULL) return MRING_ERR_NULL;
    if (offset >= mring_count(ring)) return MRING_ERR_SIZE;

    memcpy(elem, elem_at(ring, ring->tail + offset), ring->elem_size);
    return MRING_OK;
}

/* Batch operations */

uint32_t mring_push_many(mring_t *ring, const void *elems, uint32_t count)
{
    if (ring == NULL || elems == NULL || count == 0) return 0;

    uint32_t avail = mring_free(ring);
    if (count > avail) count = avail;

    {
        const uint8_t *src = (const uint8_t *)elems;
        uint32_t i;
        for (i = 0; i < count; i++) {
            memcpy(elem_at(ring, ring->head), src + i * ring->elem_size,
                   ring->elem_size);
            ring->head++;
        }
    }

    return count;
}

uint32_t mring_pop_many(mring_t *ring, void *elems, uint32_t count)
{
    if (ring == NULL || count == 0) return 0;

    uint32_t avail = mring_count(ring);
    if (count > avail) count = avail;

    {
        uint8_t *dst = (uint8_t *)elems;
        uint32_t i;
        for (i = 0; i < count; i++) {
            if (dst != NULL) {
                memcpy(dst + i * ring->elem_size, elem_at(ring, ring->tail),
                       ring->elem_size);
            }
            ring->tail++;
        }
    }

    return count;
}

/* Overwrite mode */

mring_err_t mring_push_overwrite(mring_t *ring, const void *elem)
{
    if (ring == NULL || elem == NULL) return MRING_ERR_NULL;

    if (mring_is_full(ring)) {
        /* Advance tail to make room (discard oldest). */
        ring->tail++;
    }

    memcpy(elem_at(ring, ring->head), elem, ring->elem_size);
    ring->head++;

    return MRING_OK;
}

/* Reset */

mring_err_t mring_clear(mring_t *ring)
{
    if (ring == NULL) return MRING_ERR_NULL;
    ring->head = 0;
    ring->tail = 0;
    return MRING_OK;
}

/* Direct access */

const void *mring_ptr_at(const mring_t *ring, uint32_t offset)
{
    if (ring == NULL) return NULL;
    if (offset >= mring_count(ring)) return NULL;
    return elem_at(ring, ring->tail + offset);
}
