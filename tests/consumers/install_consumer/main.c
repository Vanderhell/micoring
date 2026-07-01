#include "mring.h"

#include <stdint.h>

int main(void)
{
    mring_t ring;
    uint8_t storage[4U * sizeof(uint32_t)];

    return mring_init(&ring, storage, sizeof(storage), 4U, sizeof(uint32_t)) == MRING_OK ? 0 : 1;
}
