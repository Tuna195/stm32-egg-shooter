#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <climits>
#include "cmsis_os.h"
#include "high_score.h"

extern "C" osMessageQueueId_t shootEventQueueHandle;

Model::Model() : modelListener(0)
{
    const uint32_t savedHighScore = HighScore_Load();
    highScore = (savedHighScore > static_cast<uint32_t>(INT_MAX))
                    ? INT_MAX
                    : static_cast<int>(savedHighScore);
}

void Model::setFinalScore(int s)
{
    finalScore = s;

    if (s > highScore)
    {
        highScore = s;
        (void)HighScore_Save(static_cast<uint32_t>(highScore));
    }
}

void Model::tick()
{
    if (shootEventQueueHandle != NULL)
    {
        uint8_t msg;

        if (osMessageQueueGet(shootEventQueueHandle, &msg, NULL, 0) == osOK)
        {
            if (modelListener != nullptr)
            {
                modelListener->shootButtonPressed();
            }
        }
    }
}
