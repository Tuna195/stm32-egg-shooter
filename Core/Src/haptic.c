#include "haptic.h"

#include <stddef.h>

typedef struct
{
    uint8_t intensity_percent;
    uint16_t duration_ms;
} HapticStep;

typedef struct
{
    const HapticStep *steps;
    uint8_t step_count;
} HapticPattern;

#define ARRAY_LENGTH(array) ((uint8_t)(sizeof(array) / sizeof((array)[0])))

static const HapticStep shoot_steps[] = {
    {80U, 40U}
};

static const HapticStep pop_steps[] = {
    {100U, 60U}, {0U, 50U}, {100U, 60U}
};

static const HapticStep warning_steps[] = {
    {70U, 100U}, {0U, 70U}, {70U, 100U}
};

static const HapticStep combo_steps[] = {
    {100U, 45U}, {0U, 40U}, {100U, 45U},
    {0U, 40U}, {100U, 90U}
};

static const HapticStep gameover_steps[] = {
    {100U, 600U}
};

static const HapticStep p1_score_steps[] = {
    {85U, 100U}
};

static const HapticStep p2_score_steps[] = {
    {85U, 70U}, {0U, 60U}, {85U, 70U}
};

static const HapticPattern patterns[HAPTIC_PATTERN_COUNT] = {
    [HAPTIC_NONE]     = {NULL, 0U},
    [HAPTIC_SHOOT]    = {shoot_steps, ARRAY_LENGTH(shoot_steps)},
    [HAPTIC_POP]      = {pop_steps, ARRAY_LENGTH(pop_steps)},
    [HAPTIC_WARNING]  = {warning_steps, ARRAY_LENGTH(warning_steps)},
    [HAPTIC_COMBO]    = {combo_steps, ARRAY_LENGTH(combo_steps)},
    [HAPTIC_GAMEOVER] = {gameover_steps, ARRAY_LENGTH(gameover_steps)},
    [HAPTIC_P1_SCORE] = {p1_score_steps, ARRAY_LENGTH(p1_score_steps)},
    [HAPTIC_P2_SCORE] = {p2_score_steps, ARRAY_LENGTH(p2_score_steps)}
};

static TIM_HandleTypeDef *pwm_timer;
static uint32_t pwm_channel;
static const HapticPattern *active_pattern;
static uint8_t active_step;
static uint32_t remaining_ms;
static volatile bool is_busy;

static void set_intensity(uint8_t intensity_percent)
{
    uint32_t period;
    uint32_t compare;

    if (pwm_timer == NULL)
    {
        return;
    }

    if (intensity_percent > 100U)
    {
        intensity_percent = 100U;
    }

    period = __HAL_TIM_GET_AUTORELOAD(pwm_timer) + 1U;
    compare = (period * intensity_percent) / 100U;
    if (compare > __HAL_TIM_GET_AUTORELOAD(pwm_timer))
    {
        compare = __HAL_TIM_GET_AUTORELOAD(pwm_timer);
    }

    __HAL_TIM_SET_COMPARE(pwm_timer, pwm_channel, compare);
}

HAL_StatusTypeDef Haptic_Init(TIM_HandleTypeDef *timer, uint32_t channel)
{
    HAL_StatusTypeDef status;

    if (timer == NULL)
    {
        return HAL_ERROR;
    }

    pwm_timer = timer;
    pwm_channel = channel;
    active_pattern = NULL;
    active_step = 0U;
    remaining_ms = 0U;
    is_busy = false;

    __HAL_TIM_SET_COMPARE(pwm_timer, pwm_channel, 0U);
    status = HAL_TIM_PWM_Start(pwm_timer, pwm_channel);
    if (status != HAL_OK)
    {
        pwm_timer = NULL;
    }

    return status;
}

void Haptic_Play(HapticPatternId pattern)
{
    if ((pwm_timer == NULL) || (pattern <= HAPTIC_NONE) ||
        (pattern >= HAPTIC_PATTERN_COUNT))
    {
        return;
    }

    active_pattern = &patterns[pattern];
    active_step = 0U;
    remaining_ms = active_pattern->steps[0].duration_ms;
    is_busy = true;
    set_intensity(active_pattern->steps[0].intensity_percent);
}

void Haptic_Update(uint32_t elapsed_ms)
{
    if ((!is_busy) || (elapsed_ms == 0U))
    {
        return;
    }

    while (is_busy && (elapsed_ms >= remaining_ms))
    {
        elapsed_ms -= remaining_ms;
        active_step++;

        if (active_step >= active_pattern->step_count)
        {
            Haptic_Stop();
        }
        else
        {
            remaining_ms = active_pattern->steps[active_step].duration_ms;
            set_intensity(active_pattern->steps[active_step].intensity_percent);
        }
    }

    if (is_busy)
    {
        remaining_ms -= elapsed_ms;
    }
}

void Haptic_Stop(void)
{
    set_intensity(0U);
    active_pattern = NULL;
    active_step = 0U;
    remaining_ms = 0U;
    is_busy = false;
}

bool Haptic_IsBusy(void)
{
    return is_busy;
}
