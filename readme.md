> [!IMPORTANT]
> **Sau khi clone/pull:** không dùng lại các file `STM32CubeIDE/Debug/**`
> từ máy khác. Đây là build output do STM32CubeIDE sinh ra và có thể chứa
> đường dẫn tuyệt đối kiểu Windows (`D:/...`) hoặc macOS (`/Users/...`).
> Hãy import project trong thư mục `STM32CubeIDE`, sau đó chọn
> **Project → Clean… → Build Project** để IDE tự sinh `Debug/` phù hợp với
> máy hiện tại. Không commit lại `Debug/`, `Release/` hoặc các file
> `subdir.mk`.
>
> **Dây joystick SW đã chuyển từ PC2 sang PG3.** PC2 là `LCD_CS` của màn hình
> trên STM32F429I-DISCO nên không được dùng làm nút joystick. Nối chân SW của
> joystick vào **PG3**; firmware cấu hình PG3 pull-up và EXTI cạnh xuống.

# GAME BẮN TRỨNG KHỦNG LONG

## GIỚI THIỆU

__Đề bài__: Game bắn trứng khủng long (kiểu *bubble shooter*).

__Mục tiêu__: Phát triển một trò chơi tương tác trên nền tảng nhúng STM32F429,
dùng giao diện đồ họa TouchGFX, điều khiển bằng joystick analog, có phản hồi
rung (haptic) và logic game hoàn chỉnh.

__Tính năng chính__:

1. **Điều khiển hướng bắn bằng joystick (ADC + DMA)**
   – Hai trục X/Y của joystick được lấy mẫu bằng **ADC + DMA vòng tròn**, kích
     nhịp bởi **TIM3** (~1 kHz).
   – GUI chỉ đọc **giá trị trung bình** của buffer trong `handleTickEvent()`,
     không chờ ADC → không chặn khung hình.
   – Có dead-zone dạng tròn (giữ đúng tỉ lệ X/Y) và làm mượt góc; góc bắn giới
     hạn trong khoảng **±80°**, trở về 0° khi thả cần.

2. **Phản hồi rung (Haptic – motor rung PWM)**
   – Dùng **motor rung** điều khiển bằng PWM (`TIM4_CH1` / PD12) thay cho âm
     thanh, theo yêu cầu môn học.
   – Một *pattern engine* phát các rung khác nhau cho từng sự kiện: bắn, xoá
     nhóm trứng, combo, cảnh báo khi hạ lưới, và Game Over.
   – Engine chạy phi chặn: ISR/hàm game chỉ đặt cường độ, việc đếm thời gian do
     `Haptic_Update()` xử lý trong `defaultTask` mỗi 10 ms.

3. **Sinh màn chơi ngẫu nhiên (RNG)**
   – Mỗi ván, lưới trứng được sinh ngẫu nhiên với 5 màu.
   – Dùng `HAL_RNG_GenerateRandomNumber()` (fallback `HAL_GetTick()` nếu RNG
     lỗi) để chọn màu, tăng tính bất ngờ.

4. **Giao diện TouchGFX**
   – Ba màn hình: **Screen1** (bắt đầu), **Screen2** (chơi game), **GameOver**
     (kết thúc, hiển thị điểm và điểm cao nhất).
   – Hoạt ảnh khi đạn bay, trứng nổ (tách đôi) và trứng mất neo rơi xuống.
   – Cập nhật liên tục trong `handleTickEvent()` với tốc độ ổn định.

5. **Logic game hoàn chỉnh**
   – Lưới tổ ong (hex) 8×8, tự phát hiện nhóm trứng cùng màu (DFS 6 hướng); đủ
     **≥ 3 quả** thì xoá cả nhóm.
   – Sau khi xoá, các cụm trứng mất điểm neo với trần sẽ **rơi xuống**.
   – Cộng điểm, **combo** khi xoá nhóm lớn, cập nhật điểm cao nhất.
   – Hạ lưới sau mỗi số phát bắn nhất định; trứng chạm **danger line** → Game
     Over. Đạn phản xạ khi chạm mép trái/phải.

6. **Nút bắn bằng ngắt (EXTI) + hàng đợi**
   – Nút SW joystick trên **PG3** dùng **EXTI cạnh xuống**, chống dội trong ISR
     rồi đẩy sự kiện vào **CMSIS-RTOS message queue**.
   – GUI task lấy sự kiện từ queue để bắn an toàn — **ISR không gọi trực tiếp
     TouchGFX và không truyền UART**.
   – Nút USER tích hợp trên **PA0** dùng làm nút bắn dự phòng.

7. **Lưu điểm cao nhất vào Flash**
   – Điểm cao nhất được ghi bền vào Flash nội (sector 23), đọc lại khi khởi
     động. Việc ghi Flash chạy trong một task riêng (bất đồng bộ) để không làm
     giật giao diện.

## TÁC GIẢ

- Tên nhóm: _(điền)_
- Thành viên và phân công:

  | STT | Họ tên | MSSV | Công việc |
  |--:|--|--|--|
  | 1 | Quang Sơn | _(điền)_ | Input: driver ADC + DMA joystick, calibrate, ghép vào game |
  | 2 | Thành | _(điền)_ | Ngắt EXTI nút bắn + chống dội, ghép nút bắn vào game |
  | 3 | Thắng | _(điền)_ | Haptic: motor rung PWM, pattern engine, ghép rung vào sự kiện game |
  | 4 | Đỗ Sơn | _(điền)_ | Logic game (Screen2View), ghép các phần, ra bản 1P hoàn chỉnh |
  | 5 | Tuấn | _(điền)_ | Leader / UI / cấu hình `.ioc` / build & merge Git |

## MÔI TRƯỜNG HOẠT ĐỘNG

- **Kit sử dụng**: `STM32F429I-DISCO` (STM32F429ZI, màn hình TFT 2.4" tích hợp,
  bộ nhớ Flash 2 MB).

- **Module, linh kiện**:
  - `Joystick 2 trục` (analog + nút SW): điều khiển hướng bắn và bắn.
  - `Màn hình TFT LCD tích hợp` (ILI9341, LTDC): hiển thị game và hiệu ứng.
  - `Motor rung` (module 3 chân có mạch lái): phản hồi haptic.
  - `Nguồn USB 5V`: cấp nguồn cho kit.

## SƠ ĐỒ KẾT NỐI

### Joystick hai trục

| Chân joystick | Chân STM32F429I-DISCO | Chức năng |
|---|---|---|
| `VCC` | `3V3` | Nguồn 3,3 V |
| `GND` | `GND` | Mass chung |
| `X` / `VRx` | `PC3` | `ADC1_IN13`, DMA2 Stream0 (vòng tròn) |
| `Y` / `VRy` | `PA5` | `ADC2_IN5`, DMA2 Stream2 (vòng tròn) |
| `SW` | `PG3` | GPIO pull-up, ngắt `EXTI3` cạnh xuống để bắn |

> [!WARNING]
> Không nối `SW` vào `PC2`. PC2 đang được màn hình tích hợp dùng làm `LCD_CS`.
> Nút USER tích hợp trên `PA0` (`EXTI0`) vẫn dùng được như nút bắn dự phòng.

### Module motor rung ba chân

| Chân module | Chân STM32F429I-DISCO | Chức năng |
|---|---|---|
| `VCC` | `3V3` hoặc nguồn đúng định mức module | Nguồn cho module |
| `GND` | `GND` | Mass chung với STM32 |
| `IN` / `S` | `PD12` | PWM `TIM4_CH1` điều khiển cường độ rung |

> [!CAUTION]
> Bảng trên áp dụng cho **module motor rung ba chân có sẵn mạch driver**. Không
> nối motor rung hai dây trực tiếp vào PD12; cần transistor/MOSFET, diode bảo vệ
> và nguồn phù hợp với dòng của motor.

## KIẾN TRÚC PHẦN MỀM

### Hệ điều hành & luồng (FreeRTOS / CMSIS-RTOS2)

| Task | Nhiệm vụ |
|---|---|
| `GUI_Task` | Chạy TouchGFX: đọc joystick, cập nhật logic game, vẽ giao diện |
| `defaultTask` | Gọi `Haptic_Update(10)` mỗi 10 ms để chạy pattern rung |
| `highScoreTask` | Ghi điểm cao nhất vào Flash khi có yêu cầu (bất đồng bộ) |

`TIM6` được dùng làm nguồn thời gian (time base) cho HAL.

### Luồng dữ liệu điều khiển

```
Joystick X/Y ─(ADC+DMA vòng tròn, TIM3 ~1kHz)─► buffer ─► Joystick_GetADC() (trung bình)
                                                              │
                                                    Screen2View::updateJoystickInput()
                                                              │  (dead-zone, làm mượt góc)
                                                              ▼
                                                        góc bắn (aimAngle)

Nút SW/PG3 ─(EXTI3, chống dội 50ms)─► shootEventQueue ─► Model::tick() ─► performShootAction()
```

### Sinh số ngẫu nhiên

`getRandom32()` dùng `HAL_RNG_GenerateRandomNumber()`, nếu lỗi thì fallback về
`HAL_GetTick()`. `randomColor()` trả về màu 1–5.

### Lưu điểm cao nhất (Flash)

- Vùng lưu: `0x081E0000`, `FLASH_SECTOR_23`, kích thước 128 KiB.
- Mỗi bản ghi gồm `magic` + `score` + `score_inv` (nghịch đảo bit để kiểm tra
  toàn vẹn). Ghi kiểu **append** để tránh xoá sector mỗi lần; chỉ xoá khi sector
  đầy. `magic` được ghi **cuối cùng** nên nếu mất điện giữa chừng, bản ghi dở dang
  bị coi là không hợp lệ và các bản cũ vẫn đọc được.
- `HighScore_RequestSave()` đẩy điểm vào queue để `highScoreTask` ghi Flash, nhờ
  đó GUI không bị khựng.

## ĐẶC TẢ HÀM (Screen2View)

```C
/**
 * Hàm được gọi mỗi khung hình (tick) của TouchGFX. Cập nhật toàn bộ logic:
 *  - Đọc joystick và cập nhật hướng bắn (updateAimDirection).
 *  - Cập nhật viên đạn đang bay (updateProjectile).
 *  - Cập nhật trứng rơi và hoạt ảnh trứng nổ.
 *  - Nếu có nhóm cần xử lý (pendingGroupClear) thì tìm & xoá nhóm, kiểm tra
 *    danger line, rồi chuẩn bị lượt bắn kế tiếp.
 */
void Screen2View::handleTickEvent();

/**
 * Đọc joystick từ DMA (trung bình buffer), trừ tâm danh định (2048), áp dead-zone
 * tròn rồi ghi vào joystickX/joystickY. KHÔNG đọc nút bắn ở đây — nút bắn do EXTI
 * xử lý. (Tên hàm được đặt calibrate ở tâm danh định để người chơi có thể gạt cần
 * ngay khi vào màn.)
 */
void Screen2View::updateJoystickInput();

/**
 * Bắn một viên đạn theo góc aimAngle hiện tại. Trả về true nếu tạo được phát bắn
 * (đang không có đạn bay, không có nhóm chờ xử lý, không có trứng đang rơi/nổ).
 * performShootAction() gọi hàm này rồi phát rung HAPTIC_SHOOT khi bắn thành công.
 */
bool Screen2View::shootEgg();

/**
 * Kiểm tra va chạm giữa đạn đang bay và trứng trên lưới (theo bán kính). Nếu trúng
 * → dừng đạn và gọi handleCollisionWithEgg(). Nếu không trúng mà chạm trần → gắn
 * vào hàng trên cùng.
 */
void Screen2View::checkProjectileCollision();

/**
 * Từ ô (row,col), duyệt DFS 6 hướng (tổ ong, phân biệt hàng chẵn/lẻ) tìm nhóm cùng
 * màu liền kề. Nếu ≥ 3 quả → nổ cả nhóm, cộng điểm (có COMBO_BONUS khi nhóm ≥ 6),
 * gỡ các cụm mất neo (detachUnsupportedEggs), vẽ lại lưới và phát rung tương ứng.
 */
void Screen2View::findAndRemoveMatchingGroup(int row, int col);

/**
 * Hạ toàn bộ lưới xuống 1 hàng, thêm hàng mới ở trên, đảo pha 8/7 ô và phát rung
 * cảnh báo (HAPTIC_WARNING). Gọi sau mỗi SHOTS_BEFORE_DROP phát bắn.
 */
void Screen2View::dropEggGrid();
```

## THÔNG SỐ / HẰNG SỐ CHÍNH

| Tên | Giá trị | Ý nghĩa |
|---|--:|---|
| `ROWS` × `COLS` | 8 × 8 | Kích thước lưới trứng (tổ ong) |
| Số màu trứng | 5 | RED, BLUE, GREEN, YELLOW, PURPLE |
| `POINTS_PER_EGG` | 10 | Điểm mỗi quả bị xoá |
| Ngưỡng xoá nhóm | ≥ 3 | Số quả cùng màu tối thiểu để nổ |
| Ngưỡng combo | ≥ 6 | Nhóm đủ lớn được cộng thêm `COMBO_BONUS_EGGS` |
| `SHOTS_BEFORE_DROP` | 5 | Số phát bắn giữa hai lần hạ lưới |
| `DANGER_LINE_Y` | 220 | Vạch thua (px) |
| `JOYSTICK_DMA_SAMPLES` | 16 | Số mẫu ADC lấy trung bình mỗi trục |
| `JOYSTICK_DEADZONE` | 200 | Bán kính dead-zone (đơn vị ADC) |
| Góc bắn | ±80° | Giới hạn `AIM_MIN/MAX_ANGLE` |

## GHI CHÚ CHO NGƯỜI PHÁT TRIỂN

- **Log debug qua UART đang TẮT** để không chèn `HAL_UART_Transmit` blocking vào
  luồng render (gây giật). Bật lại bằng cách đặt `#define GAME_DEBUG_UART 1` ở đầu
  `TouchGFX/gui/src/screen2_screen/Screen2View.cpp`.
- Cấu hình chân (`.ioc`) do người phụ trách UI/Leader quản lý; tránh sửa `.ioc`
  song song để không xung đột code sinh tự động.

## KẾT QUẢ

_(Cập nhật ảnh chụp màn hình / video demo của nhóm tại đây.)_
