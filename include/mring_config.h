/*
 * Generated public configuration header for source-tree builds.
 * Install and build-tree consumers receive the configured variant.
 */

#ifndef MRING_CONFIG_H
#define MRING_CONFIG_H

#include <stdint.h>

#define MRING_CONCURRENCY_SINGLE_CONTEXT 1
#define MRING_CONCURRENCY_SPSC_ATOMIC 2

#if defined(MRING_CONCURRENCY_MODE) && (MRING_CONCURRENCY_MODE != MRING_CONCURRENCY_SINGLE_CONTEXT)
#error "MRING_CONCURRENCY_MODE must come from mring_config.h"
#endif
#undef MRING_CONCURRENCY_MODE
#define MRING_CONCURRENCY_MODE MRING_CONCURRENCY_SINGLE_CONTEXT

#ifndef MRING_HAVE_USER_ATOMICS
#define MRING_HAVE_USER_ATOMICS 0
#endif

#define MRING_STATIC_ASSERT(name, expr) typedef char mring_static_assert_##name[(expr) ? 1 : -1]

#if (MRING_CONCURRENCY_MODE != MRING_CONCURRENCY_SINGLE_CONTEXT) && \
    (MRING_CONCURRENCY_MODE != MRING_CONCURRENCY_SPSC_ATOMIC)
#error "MRING_CONCURRENCY_MODE must be MRING_CONCURRENCY_SINGLE_CONTEXT or MRING_CONCURRENCY_SPSC_ATOMIC"
#endif

#if MRING_HAVE_USER_ATOMICS
#if !defined(MRING_USER_ATOMIC_LOAD_ACQUIRE32) || !defined(MRING_USER_ATOMIC_STORE_RELEASE32)
#error "MRING_HAVE_USER_ATOMICS requires MRING_USER_ATOMIC_LOAD_ACQUIRE32 and MRING_USER_ATOMIC_STORE_RELEASE32"
#endif
#endif

MRING_STATIC_ASSERT(concurrency_mode_width, sizeof(uint32_t) == 4U);

#endif /* MRING_CONFIG_H */
