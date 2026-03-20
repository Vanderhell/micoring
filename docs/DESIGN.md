# Design Rationale

## 1. Power-of-2 Capacity

Decision: capacity must be a power of 2.

Why: index wrapping becomes `index & (capacity - 1)` instead of modulo. That is
cheap on small MCUs and keeps the implementation simple and predictable.

Tradeoff: some requested capacities need to be rounded up.

## 2. Monotonic Head And Tail

Decision: `head` and `tail` are monotonic `uint32_t` counters.

Why: this avoids the classic full-vs-empty ambiguity and makes count tracking
straightforward through unsigned subtraction.

Tradeoff: counters eventually wrap, but unsigned arithmetic keeps the logic
correct for normal operating ranges.

## 3. Volatile Counters For SPSC

Decision: `head` and `tail` are `volatile uint32_t`.

Why: on common embedded targets such as Cortex-M, aligned 32-bit reads and
writes are atomic in practice for this single-writer/single-reader pattern.

Tradeoff: this is not a general-purpose lock-free primitive for arbitrary
desktop multithreading models.

## 4. memcpy-Based Element Access

Decision: elements are copied with `memcpy`.

Why: the library stays generic, alignment-safe, and correct for arbitrary fixed
element types.

Tradeoff: large structs cost more to copy than pointer-based queues.

## 5. Separate Overwrite API

Decision: overwrite behavior is exposed as `mring_push_overwrite`.

Why: standard push stays branch-light and the caller chooses overwrite semantics
explicitly at the call site.

Tradeoff: the API surface is slightly larger.
