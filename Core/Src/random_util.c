#include "main.h"
#include "random_utils.h"

extern RNG_HandleTypeDef hrng;

uint32_t getRandom32(void)
{
    uint32_t val;
    if (HAL_RNG_GenerateRandomNumber(&hrng, &val) != HAL_OK)
    {
        // fallback nếu RNG lỗi
        val = HAL_GetTick();
    }
    return val;
}

uint8_t randomColor(void)
{
    return (getRandom32() % 5) + 1;  // 1 đến 5
}
