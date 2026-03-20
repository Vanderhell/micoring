/*
 * micoring - Generic, ISR-safe ring buffer for embedded systems.
 *
 * C99 | Zero dependencies | Zero allocations | Lock-free SPSC | Portable
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/micoring
 */

#ifndef MRING_H
#define MRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Error codes */

typedef enum {
    MRING_OK          =  0,   /* Success. */
    MRING_ERR_NULL    = -1,   /* NULL pointer argument. */
    MRING_ERR_FULL    = -2,   /* Ring buffer is full. */
    MRING_ERR_EMPTY   = -3,   /* Ring buffer is empty. */
    MRING_ERR_INVALID = -4,   /* Invalid configuration. */
    MRING_ERR_SIZE    = -5,   /* Not enough space / items. */
} mring_err_t;

const char *mring_err_str(mring_err_t err);

/* Ring buffer instance */

/*
 * Generic ring buffer.
 *
 * Stores fixed-size elements in a caller-provided byte array. The capacity
 * MUST be a power of 2 - this enables fast index wrapping via bitmask
 * instead of modulo, and is critical for lock-free SPSC operation.
 *
 * Lock-free guarantee: one producer (push) and one consumer (pop) can
 * operate concurrently without locks, provided head/tail are read/written
 * atomically (which they are on all 32-bit ARM cores as uint32_t).
 *
 * For multi-producer or multi-consumer, wrap calls in a critical section.
 */
typedef struct {
    uint8_t *buf;            /* Storage buffer (caller-provided). */
    uint32_t capacity;       /* Number of elements (must be power of 2). */
    uint32_t elem_size;      /* Size of each element in bytes. */
    volatile uint32_t head;  /* Next write position (producer). */
    volatile uint32_t tail;  /* Next read position (consumer). */
} mring_t;

/* Init */

/*
 * Initialise a ring buffer.
 *
 * @param ring       Instance (caller-allocated).
 * @param buf        Storage buffer (must be at least capacity * elem_size bytes).
 * @param capacity   Number of elements. MUST be a power of 2.
 * @param elem_size  Size of each element in bytes.
 * @return MRING_OK on success, MRING_ERR_INVALID if capacity is not power of 2.
 */
mring_err_t mring_init(mring_t *ring, void *buf, uint32_t capacity,
                       uint32_t elem_size);

/* Query */

/* Number of elements currently stored. */
uint32_t mring_count(const mring_t *ring);

/* Number of free slots. */
uint32_t mring_free(const mring_t *ring);

/* Is the ring empty? */
bool mring_is_empty(const mring_t *ring);

/* Is the ring full? */
bool mring_is_full(const mring_t *ring);

/* Total capacity in elements. */
uint32_t mring_capacity(const mring_t *ring);

/* Single-element operations */

/*
 * Push one element to the back (producer side).
 *
 * @param ring  Ring buffer.
 * @param elem  Pointer to element data (elem_size bytes copied in).
 * @return MRING_OK or MRING_ERR_FULL.
 */
mring_err_t mring_push(mring_t *ring, const void *elem);

/*
 * Pop one element from the front (consumer side).
 *
 * @param ring  Ring buffer.
 * @param elem  Destination buffer (elem_size bytes copied out). May be NULL to discard.
 * @return MRING_OK or MRING_ERR_EMPTY.
 */
mring_err_t mring_pop(mring_t *ring, void *elem);

/*
 * Peek at the front element without removing it.
 *
 * @param ring  Ring buffer.
 * @param elem  Destination buffer (elem_size bytes copied out).
 * @return MRING_OK or MRING_ERR_EMPTY.
 */
mring_err_t mring_peek(const mring_t *ring, void *elem);

/*
 * Peek at an arbitrary element by offset from front.
 *
 * @param ring    Ring buffer.
 * @param offset  0 = front, 1 = second, etc.
 * @param elem    Destination buffer.
 * @return MRING_OK, MRING_ERR_EMPTY, or MRING_ERR_SIZE if offset >= count.
 */
mring_err_t mring_peek_at(const mring_t *ring, uint32_t offset, void *elem);

/* Batch operations */

/*
 * Push multiple elements. Copies as many as fit.
 *
 * @param ring   Ring buffer.
 * @param elems  Array of elements.
 * @param count  Number of elements to push.
 * @return Number of elements actually pushed (0 .. count).
 */
uint32_t mring_push_many(mring_t *ring, const void *elems, uint32_t count);

/*
 * Pop multiple elements. Copies as many as available.
 *
 * @param ring   Ring buffer.
 * @param elems  Destination array (may be NULL to discard).
 * @param count  Maximum elements to pop.
 * @return Number of elements actually popped (0 .. count).
 */
uint32_t mring_pop_many(mring_t *ring, void *elems, uint32_t count);

/* Overwrite mode */

/*
 * Push one element, overwriting the oldest if full.
 *
 * This is useful for "latest N samples" buffers where you always want the
 * most recent data and can afford to lose old entries.
 *
 * @param ring  Ring buffer.
 * @param elem  Pointer to element data.
 * @return MRING_OK always (never fails).
 */
mring_err_t mring_push_overwrite(mring_t *ring, const void *elem);

/* Reset */

/*
 * Clear all elements (reset head and tail).
 * NOT safe to call while a concurrent producer/consumer is active.
 */
mring_err_t mring_clear(mring_t *ring);

/* Direct access (advanced) */

/*
 * Get a direct pointer to the element at the given offset from front.
 * The pointer is valid until the next pop/clear.
 *
 * @param ring    Ring buffer.
 * @param offset  0 = front element.
 * @return Pointer to element, or NULL if offset >= count.
 */
const void *mring_ptr_at(const mring_t *ring, uint32_t offset);

/* Typed wrapper macros */

/*
 * Convenience macros for common element types.
 *
 * Usage:
 *   MRING_DEFINE_TYPED(u8ring, uint8_t)
 *
 * Generates:
 *   u8ring_push(ring, val)
 *   u8ring_pop(ring, &val)
 *   u8ring_peek(ring, &val)
 */
#define MRING_DEFINE_TYPED(prefix, type)                                   \
    static inline mring_err_t prefix##_push(mring_t *r, type val) {        \
        return mring_push(r, &val);                                        \
    }                                                                      \
    static inline mring_err_t prefix##_pop(mring_t *r, type *val) {        \
        return mring_pop(r, val);                                          \
    }                                                                      \
    static inline mring_err_t prefix##_peek(const mring_t *r, type *val) { \
        return mring_peek(r, val);                                         \
    }

#ifdef __cplusplus
}
#endif

#endif /* MRING_H */
