#include "mring.h"

MRING_DEFINE_TYPED(shared_prefix, uint32_t)

int wrapper_collision_a(mring_t *ring, uint32_t value)
{
    return shared_prefix_push(ring, value);
}
