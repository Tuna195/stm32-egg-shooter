#include "high_score.h"

#define HIGH_SCORE_MAGIC 0x48534352U

typedef struct
{
    uint32_t magic;
    uint32_t score;
    uint32_t score_inv;
} HighScoreRecord;

static const volatile HighScoreRecord *const high_score_record =
    (const volatile HighScoreRecord *)HIGH_SCORE_FLASH_ADDR;

static uint8_t is_record_valid(void)
{
    return (high_score_record->magic == HIGH_SCORE_MAGIC) &&
           (high_score_record->score_inv == ~high_score_record->score);
}

uint32_t HighScore_Load(void)
{
    if (!is_record_valid())
    {
        return 0U;
    }

    return high_score_record->score;
}

HAL_StatusTypeDef HighScore_Save(uint32_t score)
{
    FLASH_EraseInitTypeDef erase_init = {0};
    uint32_t sector_error = 0U;
    HAL_StatusTypeDef status;
    const uint32_t record_words[] = {
        HIGH_SCORE_MAGIC,
        score,
        ~score
    };

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK)
    {
        return status;
    }

    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Sector = HIGH_SCORE_FLASH_SECTOR;
    erase_init.NbSectors = 1U;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    status = HAL_FLASHEx_Erase(&erase_init, &sector_error);
    if (status == HAL_OK)
    {
        for (uint32_t i = 0U; i < (sizeof(record_words) / sizeof(record_words[0])); ++i)
        {
            status = HAL_FLASH_Program(
                FLASH_TYPEPROGRAM_WORD,
                HIGH_SCORE_FLASH_ADDR + (i * sizeof(uint32_t)),
                record_words[i]);
            if (status != HAL_OK)
            {
                break;
            }
        }
    }

    (void)HAL_FLASH_Lock();
    return status;
}
