#ifndef HIGH_SCORE_H
#define HIGH_SCORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

#define HIGH_SCORE_FLASH_ADDR   0x081E0000U
#define HIGH_SCORE_FLASH_SECTOR FLASH_SECTOR_23
#define HIGH_SCORE_FLASH_SIZE   (128U * 1024U)

uint32_t HighScore_Load(void);
HAL_StatusTypeDef HighScore_Save(uint32_t score);
bool HighScore_AsyncInit(void);
bool HighScore_RequestSave(uint32_t score);

#ifdef __cplusplus
}
#endif

#endif /* HIGH_SCORE_H */
