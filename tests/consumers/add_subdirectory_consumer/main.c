#include "mring.h"

#include <stdint.h>

int main(void)
{
    mring_t ring;
    uint8_t storage[4U * sizeof(uint32_t)];
    uint32_t value = 7U;
    uint32_t out = 0U;

    if (mring_init(&ring, storage, sizeof(storage), 4U, sizeof(uint32_t)) != MRING_OK) {
        return 1;
    }
    if (mring_push(&ring, &value) != MRING_OK) {
        return 1;
    }
    if (mring_pop(&ring, &out) != MRING_OK) {
        return 1;
    }
    return (out == value) ? 0 : 1;
}
