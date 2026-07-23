#include "high_score.h"

#include <stddef.h>

#include "cmsis_os2.h"

#define HIGH_SCORE_MAGIC 0x48534352U
#define HIGH_SCORE_QUEUE_DEPTH 1U
#define HIGH_SCORE_TASK_STACK_SIZE 768U

typedef struct
{
    uint32_t magic;
    uint32_t score;
    uint32_t score_inv;
} HighScoreRecord;

static const volatile HighScoreRecord *const high_score_records =
    (const volatile HighScoreRecord *)HIGH_SCORE_FLASH_ADDR;

#define HIGH_SCORE_RECORD_COUNT \
    (HIGH_SCORE_FLASH_SIZE / sizeof(HighScoreRecord))

static osMessageQueueId_t high_score_queue;
static osThreadId_t high_score_task;

static const osThreadAttr_t high_score_task_attributes = {
    .name = "highScoreTask",
    .stack_size = HIGH_SCORE_TASK_STACK_SIZE,
    .priority = (osPriority_t)osPriorityLow
};

static uint8_t is_record_valid(const volatile HighScoreRecord *record)
{
    return (record->magic == HIGH_SCORE_MAGIC) &&
           (record->score_inv == ~record->score);
}

static uint8_t is_record_erased(const volatile HighScoreRecord *record)
{
    return (record->magic == UINT32_MAX) &&
           (record->score == UINT32_MAX) &&
           (record->score_inv == UINT32_MAX);
}

static uint32_t find_empty_record_address(void)
{
    for (uint32_t i = 0U; i < HIGH_SCORE_RECORD_COUNT; ++i)
    {
        if (is_record_erased(&high_score_records[i]))
        {
            return HIGH_SCORE_FLASH_ADDR + (i * sizeof(HighScoreRecord));
        }
    }

    return 0U;
}

uint32_t HighScore_Load(void)
{
    uint32_t high_score = 0U;

    for (uint32_t i = 0U; i < HIGH_SCORE_RECORD_COUNT; ++i)
    {
        if (is_record_valid(&high_score_records[i]) &&
            (high_score_records[i].score > high_score))
        {
            high_score = high_score_records[i].score;
        }
    }

    return high_score;
}

HAL_StatusTypeDef HighScore_Save(uint32_t score)
{
    FLASH_EraseInitTypeDef erase_init = {0};
    uint32_t sector_error = 0U;
    HAL_StatusTypeDef status;
    uint32_t record_address;

    if (score <= HighScore_Load())
    {
        return HAL_OK;
    }

    record_address = find_empty_record_address();

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK)
    {
        return status;
    }

    /*
     * Append records until the sector is full. This avoids a 128 KiB erase
     * on every new high score and preserves records created by older firmware.
     */
    if (record_address == 0U)
    {
        erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
        erase_init.Sector = HIGH_SCORE_FLASH_SECTOR;
        erase_init.NbSectors = 1U;
        erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;

        status = HAL_FLASHEx_Erase(&erase_init, &sector_error);
        if (status != HAL_OK)
        {
            (void)HAL_FLASH_Lock();
            return status;
        }

        record_address = HIGH_SCORE_FLASH_ADDR;
    }

    status = HAL_FLASH_Program(
        FLASH_TYPEPROGRAM_WORD,
        record_address + offsetof(HighScoreRecord, score),
        score);
    if (status == HAL_OK)
    {
        status = HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_WORD,
            record_address + offsetof(HighScoreRecord, score_inv),
            ~score);
    }
    if (status == HAL_OK)
    {
        /*
         * Commit the magic word last. An interrupted write therefore leaves
         * an invalid record while previous valid records remain readable.
         */
        status = HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_WORD,
            record_address + offsetof(HighScoreRecord, magic),
            HIGH_SCORE_MAGIC);
    }

    (void)HAL_FLASH_Lock();
    return status;
}

static void HighScore_Task(void *argument)
{
    uint32_t requested_score;
    (void)argument;

    for (;;)
    {
        if (osMessageQueueGet(
                high_score_queue,
                &requested_score,
                NULL,
                osWaitForever) == osOK)
        {
            uint32_t newer_score;

            while (osMessageQueueGet(
                       high_score_queue,
                       &newer_score,
                       NULL,
                       0U) == osOK)
            {
                if (newer_score > requested_score)
                {
                    requested_score = newer_score;
                }
            }

            (void)HighScore_Save(requested_score);
        }
    }
}

bool HighScore_AsyncInit(void)
{
    if ((high_score_queue != NULL) && (high_score_task != NULL))
    {
        return true;
    }

    high_score_queue = osMessageQueueNew(
        HIGH_SCORE_QUEUE_DEPTH,
        sizeof(uint32_t),
        NULL);
    if (high_score_queue == NULL)
    {
        return false;
    }

    high_score_task = osThreadNew(
        HighScore_Task,
        NULL,
        &high_score_task_attributes);
    if (high_score_task == NULL)
    {
        (void)osMessageQueueDelete(high_score_queue);
        high_score_queue = NULL;
        return false;
    }

    return true;
}

bool HighScore_RequestSave(uint32_t score)
{
    osStatus_t status;

    if (high_score_queue == NULL)
    {
        return false;
    }

    status = osMessageQueuePut(high_score_queue, &score, 0U, 0U);
    if (status == osErrorResource)
    {
        uint32_t replaced_score;

        if ((osMessageQueueGet(
                 high_score_queue,
                 &replaced_score,
                 NULL,
                 0U) == osOK) &&
            (replaced_score > score))
        {
            score = replaced_score;
        }
        status = osMessageQueuePut(high_score_queue, &score, 0U, 0U);
    }

    return status == osOK;
}
