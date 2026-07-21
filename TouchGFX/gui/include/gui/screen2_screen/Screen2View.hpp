#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/Color.hpp>

#define ROWS 5
#define VISIBLE_ROWS 5
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
    bool hasEggBelowVisible() const;
    void addNewTopRow();
    void notifyGameOver();

private:
    uint8_t eggGrid[ROWS][COLS];
    touchgfx::Image eggImages[ROWS][COLS];
    int score;
    Unicode::UnicodeChar scoreBuffer[10];
    int shotCount;
    static const int SHOTS_BEFORE_DROP = 5;
    void onEggShot();
    bool checkGameOver();
    bool checkBottomRowOccupied();
    void triggerGameOver();

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
    void shootEgg();
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
    void debugJoystick();
    void debugAiming();
    void smoothAngleToTarget();
    void setAimAngle(float angle);
    void resetAimToCenter();
    int16_t  applySmoothCurve(int16_t value, int16_t maxRange);
    int getCannonBaseX();
    int getCannonBaseY();
    bool isValidGridPosition(int row, int col);
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
