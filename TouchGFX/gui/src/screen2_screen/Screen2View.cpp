 #include <gui/screen2_screen/Screen2View.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Unicode.hpp>
#include "main.h"
#include "haptic.h"
#include <cmath>
#include <cstdio>   // để dùng snprintf
#include <cstring>  // để dùng strlen
#include <vector>       // std::vector
#include <utility>      // std::pair
#include <functional>   // std::function
template <typename T>
T clamp(T value, T minVal, T maxVal)
{
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

Screen2View::Screen2View() : score(0), shotCount(0), gridPhase(0), droppedLineCount(0),
                           targetAngle(0), currentAngleFloat(0),
                           isAiming(false), lastAimTime(0)
{
    dangerLine.setPosition(0, DANGER_LINE_Y, 240, 2);
    dangerLine.setColor(touchgfx::Color::getColorFromRGB(255, 48, 48));
    add(dangerLine);

    // Khởi tạo mảng eggGrid
    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            eggGrid[i][j] = EMPTY;
        }
    }

    activeFallingEggs = 0;
    for (int i = 0; i < MAX_FALLING_EGGS; ++i)
    {
        fallingEggActive[i] = false;
        fallingEggY[i] = 0.0f;
        fallingEggVelocity[i] = 0.0f;
    }

    activePopEggs = 0;
    for (int i = 0; i < MAX_POP_EGGS; ++i)
    {
        popEggActive[i] = false;
        popEggStartTick[i] = 0U;
        popEggStartX[i] = 0;
        popEggStartY[i] = 0;
    }
}

// CÁCH 2: DI CHUYỂN WIDGET ĐÚNG CÁCH
void Screen2View::setupScreen()
{
	Screen2ViewBase::setupScreen();

        // Joystick SW (PC2) is active-low; the blue USER button (PA0) is
        // active-high on the STM32F429I-DISCO board.
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();

        GPIO_InitTypeDef buttonConfig = {0};
        buttonConfig.Pin = GPIO_PIN_2;
        buttonConfig.Mode = GPIO_MODE_INPUT;
        buttonConfig.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(GPIOC, &buttonConfig);

        buttonConfig.Pin = GPIO_PIN_0;
        buttonConfig.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &buttonConfig);

        lastButtonPressed =
            (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2) == GPIO_PIN_RESET) ||
            (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
        lastShotButtonTick = HAL_GetTick();

	    // Reset joystick calibration khi vào màn hình
	    resetJoystickCalibration();

	    // Force invalidate toàn bộ widget trước
	    nextEgg.invalidateContent();

	    // Khởi tạo game (code cũ)
	    for(int row = 0; row < ROWS; row++) {
	        for(int col = 0; col < COLS; col++) {
	            eggImages[row][col].setVisible(false);
	            container2.add(eggImages[row][col]);
	        }
	    }

        activeFallingEggs = 0;
        for (int i = 0; i < MAX_FALLING_EGGS; ++i)
        {
            fallingEggActive[i] = false;
            fallingEggImages[i].setVisible(false);
            container2.add(fallingEggImages[i]);
        }

        activePopEggs = 0;
        for (int i = 0; i < MAX_POP_EGGS; ++i)
        {
            popEggActive[i] = false;
            popEggLeftImages[i].setAlpha(255U);
            popEggRightImages[i].setAlpha(255U);
            popEggLeftImages[i].setVisible(false);
            popEggRightImages[i].setVisible(false);
            container2.add(popEggLeftImages[i]);
            container2.add(popEggRightImages[i]);
        }

	    shotCount = 0;
	    gridPhase = 0;
	    droppedLineCount = 0;
	    aimAngle = 0;
	    projectileActive = false;
	    pendingGroupClear = false;

	    initEggGrid();
	    renderEggGrid();
	    spawnNextEgg();
	    score = 0;
	    updateScore(0);  // Hiển thị điểm ban đầu = 0

	    updateCannonAndEggPosition();
	    // Cấu hình hình ảnh viên đạn
	    projectileImage.setBitmap(BITMAP_EGG_BLUE_ID); // Dùng ảnh đạn, bạn có thể chọn ảnh khác
	    projectileImage.setXY(0, 0); // Ban đầu để ngoài màn hình
	    projectileImage.setVisible(false);
	    add(projectileImage); // Cho nó hiển thị lên màn hình
	    // THÊM: Khởi tạo hệ thống ngắm
	        targetAngle = 0;
	        currentAngleFloat = 0;
	        aimAngle = 0;
	        isAiming = false;
	        lastAimTime = HAL_GetTick();

}
void Screen2View::tearDownScreen()
{
    clearNextEgg();
    clearEggGrid();
    Screen2ViewBase::tearDownScreen();
}

void Screen2View::initEggGrid()
{
    // Xóa toàn bộ grid trước
    for(int row = 0; row < ROWS; row++)
    {
        for(int col = 0; col < COLS; col++)
        {
            eggGrid[row][col] = EMPTY;
        }
    }


    for(int row = 0; row < 2; row++) // 3 hàng đầu
    {
        // Hàng chẵn có 8 quả; hàng lẻ có 7 quả và lệch nửa ô.
        int maxCols = COLS - (isShiftedRow(row) ? 1 : 0);

        // Căn giữa các trứng trong hàng
        int startCol = (COLS - maxCols) / 2;

        for(int col = 0; col < maxCols; col++)
        {
            int actualCol = startCol + col;
            if(actualCol >= 0 && actualCol < COLS)
            {
                eggGrid[row][actualCol] = randomColor();;
            }
        }
    }

    // Tạo một số pattern có thể match được dễ dàng
    ensureMatchablePattern();
}

void Screen2View::ensureMatchablePattern()
{
    // Tạo một vài nhóm 2 quả cùng màu để dễ match
    for(int row = 0; row < 3; row++)
    {
        for(int col = 0; col < COLS - 1; col++)
        {
            if(eggGrid[row][col] != EMPTY && eggGrid[row][col + 1] != EMPTY)
            {
                // 30% cơ hội tạo cặp cùng màu
                uint32_t chance = (getRandom32() + row * col) % 100;
                if(chance < 30)
                {
                    eggGrid[row][col + 1] = eggGrid[row][col];
                }
            }
        }
    }
}

void Screen2View::renderEggGrid()
{
    // eggImages là con của container2 nên dùng tọa độ cục bộ của container.
    const int baseX = 0;
    const int baseY = 0;

    // Kích thước trứng và khoảng cách
    const int EGG_WIDTH = 32;
    const int EGG_HEIGHT = 32;
    const int EGG_SPACING_X = 30; // Trứng tròn chạm sát nhau theo chiều ngang
    const int EGG_SPACING_Y = 26; // Khoảng cách chuẩn gần đúng của lưới tổ ong
    const int HEX_OFFSET = 15;    // Nửa bước ngang cho hàng lẻ

    for(int row = 0; row < ROWS; row++)
    {
        for(int col = 0; col < COLS; col++)
        {
            if (!isValidGridPosition(row, col))
            {
                eggImages[row][col].setVisible(false);
                continue;
            }

            if(eggGrid[row][col] != EMPTY)
            {
                // Chọn bitmap theo màu
                switch(eggGrid[row][col])
                {
                    case RED:    eggImages[row][col].setBitmap(BITMAP_EGG_RED_ID); break;
                    case BLUE:   eggImages[row][col].setBitmap(BITMAP_EGG_BLUE_ID); break;
                    case GREEN:  eggImages[row][col].setBitmap(BITMAP_EGG_GREEN_ID); break;
                    case YELLOW: eggImages[row][col].setBitmap(BITMAP_EGG_YELLOW_ID); break;
                    case PURPLE: eggImages[row][col].setBitmap(BITMAP_EGG_PURPLE_ID); break;
                }

                // Tính toán vị trí hexagonal pattern
                int xOffset = isShiftedRow(row) ? HEX_OFFSET : 0;
                int x = baseX + col * EGG_SPACING_X + xOffset;
                int y = baseY + row * EGG_SPACING_Y;

                eggImages[row][col].setXY(x, y);
                eggImages[row][col].setWidthHeight(EGG_WIDTH, EGG_HEIGHT);
                eggImages[row][col].setVisible(true);
            }
            else
            {
                eggImages[row][col].setVisible(false);
            }
        }
    }

    // Invalidate toàn bộ view
    container2.invalidate();
}
// Function được gọi mỗi khi người chơi bắn trứng
void Screen2View::onEggShot()
{
    shotCount++;

    // Sau mỗi SHOTS_BEFORE_DROP lần bắn, hạ lưới xuống
    if(shotCount >= SHOTS_BEFORE_DROP)
    {
        dropEggGrid();
        shotCount = 0; // Reset counter

        if (hasEggCrossedDangerLine())
        {
            triggerGameOver();
            return;
        }
    }

    // Spawn trứng mới cho lần bắn tiếp theo
    spawnNextEgg();
}


/** Trả về true nếu mép dưới của bất kỳ trứng nào chạm danger line. */
bool Screen2View::hasEggCrossedDangerLine() const
{
    const int gridTopY = container2.getY();

    for (int row = 0; row < ROWS; ++row)
    {
        const int eggBottomY = gridTopY + row * EGG_SPACING_Y + EGG_HEIGHT;
        if (eggBottomY < DANGER_LINE_Y)
        {
            continue;
        }

        for (int col = 0; col < COLS; ++col)
        {
            if (isValidGridPosition(row, col) && eggGrid[row][col] != EMPTY)
            {
                return true;
            }
        }
    }

    return false;
}

void Screen2View::dropEggGrid()
{
    Haptic_Play(HAPTIC_WARNING);

    // Di chuyển tất cả trứng xuống 1 hàng
    for(int row = ROWS - 1; row > 0; row--)
    {
        for(int col = 0; col < COLS; col++)
        {
            eggGrid[row][col] = eggGrid[row - 1][col];
        }
    }

    // Đảo pha khi hạ lưới để một hàng 8 ô vẫn là hàng 8 ô tại vị trí
    // mới. Nếu không, col cuối sẽ bị xóa âm thầm khi rơi vào hàng 7 ô.
    gridPhase ^= 1;

    // Thêm hàng mới ở trên (có thể random hoặc theo pattern)
    addNewTopRow();

    // Render lại; onEggShot() sẽ kiểm tra danger line sau khi hạ lưới.
    renderEggGrid();
}

void Screen2View::addNewTopRow()
{
    // Thêm hàng mới ở trên theo pha 8/7 ô hiện tại.
    const int topRowColumns = COLS - (isShiftedRow(0) ? 1 : 0);
    for(int col = 0; col < topRowColumns; col++)
    {
        eggGrid[0][col] = randomColor(); // Luôn có trứng ở mỗi ô
    }
    for (int col = topRowColumns; col < COLS; ++col)
    {
        eggGrid[0][col] = EMPTY;
    }
}
/* Hàm dùng chung để đóng game và nhảy màn */
void Screen2View::triggerGameOver()
{
    Haptic_Play(HAPTIC_GAMEOVER);

    // Tắt viên đạn đang bay (nếu có)
    projectileActive = false;
    projectileImage.setVisible(false);
    projectileImage.invalidate();

    const char* gameOverMessage = "GAME OVER - danger line crossed!\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)gameOverMessage,
        strlen(gameOverMessage), 100);

    notifyGameOver();
}
void Screen2View::notifyGameOver()
{
    presenter->handleGameOver();   // Gọi sang presenter
}
bool Screen2View::checkGameOver()
{
    return hasEggCrossedDangerLine();
}


void Screen2View::spawnNextEgg()
{
    // Ẩn trứng cũ trước
    nextEgg.setVisible(false);
    nextEgg.invalidate();

    uint8_t color = randomColor();
    currentEggColor = color; // <== THÊM DÒNG NÀY

    switch(color)
    {
        case RED:    nextEgg.setBitmap(BITMAP_EGG_RED_ID); break;
        case BLUE:   nextEgg.setBitmap(BITMAP_EGG_BLUE_ID); break;
        case GREEN:  nextEgg.setBitmap(BITMAP_EGG_GREEN_ID); break;
        case YELLOW: nextEgg.setBitmap(BITMAP_EGG_YELLOW_ID); break;
        case PURPLE: nextEgg.setBitmap(BITMAP_EGG_PURPLE_ID); break;
    }

    // Hiển thị trứng mới
    nextEgg.setVisible(true);
    nextEgg.invalidate();
}
void Screen2View::updateScore(int destroyedCount)
{
    score += destroyedCount * 10;
    Unicode::snprintf(scoreTextBuffer, 10, "%d", score);
    scoreText.setWildcard(scoreTextBuffer);
    scoreText.invalidate();
}


void Screen2View::clearEggGrid()
{
    for(int row = 0; row < ROWS; row++)
    {
        for(int col = 0; col < COLS; col++)
        {
            eggImages[row][col].setVisible(false);
            eggGrid[row][col] = EMPTY;
        }
    }

    activeFallingEggs = 0;
    for (int i = 0; i < MAX_FALLING_EGGS; ++i)
    {
        fallingEggActive[i] = false;
        fallingEggImages[i].setVisible(false);
    }


    activePopEggs = 0;
    for (int i = 0; i < MAX_POP_EGGS; ++i)
    {
        popEggActive[i] = false;
        popEggLeftImages[i].setAlpha(255U);
        popEggRightImages[i].setAlpha(255U);
        popEggLeftImages[i].setVisible(false);
        popEggRightImages[i].setVisible(false);
    }
}
// Thêm vào Screen2View.cpp

// Trong constructor


int readADCChannel(ADC_HandleTypeDef* hadc, uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    HAL_ADC_ConfigChannel(hadc, &sConfig);

    HAL_ADC_Start(hadc);
    HAL_ADC_PollForConversion(hadc, 100);
    int val = HAL_ADC_GetValue(hadc);
    HAL_ADC_Stop(hadc);

    return val;
}

void Screen2View::updateJoystickInput()
{
    extern ADC_HandleTypeDef hadc1;
    extern ADC_HandleTypeDef hadc2;
    extern UART_HandleTypeDef huart1;

    // Đọc giá trị ADC từ joystick
    uint32_t adcX = readADCChannel(&hadc1, ADC_CHANNEL_13);
    uint32_t adcY = readADCChannel(&hadc2, ADC_CHANNEL_5);

    // Nếu chưa calibrate, lấy vị trí hiện tại làm gốc
    if (!isCalibrated) {
        joystickCenterX = adcX;
        joystickCenterY = adcY;
        isCalibrated = true;

        char msg[64];
        snprintf(msg, sizeof(msg), "Calibrated - X:%lu Y:%lu\r\n", adcX, adcY);
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
    }

    // Tính toán rawX/rawY
    int16_t rawX = (int16_t)(adcX - joystickCenterX);
    int16_t rawY = (int16_t)(adcY - joystickCenterY);

    // Chỉ áp dụng deadzone một lần trên giá trị ADC thô.
    if (abs(rawX) < JOYSTICK_DEADZONE) rawX = 0;
    if (abs(rawY) < JOYSTICK_DEADZONE) rawY = 0;

    // Clamp và scale
    const int MAX_RANGE = 800;
    rawX = clamp<int16_t>(rawX, -MAX_RANGE, MAX_RANGE);
    rawY = clamp<int16_t>(rawY, -MAX_RANGE, MAX_RANGE);

    // Gán lại cho biến toàn cục của joystick (dùng để tính góc ngắm)
    joystickX = rawX;
    joystickY = rawY;

    // ĐÃ XÓA TOÀN BỘ PHẦN ĐỌC BUTTON VÀ CHỐNG DỘI Ở ĐÂY!
}


// Thay thế hàm updateAimDirection() hiện tại bằng code này:

void Screen2View::updateAimDirection()
{
    updateJoystickInput();

    uint32_t currentTime = HAL_GetTick();

    // Kiểm tra xem có đang di chuyển joystick không
    if(joystickX != 0 || joystickY != 0)
    {
        // Đang ngắm - tính góc đích mới
        isAiming = true;
        lastAimTime = currentTime;

        // Tính góc đích từ joystick
        float angleRad = atan2f((float)joystickX, -(float)joystickY);
        float newTargetAngle = angleRad * 180.0f / 3.14159f;

        // Giới hạn góc
        newTargetAngle = clamp(newTargetAngle, -80.0f, 80.0f);

        // Cập nhật góc đích
        targetAngle = newTargetAngle;
    }
    else
    {
        // Không di chuyển joystick - kiểm tra xem có nên dừng ngắm không
        if(isAiming && (currentTime - lastAimTime) > STOP_AIM_DELAY)
        {
            isAiming = false; // Dừng ngắm nhưng GIỮ NGUYÊN góc hiện tại
        }
    }

    // Luôn luôn làm mượt góc về phía targetAngle
    smoothAngleToTarget();

    // Cập nhật vị trí cannon và egg
    updateCannonAndEggPosition();
}
// Gọi debugAiming() trong handleTickEvent() để kiểm tra:
void Screen2View::handleTickEvent()
{
    updateAimDirection();
    updateProjectile();
    updateFallingEggs();
    updatePopAnimations();
    debugAiming();  // THÊM dòng này để debug
    if (pendingGroupClear)
    {
        pendingGroupClear = false; // Đánh dấu đã xử lý
        findAndRemoveMatchingGroup(pendingRow, pendingCol);

        // Cho phép nhóm vừa tạo được xóa trước; chỉ game-over nếu sau đó
        // vẫn còn trứng chạm hoặc vượt qua danger line.
        if (hasEggCrossedDangerLine())
        {
            triggerGameOver();
            return;
        }

        // Sau khi xử lý xong, tạo quả mới
        onEggShot();
    }
}
void Screen2View::clearNextEgg()
{
    nextEgg.setVisible(false);
    nextEgg.invalidate();
}
// Cải thiện hàm test joystick
void Screen2View::testJoystickWithNextEgg()
{
    updateJoystickInput();

    static uint32_t lastPrintTick = 0;
    uint32_t now = HAL_GetTick();

    if (now - lastPrintTick > 500)  // In mỗi 500ms
    {
        lastPrintTick = now;

        char msg[128];
        snprintf(msg, sizeof(msg),
                "Raw X: %d, Y: %d | Angle: %d | Center X: %d, Y: %d\r\n",
                joystickX, joystickY, aimAngle, joystickCenterX, joystickCenterY);
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
    }
}
// Thêm hàm reset calibration (gọi khi cần hiệu chỉnh lại)
void Screen2View::resetJoystickCalibration()
{
    isCalibrated = false;
    joystickCenterX = 0;
    joystickCenterY = 0;

    char msg[] = "Joystick calibration reset. Move to center and wait...\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
}

// Cũng cần sửa hàm updateCannonAndEggPosition() để đồng bộ:
void Screen2View::updateCannonAndEggPosition()
{
    // THAY ĐỔI 4: Sử dụng cùng công thức tính góc
    float radians = (aimAngle * 3.14159f) / 180.0f;

    int cannonBaseX = getCannonBaseX();
    int cannonBaseY = getCannonBaseY();

    const int EGG_DISTANCE = 45;

    int nextEggX = cannonBaseX + (int)(sin(radians) * EGG_DISTANCE);
    int nextEggY = cannonBaseY - (int)(cos(radians) * EGG_DISTANCE);

    nextEgg.moveTo(nextEggX - nextEgg.getWidth()/2,
                   nextEggY - nextEgg.getHeight()/2);

    updateAimLine();
}

// CẢI THIỆN HÀM DEBUG:
void Screen2View::debugAiming()
{
    static uint32_t lastTick = 0;
    uint32_t now = HAL_GetTick();
    if (now - lastTick > 300) // Debug mỗi 300ms
    {
        lastTick = now;
//        char msg[200];
//        snprintf(msg, sizeof(msg),
//           "Target:%d.%d", (int)targetAngle, abs((int)(targetAngle * 10) % 10));
//
//        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
    }
}
// THÊM HÀM TIỆN ÍCH - Reset về góc 0:
void Screen2View::resetAimToCenter()
{
    targetAngle = 0;
    isAiming = true; // Bắt buộc làm mượt về 0
    lastAimTime = HAL_GetTick();
}
// THÊM HÀM TIỆN ÍCH - Đặt góc ngắm thủ công (nếu cần):
void Screen2View::setAimAngle(float angle)
{
    targetAngle = clamp(angle, -80.0f, 80.0f);
    currentAngleFloat = targetAngle;
    aimAngle = (int)round(currentAngleFloat);
    isAiming = false;
}
// Cập nhật đường ngắm từ base cannon
void Screen2View::updateAimLine()
{
    // Chưa có Line widget; hướng ngắm được thể hiện bằng cannon và nextEgg.
}


// Cập nhật hàm bắn để bắn từ đầu nòng súng
bool Screen2View::shootEgg()
{
    // Một lần chỉ có một projectile. Sau va chạm, tiếp tục khóa cho tới
    // khi nhóm đã được xử lý và quả kế tiếp đã được sinh.
    if (projectileActive || pendingGroupClear ||
        activeFallingEggs > 0 || activePopEggs > 0)
    {
        return false;
    }

    // Ẩn nextEgg khi bắn
    nextEgg.setVisible(false);
    nextEgg.invalidate();

    float radians = (aimAngle * 3.14159f) / 180.0f;

    const int SHOOT_DISTANCE = 45;

    int startX = getCannonBaseX() + (int)(sin(radians) * SHOOT_DISTANCE);
    int startY = getCannonBaseY() - (int)(cos(radians) * SHOOT_DISTANCE);


    float dirX = sin(radians);
    float dirY = -cos(radians);
    const float speed = 5.0f;

    createProjectile(startX, startY, dirX * speed, dirY * speed);
    return true;
}

void Screen2View::performShootAction()
{
    extern UART_HandleTypeDef huart1;
    // Chỉ phản hồi rung và log khi một phát bắn thực sự được tạo ra thành công
    if (shootEgg())
    {
        // Rung bất đồng bộ
        Haptic_Play(HAPTIC_SHOOT);
        // Debug UART cũ
        const char* beepMsg = "Beep!\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)beepMsg, strlen(beepMsg), 100);
    }
}

// Hàm tạo đạn bay (cần implement thêm)
void Screen2View::createProjectile(int x, int y, float vx, float vy)
{
    projectileX = x;
    projectileY = y;
    projectileVX = vx;
    projectileVY = vy;
    projectileActive = true;

    // Vị trí viên đạn lúc đầu
    projectileImage.setXY((int)projectileX - projectileImage.getWidth()/2,
                          (int)projectileY - projectileImage.getHeight()/2);
    projectileImage.setVisible(true);
    projectileImage.invalidate();
    projectileColor = currentEggColor;
    // Cập nhật bitmap đạn theo màu
    switch(currentEggColor)
    {
        case RED:    projectileImage.setBitmap(BITMAP_EGG_RED_ID); break;
        case BLUE:   projectileImage.setBitmap(BITMAP_EGG_BLUE_ID); break;
        case GREEN:  projectileImage.setBitmap(BITMAP_EGG_GREEN_ID); break;
        case YELLOW: projectileImage.setBitmap(BITMAP_EGG_YELLOW_ID); break;
        case PURPLE: projectileImage.setBitmap(BITMAP_EGG_PURPLE_ID); break;
    }

}


void Screen2View::updateProjectile()
{
    if (!projectileActive) return;

    const int BALL_SIZE = 32;
    const int SCREEN_WIDTH = 240 ;

    // Cập nhật vị trí
    projectileX += projectileVX;
    projectileY += projectileVY;

    // Xử lý phản xạ cạnh trái
    if (projectileX <= 0)
    {
        projectileX = 0;
        projectileVX = -projectileVX;
    }
    // Xử lý phản xạ cạnh phải
    else if (projectileX + BALL_SIZE >= SCREEN_WIDTH)
    {
        projectileX = SCREEN_WIDTH - BALL_SIZE;
        projectileVX = -projectileVX;
    }

    // Kiểm tra va chạm với bóng
    checkProjectileCollision();

    // Cập nhật hình ảnh nếu vẫn còn active
    if (projectileActive)
    {
        updateProjectileVisual();
    }

    // Kiểm tra timeout để tránh treo
    static uint32_t projectileStartTime = 0;
    if (projectileActive && projectileStartTime == 0)
    {
        projectileStartTime = HAL_GetTick();
    }
    else if (projectileActive && (HAL_GetTick() - projectileStartTime) > 10000) // 5 giây
    {
        projectileActive = false;
        projectileImage.setVisible(false);
        projectileImage.invalidate();
        projectileStartTime = 0;

        HAL_UART_Transmit(&huart1, (uint8_t*)"Projectile timeout\r\n", 20, 100);
    }
    else if (!projectileActive)
    {
        projectileStartTime = 0;
    }

}




void Screen2View::updateAimVisual()
{
    // Nếu chưa có gì cụ thể thì để hàm rỗng cũng được
    // Sau này có thể thêm hiệu ứng như đường bắn, tia laser, mũi tên...
}
// THAY THẾ hàm handleCollisionWithEgg() bằng version cải tiến này:

void Screen2View::handleCollisionWithEgg(int hitRow, int hitCol)
{
    if (!isValidGridPosition(hitRow, hitCol))
    {
        // Nếu vị trí không hợp lệ, đặt vào hàng đầu
        hitRow = 0;
        const int topRowMaxCol = isShiftedRow(0) ? COLS - 2 : COLS - 1;
        hitCol = clamp(hitCol, 0, topRowMaxCol);
    }

    int attachRow = -1, attachCol = -1;

    // Trường hợp 1: va vào ô trống → gắn trực tiếp
    if (eggGrid[hitRow][hitCol] == EMPTY)
    {
        attachRow = hitRow;
        attachCol = hitCol;
    }
    else
    {
        // Trường hợp 2: va vào trứng có sẵn → tìm ô lân cận gần nhất theo lưới tổ ong
        const int dr[] = {-1,-1,0,0,1,1};
        const int dc_even[] = {-1,0,-1,1,-1,0};
        const int dc_odd[]  = {0,1,-1,1,0,1};

        float minDistSquared = 1e9;
        int bestRow = -1, bestCol = -1;

        for (int i = 0; i < 6; ++i)
        {
            int nr = hitRow + dr[i];
            int nc = hitCol +
                (isShiftedRow(hitRow) ? dc_odd[i] : dc_even[i]);

            if (!isValidGridPosition(nr, nc)) continue;
            if (eggGrid[nr][nc] != EMPTY) continue;

            // Tính tâm ô trứng (pixel)
            int baseX = container2.getX();
            int baseY = container2.getY();
            int xOffset = isShiftedRow(nr) ? 15 : 0;
            float cellX = baseX + nc * 30 + xOffset + 16;
            float cellY = baseY + nr * 26 + 16;

            float dx = projectileX - cellX;
            float dy = projectileY - cellY;
            float d2 = dx * dx + dy * dy;

            if (d2 < minDistSquared)
            {
                minDistSquared = d2;
                bestRow = nr;
                bestCol = nc;
            }
        }

        attachRow = bestRow;
        attachCol = bestCol;
    }

    // Nếu tìm được ô trống để gắn
    if (attachRow != -1 && attachCol != -1 &&
        isValidGridPosition(attachRow, attachCol) &&
        eggGrid[attachRow][attachCol] == EMPTY)
    {
        eggGrid[attachRow][attachCol] = projectileColor;
        renderEggGrid();

        char msg[64];
        snprintf(msg, sizeof(msg), "Attached at (%d,%d) color:%d\r\n",
                attachRow, attachCol, currentEggColor);
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
        pendingGroupClear = true;
        pendingRow = attachRow;
        pendingCol = attachCol;

    }
    else
    {
        // Không tìm được → ép vào hàng đầu hoặc báo game over
        handleFailedAttachment(hitCol);
    }

    // Luôn dừng viên đạn
    projectileActive = false;
    projectileImage.setVisible(false);
    projectileImage.invalidate();

}




// HÀM XỬ LÝ KHI KHÔNG ĐẶT ĐƯỢC TRỨNG:
void Screen2View::handleFailedAttachment(int col)
{
    // Thử đặt ở hàng đầu
    const int topRowMaxCol = isShiftedRow(0) ? COLS - 2 : COLS - 1;
    col = clamp(col, 0, topRowMaxCol);

    if (eggGrid[0][col] == EMPTY)
    {
        eggGrid[0][col] = projectileColor;
        renderEggGrid();

        HAL_UART_Transmit(&huart1, (uint8_t*)"Forced attach at top\r\n", 22, 100);
        pendingGroupClear = true;
        pendingRow = 0;
        pendingCol = col;
    }
    else
    {
        // Tìm ô trống đầu tiên ở hàng đầu
        for (int c = 0; c < COLS; c++)
        {
            if (!isValidGridPosition(0, c))
            {
                continue;
            }

            if (eggGrid[0][c] == EMPTY)
            {
                eggGrid[0][c] = projectileColor;
                renderEggGrid();
                pendingGroupClear = true;
                pendingRow = 0;
                pendingCol = c;
                return;
            }
        }

        // Nếu hàng đầu đầy - Game Over
        HAL_UART_Transmit(&huart1, (uint8_t*)"Game Over - Top row full!\r\n", 27, 100);
        triggerGameOver();
    }
}


// THÊM VÀO HEADER FILE (.hpp):
// int findBestAttachPosition(int hitRow, int hitCol);
// int findBestAttachColumn(int targetRow, int preferredCol);
// void handleFailedAttachment(int col);
void Screen2View::findAndRemoveMatchingGroup(int row, int col)
{
    if(!isValidGridPosition(row,col)) return;
    int color = eggGrid[row][col];
    if(color == EMPTY) return;

    static bool visited[ROWS][COLS];
    memset(visited, 0, sizeof(visited));

    const int MAX = ROWS*COLS;
    int stackR[MAX], stackC[MAX], top = 0;
    int groupR[MAX], groupC[MAX], gsize = 0;

    stackR[top] = row; stackC[top++] = col; visited[row][col] = true;

    const int dr[6]      = {-1,-1,0,0,1,1};
    const int dc_even[6] = {-1,0,-1,1,-1,0};
    const int dc_odd[6]  = {0,1,-1,1,0,1};

    while(top)
    {
        int r = stackR[--top], c = stackC[top];
        groupR[gsize] = r; groupC[gsize++] = c;

        for(int i=0;i<6;i++)
        {
            int nr = r + dr[i];
            int nc = c + (isShiftedRow(r) ? dc_odd[i] : dc_even[i]);
            if(!isValidGridPosition(nr,nc) || visited[nr][nc] || eggGrid[nr][nc]!=color) continue;
            visited[nr][nc] = true;
            stackR[top] = nr; stackC[top++] = nc;
        }
    }

    const bool groupRemoved = gsize >= 3;
    if (groupRemoved)
    {
        for (int i = 0; i < gsize; ++i)
        {
            const int groupRow = groupR[i];
            const int groupCol = groupC[i];
            startPopAnimation(
                groupRow, groupCol, eggGrid[groupRow][groupCol]);
            eggGrid[groupRow][groupCol] = EMPTY;
        }
    }

    // Chạy kiểm tra neo trần sau mọi lần resolve, kể cả khi phát bắn
    // không tạo nhóm. Điều này tự sửa mọi cụm mồ côi còn sót từ state cũ.
    const int droppedEggs = detachUnsupportedEggs();
    if (groupRemoved || droppedEggs > 0)
    {
        updateScore((groupRemoved ? gsize : 0) + droppedEggs);
        renderEggGrid();
    }

    if (groupRemoved)
    {
        Haptic_Play((gsize >= 6) ? HAPTIC_COMBO : HAPTIC_POP);
        HAL_UART_Transmit(&huart1, (uint8_t*)"Group cleared!\r\n", 16, 100);
    }
}

void Screen2View::startPopAnimation(int row, int col, uint8_t color)
{
    int slot = -1;
    for (int i = 0; i < MAX_POP_EGGS; ++i)
    {
        if (!popEggActive[i])
        {
            slot = i;
            break;
        }
    }

    if (slot < 0)
    {
        return;
    }

    switch (color)
    {
        case RED:
            popEggLeftImages[slot].setBitmap(BITMAP_EGG_RED_BROKEN_LEFT_ID);
            popEggRightImages[slot].setBitmap(BITMAP_EGG_RED_BROKEN_RIGHT_ID);
            break;
        case BLUE:
            popEggLeftImages[slot].setBitmap(BITMAP_EGG_BLUE_BROKEN_LEFT_ID);
            popEggRightImages[slot].setBitmap(BITMAP_EGG_BLUE_BROKEN_RIGHT_ID);
            break;
        case GREEN:
            popEggLeftImages[slot].setBitmap(BITMAP_EGG_GREEN_BROKEN_LEFT_ID);
            popEggRightImages[slot].setBitmap(BITMAP_EGG_GREEN_BROKEN_RIGHT_ID);
            break;
        case YELLOW:
            popEggLeftImages[slot].setBitmap(BITMAP_EGG_YELLOW_BROKEN_LEFT_ID);
            popEggRightImages[slot].setBitmap(BITMAP_EGG_YELLOW_BROKEN_RIGHT_ID);
            break;
        case PURPLE:
            popEggLeftImages[slot].setBitmap(BITMAP_EGG_PURPLE_BROKEN_LEFT_ID);
            popEggRightImages[slot].setBitmap(BITMAP_EGG_PURPLE_BROKEN_RIGHT_ID);
            break;
        default: return;
    }

    const int x = col * 30 + (isShiftedRow(row) ? 15 : 0);
    const int y = row * EGG_SPACING_Y;
    popEggStartX[slot] = static_cast<int16_t>(x);
    popEggStartY[slot] = static_cast<int16_t>(y);
    popEggLeftImages[slot].setXY(x, y);
    popEggRightImages[slot].setXY(x, y);
    popEggLeftImages[slot].setAlpha(255U);
    popEggRightImages[slot].setAlpha(255U);
    popEggLeftImages[slot].setVisible(true);
    popEggRightImages[slot].setVisible(true);
    popEggLeftImages[slot].invalidate();
    popEggRightImages[slot].invalidate();
    popEggStartTick[slot] = HAL_GetTick();
    popEggActive[slot] = true;
    ++activePopEggs;
}

void Screen2View::updatePopAnimations()
{
    const uint32_t now = HAL_GetTick();

    for (int i = 0; i < MAX_POP_EGGS; ++i)
    {
        if (!popEggActive[i])
        {
            continue;
        }

        const uint32_t elapsed = now - popEggStartTick[i];
        if (elapsed >= POP_ANIMATION_MS)
        {
            popEggActive[i] = false;
            popEggLeftImages[i].setVisible(false);
            popEggRightImages[i].setVisible(false);
            popEggLeftImages[i].setAlpha(255U);
            popEggRightImages[i].setAlpha(255U);
            popEggLeftImages[i].invalidate();
            popEggRightImages[i].invalidate();
            --activePopEggs;
            continue;
        }

        const int separation = 2 + static_cast<int>((12U * elapsed) / POP_ANIMATION_MS);
        const int drop = 2 + static_cast<int>(
            (18U * elapsed * elapsed) /
            (POP_ANIMATION_MS * POP_ANIMATION_MS));

        popEggLeftImages[i].moveTo(
            popEggStartX[i] - separation, popEggStartY[i] + drop);
        popEggRightImages[i].moveTo(
            popEggStartX[i] + separation, popEggStartY[i] + drop + 1);

        const uint32_t fadeStart = (POP_ANIMATION_MS * 3U) / 5U;
        uint8_t alpha = 255U;
        if (elapsed > fadeStart)
        {
            alpha = static_cast<uint8_t>(
                (255U * (POP_ANIMATION_MS - elapsed)) /
                (POP_ANIMATION_MS - fadeStart));
        }
        popEggLeftImages[i].setAlpha(alpha);
        popEggRightImages[i].setAlpha(alpha);
        popEggLeftImages[i].invalidate();
        popEggRightImages[i].invalidate();
    }
}

int Screen2View::detachUnsupportedEggs()
{
    bool connected[ROWS][COLS];
    memset(connected, 0, sizeof(connected));

    const int maxEggs = ROWS * COLS;
    int stackR[maxEggs];
    int stackC[maxEggs];
    int top = 0;

    // Mọi quả ở hàng trên cùng đều được xem là neo vào trần.
    for (int col = 0; col < COLS; ++col)
    {
        if (isValidGridPosition(0, col) && eggGrid[0][col] != EMPTY)
        {
            connected[0][col] = true;
            stackR[top] = 0;
            stackC[top++] = col;
        }
    }

    const int dr[6] = {-1, -1, 0, 0, 1, 1};
    const int dcEven[6] = {-1, 0, -1, 1, -1, 0};
    const int dcOdd[6] = {0, 1, -1, 1, 0, 1};

    while (top > 0)
    {
        --top;
        const int row = stackR[top];
        const int col = stackC[top];

        for (int i = 0; i < 6; ++i)
        {
            const int nextRow = row + dr[i];
            const int nextCol = col +
                (isShiftedRow(row) ? dcOdd[i] : dcEven[i]);

            if (!isValidGridPosition(nextRow, nextCol) ||
                connected[nextRow][nextCol] ||
                eggGrid[nextRow][nextCol] == EMPTY)
            {
                continue;
            }

            connected[nextRow][nextCol] = true;
            stackR[top] = nextRow;
            stackC[top++] = nextCol;
        }
    }

    int droppedCount = 0;
    for (int row = 0; row < ROWS; ++row)
    {
        for (int col = 0; col < COLS; ++col)
        {
            if (eggGrid[row][col] == EMPTY || connected[row][col])
            {
                continue;
            }

            const uint8_t color = eggGrid[row][col];
            int slot = -1;
            for (int i = 0; i < MAX_FALLING_EGGS; ++i)
            {
                if (!fallingEggActive[i])
                {
                    slot = i;
                    break;
                }
            }

            if (slot >= 0)
            {
                switch (color)
                {
                    case RED:    fallingEggImages[slot].setBitmap(BITMAP_EGG_RED_ID); break;
                    case BLUE:   fallingEggImages[slot].setBitmap(BITMAP_EGG_BLUE_ID); break;
                    case GREEN:  fallingEggImages[slot].setBitmap(BITMAP_EGG_GREEN_ID); break;
                    case YELLOW: fallingEggImages[slot].setBitmap(BITMAP_EGG_YELLOW_ID); break;
                    case PURPLE: fallingEggImages[slot].setBitmap(BITMAP_EGG_PURPLE_ID); break;
                    default: break;
                }

                const int x = col * 30 + (isShiftedRow(row) ? 15 : 0);
                const int y = row * EGG_SPACING_Y;
                fallingEggY[slot] = static_cast<float>(y);
                fallingEggVelocity[slot] = 1.5f;
                fallingEggActive[slot] = true;
                ++activeFallingEggs;

                fallingEggImages[slot].setXY(x, y);
                fallingEggImages[slot].setVisible(true);
                fallingEggImages[slot].invalidate();
            }

            eggGrid[row][col] = EMPTY;
            ++droppedCount;
        }
    }

    return droppedCount;
}

void Screen2View::updateFallingEggs()
{
    const float gravity = 0.35f;
    const float terminalVelocity = 8.0f;

    for (int i = 0; i < MAX_FALLING_EGGS; ++i)
    {
        if (!fallingEggActive[i])
        {
            continue;
        }

        fallingEggVelocity[i] += gravity;
        if (fallingEggVelocity[i] > terminalVelocity)
        {
            fallingEggVelocity[i] = terminalVelocity;
        }

        fallingEggY[i] += fallingEggVelocity[i];
        if (fallingEggY[i] >= container2.getHeight())
        {
            fallingEggActive[i] = false;
            fallingEggImages[i].setVisible(false);
            fallingEggImages[i].invalidate();
            --activeFallingEggs;
            continue;
        }

        fallingEggImages[i].moveTo(
            fallingEggImages[i].getX(), static_cast<int16_t>(fallingEggY[i]));
    }
}



int Screen2View::getCannonBaseX() {
    return canon.getX() + canon.getWidth() / 2;
}

int Screen2View::getCannonBaseY() {
    return canon.getY();
}

void Screen2View::checkProjectileCollision()
{
    if (!projectileActive) return;

    const int EGG_WIDTH = 32;
    const int EGG_HEIGHT = 32;
    const int EGG_SPACING_X = 30;
    const int EGG_SPACING_Y = 26;
    const int HEX_OFFSET = 15;

    int baseX = container2.getX();
    int baseY = container2.getY();

    float projCenterX = projectileX;
    float projCenterY = projectileY;

    // Hai sprite tròn đường kính khoảng 30 px vừa chạm nhau ở khoảng cách này.
    const float collisionRadius = 30.0f;
    const float collisionRadiusSquared = collisionRadius * collisionRadius;

    bool collided = false;

    for (int row = 0; row < ROWS && !collided; ++row)
    {
        for (int col = 0; col < COLS && !collided; ++col)
        {
            if (!isValidGridPosition(row, col)) continue;
            if (eggGrid[row][col] == EMPTY) continue;

            int xOffset = isShiftedRow(row) ? HEX_OFFSET : 0;
            int eggX = baseX + col * EGG_SPACING_X + xOffset;
            int eggY = baseY + row * EGG_SPACING_Y;

            float eggCenterX = eggX + EGG_WIDTH / 2.0f;
            float eggCenterY = eggY + EGG_HEIGHT / 2.0f;

            float dx = projCenterX - eggCenterX;
            float dy = projCenterY - eggCenterY;
            float d2 = dx * dx + dy * dy;


            if (d2 <= collisionRadiusSquared)
            {
                collided = true;

                // Thực sự va chạm
                projectileActive = false;
                projectileImage.setVisible(false);
                projectileImage.invalidate();

                HAL_UART_Transmit(&huart1, (uint8_t*)"Va cham - STOP!\r\n", 18, 100);
                handleCollisionWithEgg(row, col);
                break;
            }
        }
    }

    // Nếu không va trứng nào, kiểm tra trần
    if (!collided && projectileY <= baseY)
    {
        float projCenterXRel = projCenterX - baseX;
        const int topRowOffset = isShiftedRow(0) ? HEX_OFFSET : 0;
        int col = (int)((projCenterXRel - topRowOffset) / EGG_SPACING_X);
        const int topRowMaxCol = isShiftedRow(0) ? COLS - 2 : COLS - 1;
        col = clamp(col, 0, topRowMaxCol);

        projectileActive = false;
        projectileImage.setVisible(false);
        projectileImage.invalidate();

        HAL_UART_Transmit(&huart1, (uint8_t*)"Chạm trần - Gắn trên cùng\r\n", 28, 100);
        handleCollisionWithEgg(0, col);
    }
}





void Screen2View::updateProjectileVisual()
{
    if (projectileActive)
    {
        // Di chuyển viên đạn đến vị trí mới
        projectileImage.moveTo((int)projectileX - projectileImage.getWidth()/2,
                               (int)projectileY - projectileImage.getHeight()/2);
        projectileImage.invalidate();
    }
    else
    {
        // Khi viên đạn không còn hoạt động, ẩn nó đi
        projectileImage.setVisible(false);
        projectileImage.invalidate();
    }
}
// THÊM hàm kiểm tra tính hợp lệ của grid:
bool Screen2View::isShiftedRow(int row) const
{
    return ((row + gridPhase) & 1) != 0;
}

bool Screen2View::isValidGridPosition(int row, int col) const
{
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS)
    {
        return false;
    }

    // Pha lưới đổi sau mỗi lần hạ; hàng lệch có 7 ô, hàng còn lại có 8.
    return !(isShiftedRow(row) && col == (COLS - 1));
}

void Screen2View::debugJoystick()
{
    static uint32_t lastTick = 0;
    uint32_t now = HAL_GetTick();
    if (now - lastTick > 500)
    {
        lastTick = now;
        char msg[128];
        snprintf(msg, sizeof(msg),
            "X: %d, Y: %d | Angle: %d | Center: (%d, %d)\r\n",
            joystickX, joystickY, aimAngle, joystickCenterX, joystickCenterY);
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
    }
}

// THÊM HÀM MỚI - Làm mượt góc:
void Screen2View::smoothAngleToTarget()
{
    // Tính khoảng cách góc cần di chuyển
    float angleDiff = targetAngle - currentAngleFloat;

    // Xử lý trường hợp góc vượt qua ±180 độ
    if(angleDiff > 180.0f) angleDiff -= 360.0f;
    if(angleDiff < -180.0f) angleDiff += 360.0f;

    // Nếu khoảng cách nhỏ, coi như đã đạt
    if(abs(angleDiff) < ANGLE_THRESHOLD)
    {
        currentAngleFloat = targetAngle;
    }
    else
    {
        // Làm mượt di chuyển về phía góc đích
        currentAngleFloat += angleDiff * SMOOTH_FACTOR;
    }

    // Cập nhật aimAngle (dạng int để sử dụng)
    aimAngle = (int)round(currentAngleFloat);
}
// THÊM HÀM CURVE MƯỢT:
int16_t Screen2View::applySmoothCurve(int16_t value, int16_t maxRange)
{
    if(value == 0) return 0;

    // Tính tỷ lệ (-1.0 đến 1.0)
    float ratio = (float)value / maxRange;

    // Áp dụng curve mượt (cubic easing)
    float smoothed = ratio * ratio * ratio;

    // Chuyển về giá trị int
    return (int16_t)(smoothed * maxRange);
}
// THÊM hàm force reset game nếu bị đơ:
void Screen2View::forceResetGame()
{
    // Dừng tất cả projectile
    projectileActive = false;
    projectileImage.setVisible(false);
    projectileImage.invalidate();

    // Reset các biến ngắm
    isAiming = false;
    targetAngle = 0;
    currentAngleFloat = 0;
    aimAngle = 0;

    // Spawn lại nextEgg
    spawnNextEgg();

    HAL_UART_Transmit(&huart1, (uint8_t*)"Game force reset!\r\n", 19, 100);
}
