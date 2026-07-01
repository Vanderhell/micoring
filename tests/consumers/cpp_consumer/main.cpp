#include "mring.h"

#include <cstdint>

int main()
{
    mring_t ring;
    std::uint8_t storage[2U * sizeof(std::uint32_t)] = {};
    std::uint32_t value = 9U;
    std::uint32_t out = 0U;

    if (mring_init(&ring, storage, sizeof(storage), 2U, sizeof(std::uint32_t)) != MRING_OK) {
        return 1;
    }
    if (mring_push(&ring, &value) != MRING_OK) {
        return 1;
    }
    if (mring_pop(&ring, &out) != MRING_OK) {
        return 1;
    }
    return out == value ? 0 : 1;
}
