#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/Color.hpp>

#define ROWS 8
#define COLS 8
#define EMPTY 0
#define RED 1
#define BLUE 2
#define GREEN 3
#define YELLOW 4
#define PURPLE 5


class Screen2View : public Screen2ViewBase
{
public:
    Screen2View();
    virtual ~Screen2View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent() override;
    int getScore() const { return score; }


protected:
    void initEggGrid();
    void renderEggGrid();
    void spawnNextEgg();
    void updateScore(int destroyedCount);
    void clearEggGrid();
    void ensureMatchablePattern();
    void dropEggGrid();
    bool hasEggCrossedDangerLine() const;
    void addNewTopRow();
    void notifyGameOver();

private:
    uint8_t eggGrid[ROWS][COLS];
    touchgfx::Image eggImages[ROWS][COLS];
    touchgfx::Box dangerLine;
    static const int MAX_FALLING_EGGS = ROWS * COLS;
    touchgfx::Image fallingEggImages[MAX_FALLING_EGGS];
    float fallingEggY[MAX_FALLING_EGGS];
    float fallingEggVelocity[MAX_FALLING_EGGS];
    bool fallingEggActive[MAX_FALLING_EGGS];
    int activeFallingEggs;
    static const int MAX_POP_EGGS = ROWS * COLS;
    static const uint32_t POP_ANIMATION_MS = 160U;
    touchgfx::Image popEggImages[MAX_POP_EGGS];
    uint32_t popEggStartTick[MAX_POP_EGGS];
    bool popEggActive[MAX_POP_EGGS];
    int activePopEggs;
    int score;
    Unicode::UnicodeChar scoreBuffer[10];
    int shotCount;
    int gridPhase;
    static const int SHOTS_BEFORE_DROP = 5;
    void onEggShot();
    bool checkGameOver();
    void triggerGameOver();

    static const int EGG_HEIGHT = 32;
    static const int EGG_SPACING_Y = 26;
    static const int DANGER_LINE_Y = 220;

    // Biến joystick
    int16_t joystickX, joystickY;
    int16_t aimAngle;
    uint8_t currentEggColor; // Màu của viên đạn hiện tại (nextEgg)
    uint8_t projectileColor ; // mùa của viên đang bay

    // Biến trạng thái viên đạn
    float projectileX, projectileY;
    float projectileVX, projectileVY;
    bool projectileActive;
    bool lastButtonPressed = false;
    uint32_t lastShotButtonTick = 0;

    // Hình ảnh viên đạn
    touchgfx::Image projectileImage;
    bool pendingGroupClear = false;
    int pendingRow = -1;
    int pendingCol = -1;
    static const int MAX_DROP_LINES = 5;   // hạ quá 5 dòng thì thua
    int droppedLineCount;

    // Calibration
    int16_t joystickCenterX = 0;
    int16_t joystickCenterY = 0;
    bool isCalibrated = false;

    // Cấu hình joystick
    static constexpr int16_t JOYSTICK_DEADZONE = 200;
    static constexpr int16_t JOYSTICK_MAX_RANGE = 800;

    // Hàm xử lý logic
    void clearNextEgg();
    void updateJoystickInput();
    void updateAimDirection();
    void updateCannonAndEggPosition();
    void updateAimLine();
    void updateAimVisual();
    bool shootEgg();
    void createProjectile(int x, int y, float vx, float vy);
    void updateProjectile();
    void checkProjectileCollision();
    void updateProjectileVisual();
    void testJoystickWithNextEgg();
    void debugNextEgg();
    void resetJoystickCalibration();
    // Thêm đúng tham số khớp với .cpp
    void handleCollisionWithEgg(int hitRow, int hitCol);
    void findAndRemoveMatchingGroup(int row, int col);
    int detachUnsupportedEggs();
    void updateFallingEggs();
    void startPopAnimation(int row, int col, uint8_t color);
    void updatePopAnimations();
    void debugJoystick();
    void debugAiming();
    void smoothAngleToTarget();
    void setAimAngle(float angle);
    void resetAimToCenter();
    int16_t  applySmoothCurve(int16_t value, int16_t maxRange);
    int getCannonBaseX();
    int getCannonBaseY();
    bool isShiftedRow(int row) const;
    bool isValidGridPosition(int row, int col) const;
    void forceResetGame();
    // THÊM CÁC BIẾN MỚI CHO SMOOTH AIMING:
        float targetAngle;        // Góc đích muốn đạt tới
        float currentAngleFloat;  // Góc hiện tại (dạng float để tính toán mượt)
        bool isAiming;           // Có đang ngắm không
        uint32_t lastAimTime;    // Thời gian lần cuối ngắm

        // Hằng số điều chỉnh
        static const int STOP_AIM_DELAY = 200;      // 200ms sau khi dừng mới dừng ngắm
        static constexpr float SMOOTH_FACTOR = 0.15f;
        static constexpr float ANGLE_THRESHOLD = 0.5f;
        int findBestAttachPosition(int hitRow, int hitCol);
        int findBestAttachColumn(int targetRow, int preferredCol);
        void handleFailedAttachment(int col);
        void handleBottomCollision(int col);
        void emergencyStopProjectile();

};

#endif // SCREEN2VIEW_HPP
