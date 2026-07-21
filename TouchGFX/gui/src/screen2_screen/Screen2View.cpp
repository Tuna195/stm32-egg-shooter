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

Screen2View::Screen2View() : score(0), shotCount(0), droppedLineCount(0),
                           targetAngle(0), currentAngleFloat(0),
                           isAiming(false), lastAimTime(0)
{
    // Khởi tạo mảng eggGrid
    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            eggGrid[i][j] = EMPTY;
        }
    }
}

// CÁCH 2: DI CHUYỂN WIDGET ĐÚNG CÁCH
void Screen2View::setupScreen()
{
	Screen2ViewBase::setupScreen();

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

	    shotCount = 0;
	    droppedLineCount = 0;
	    aimAngle = 0;

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
        int maxCols = COLS - (row % 2);

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
                int xOffset = (row % 2 == 1) ? HEX_OFFSET : 0; // Hàng lẻ lệch phải
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

//        // Kiểm tra game over
//        if(checkGameOver())
//        {
//
//            return;
//        }
    }

    // Spawn trứng mới cho lần bắn tiếp theo
    spawnNextEgg();
}


/** Trả về true nếu trứng đã chạm hàng cuối của vùng chơi. */
bool Screen2View::hasEggBelowVisible() const
{
    const int bottomVisibleRow = VISIBLE_ROWS - 1;
    for (int col = 0; col < COLS; ++col)
        if (eggGrid[bottomVisibleRow][col] != EMPTY)
            return true;
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

        // Hàng lẻ chỉ có 7 ô hợp lệ.
        if ((row % 2) == 1)
        {
            eggGrid[row][COLS - 1] = EMPTY;
        }
    }

    // Thêm hàng mới ở trên (có thể random hoặc theo pattern)
    addNewTopRow();

    // Render lại
    renderEggGrid();

	if(hasEggBelowVisible())
	{
		triggerGameOver();
	}
}

void Screen2View::addNewTopRow()
{
    // Thêm hàng mới ở trên với pattern logic
    uint32_t tick = HAL_GetTick();

    for(int col = 0; col < COLS; col++)
    {
        eggGrid[0][col] = randomColor(); // Luôn có trứng ở mỗi ô
    }
}
bool Screen2View::checkBottomRowOccupied()
{
    for(int col = 0; col < COLS; ++col)
        if(eggGrid[ROWS - 1][col] != EMPTY)
            return true;
    return false;
}

/* Hàm dùng chung để đóng game và nhảy màn */
void Screen2View::triggerGameOver()
{
    Haptic_Play(HAPTIC_GAMEOVER);

    // Tắt viên đạn đang bay (nếu có)
    projectileActive = false;
    projectileImage.setVisible(false);
    projectileImage.invalidate();

    HAL_UART_Transmit(&huart1,
        (uint8_t*)"GAME OVER – drop > 5!\r\n", 24, 100);

    notifyGameOver();
}
void Screen2View::notifyGameOver()
{
    presenter->handleGameOver();   // Gọi sang presenter
}
bool Screen2View::checkGameOver()
{
    // Kiểm tra xem có trứng nào ở hàng cuối không
    for(int col = 0; col < COLS; col++)
    {
        if(eggGrid[ROWS - 1][col] != EMPTY)
        {
            return true; // Game Over!
        }
    }

    // Hoặc kiểm tra xem có trứng nào quá gần đường bắn không
    for(int col = 0; col < COLS; col++)
    {
        if(eggGrid[ROWS - 2][col] != EMPTY) // Hàng áp cuối
        {
            return true; // Sắp Game Over
        }
    }

    return false;
}


void Screen2View::spawnNextEgg()
{
    // Ẩn trứng cũ trước
    nextEgg.setVisible(false);
    nextEgg.invalidate();

    uint32_t tick = HAL_GetTick();
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
    extern UART_HandleTypeDef huart1;

    // Đọc giá trị ADC từ joystick
    uint32_t adcX = readADCChannel(&hadc1, ADC_CHANNEL_13);
    uint32_t adcY = readADCChannel(&hadc1, ADC_CHANNEL_5);

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

    // Deadzone để tránh nhiễu
    const int SMALL_DEADZONE = JOYSTICK_DEADZONE / 2;
    if (abs(rawX) < SMALL_DEADZONE) rawX = 0;
    if (abs(rawY) < SMALL_DEADZONE) rawY = 0;

    // Clamp và scale
    const int MAX_RANGE = 800;
    rawX = clamp<int16_t>(rawX, -MAX_RANGE, MAX_RANGE);
    rawY = clamp<int16_t>(rawY, -MAX_RANGE, MAX_RANGE);

    // Làm mượt
    joystickX = applySmoothCurve(rawX, MAX_RANGE);
    joystickY = applySmoothCurve(rawY, MAX_RANGE);

    // Kiểm tra nút nhấn (SW)
    GPIO_PinState buttonState = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2);
    bool currentPressed = (buttonState == GPIO_PIN_RESET);

    if (currentPressed && !lastButtonPressed)
    {
        // Gọi chức năng bắn
        shootEgg();

        // Rung bất đồng bộ; không chặn GUI/game loop.
        Haptic_Play(HAPTIC_SHOOT);

        // Debug UART nếu cần
        const char* beepMsg = "Beep!\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)beepMsg, strlen(beepMsg), 100);
    }

    lastButtonPressed = currentPressed;
}

// Thay thế hàm updateAimDirection() hiện tại bằng code này:

void Screen2View::updateAimDirection()
{
    updateJoystickInput();

    uint32_t currentTime = HAL_GetTick();

    // Kiểm tra xem có đang di chuyển joystick không
    if(abs(joystickX) > JOYSTICK_DEADZONE || abs(joystickY) > JOYSTICK_DEADZONE)
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
    debugAiming();  // THÊM dòng này để debug
    if (pendingGroupClear)
    {
        pendingGroupClear = false; // Đánh dấu đã xử lý
        findAndRemoveMatchingGroup(pendingRow, pendingCol);

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
    float radians = (aimAngle * 3.14159f) / 180.0f;

    const int CANNON_BASE_X = 240;
    const int CANNON_BASE_Y = 320;
    const int AIM_LINE_LENGTH = 100;

    int endX = CANNON_BASE_X + (int)(sin(radians) * AIM_LINE_LENGTH);
    int endY = CANNON_BASE_Y - (int)(cos(radians) * AIM_LINE_LENGTH);

    // Nếu có Line widget để vẽ đường ngắm:
    // aimLine.setStart(CANNON_BASE_X, CANNON_BASE_Y);
    // aimLine.setEnd(aimLineEndX, aimLineEndY);
    // aimLine.invalidate();
}


// Cập nhật hàm bắn để bắn từ đầu nòng súng
void Screen2View::shootEgg()
{
    // Ẩn nextEgg khi bắn
    nextEgg.setVisible(false);
    nextEgg.invalidate();

    float radians = (aimAngle * 3.14159f) / 180.0f;

    const int CANNON_BASE_X = 240;
    const int CANNON_BASE_Y = 320;
    const int SHOOT_DISTANCE = 45;

    int startX = getCannonBaseX() + (int)(sin(radians) * SHOOT_DISTANCE);
    int startY = getCannonBaseY() - (int)(cos(radians) * SHOOT_DISTANCE);


    float dirX = sin(radians);
    float dirY = -cos(radians);
    float speed = 3.0f;

    createProjectile(startX, startY, dirX * speed, dirY * speed);
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
    const int SCREEN_HEIGHT = 320; // hoặc chiều cao thực tế

    // Lưu vị trí cũ
    float oldX = projectileX;
    float oldY = projectileY;

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
        hitCol = clamp(hitCol, 0, COLS - 1);
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
            int nc = hitCol + ((hitRow % 2 == 0) ? dc_even[i] : dc_odd[i]);

            if (!isValidGridPosition(nr, nc)) continue;
            if (eggGrid[nr][nc] != EMPTY) continue;

            // Tính tâm ô trứng (pixel)
            int baseX = container2.getX();
            int baseY = container2.getY();
            int xOffset = (nr % 2 == 1) ? 15 : 0;
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
    col = clamp(col, 0, COLS - 1);

    if (eggGrid[0][col] == EMPTY)
    {
        eggGrid[0][col] = currentEggColor;
        renderEggGrid();

        HAL_UART_Transmit(&huart1, (uint8_t*)"Forced attach at top\r\n", 22, 100);
        findAndRemoveMatchingGroup(0, col);
    }
    else
    {
        // Tìm ô trống đầu tiên ở hàng đầu
        for (int c = 0; c < COLS; c++)
        {
            if (eggGrid[0][c] == EMPTY)
            {
                eggGrid[0][c] = currentEggColor;
                renderEggGrid();
                findAndRemoveMatchingGroup(0, c);
                return;
            }
        }

        // Nếu hàng đầu đầy - Game Over
        HAL_UART_Transmit(&huart1, (uint8_t*)"Game Over - Top row full!\r\n", 27, 100);
        // application().gotoScreen3ScreenNoTransition();
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
            int nc = c + ((r&1)? dc_odd[i] : dc_even[i]);
            if(!isValidGridPosition(nr,nc) || visited[nr][nc] || eggGrid[nr][nc]!=color) continue;
            visited[nr][nc] = true;
            stackR[top] = nr; stackC[top++] = nc;
        }
    }

    if(gsize >= 3)
    {
        for(int i=0;i<gsize;i++) eggGrid[groupR[i]][groupC[i]] = EMPTY;
        updateScore(gsize);
        renderEggGrid();
        Haptic_Play((gsize >= 6) ? HAPTIC_COMBO : HAPTIC_POP);
        HAL_UART_Transmit(&huart1, (uint8_t*)"Group cleared!\r\n", 16, 100);
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
            if (eggGrid[row][col] == EMPTY) continue;

            int xOffset = (row % 2 == 1) ? HEX_OFFSET : 0;
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
        int col = (int)(projCenterXRel / EGG_SPACING_X);
        col = clamp(col, 0, COLS - 1);

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
bool Screen2View::isValidGridPosition(int row, int col)
{
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS)
    {
        return false;
    }

    // Lưới tổ ong: hàng chẵn 8 ô, hàng lẻ 7 ô lệch sang phải 15 px.
    return !((row % 2) == 1 && col == (COLS - 1));
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
