#ifndef HAPTIC_H
#define HAPTIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

typedef enum
{
    HAPTIC_NONE = 0,
    HAPTIC_SHOOT,
    HAPTIC_EGG_CLUSTER,
    HAPTIC_WARNING,
    HAPTIC_COMBO,
    HAPTIC_GAMEOVER,
    HAPTIC_P1_SCORE,
    HAPTIC_P2_SCORE,
    HAPTIC_PATTERN_COUNT
} HapticPatternId;

/**
 * @brief Bind the haptic driver to a configured PWM timer channel.
 *
 * CubeMX must configure and initialize the supplied timer/channel first.
 * For this project the intended output is TIM4_CH1 on PD12.
 */
HAL_StatusTypeDef Haptic_Init(TIM_HandleTypeDef *timer, uint32_t channel);

/** Start a pattern and return immediately. A new pattern replaces the old one. */
void Haptic_Play(HapticPatternId pattern);

/**
 * Advance the pattern engine by elapsed_ms.
 * Call periodically from a 1 ms/10 ms task or timer callback.
 */
void Haptic_Update(uint32_t elapsed_ms);

/** Stop the active pattern and set PWM duty cycle to zero. */
void Haptic_Stop(void);

/** Return true while a pattern is being played. */
bool Haptic_IsBusy(void);

#ifdef __cplusplus
}
#endif

#endif /* HAPTIC_H */
