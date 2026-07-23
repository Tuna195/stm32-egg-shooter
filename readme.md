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

__Đề bài__: Game bắn trứng khủng long

__Mục tiêu__: Phát triển một trò chơi tương tác trên nền tảng nhúng (STM32), sử dụng giao diện đồ họa TouchGFX, điều khiển bằng joystick, có âm thanh và logic game hoàn chỉnh.

__Sản phẩm:__
1. **Điều khiển hướng súng bằng joystick**  
   – Người chơi có thể xoay hướng nòng súng theo ý muốn bằng cần joystick (analog).  
   – Hai trục được lấy mẫu bằng ADC + DMA vòng tròn, kích bởi TIM3, sau đó chuyển đổi thành góc bắn theo thời gian thực.
   – Hướng nòng súng sẽ thay đổi mượt mà tùy theo độ lệch của cần joystick.

2. **Âm thanh hiệu ứng**  
   – Phát âm thanh mỗi khi người chơi bắn trứng, khi nhóm trứng bị xóa hoặc khi trò chơi kết thúc (Game Over).  
   – Âm thanh phát từ buzzer thông qua điều khiển GPIO, kết hợp với hiệu ứng trên giao diện.

3. **Tạo màn chơi ngẫu nhiên **  
   – Mỗi lần bắt đầu game, lưới trứng được sinh ra ngẫu nhiên với các màu khác nhau.  
   – Dữ liệu được sinh bởi bộ sinh số ngẫu nhiên của STM32, đảm bảo tính bất ngờ và tăng độ thử thách.  
   – Các nhóm trứng sẽ xuất hiện với nhiều tổ hợp khác nhau ở mỗi lượt chơi.

4. **Hiển thị giao diện bằng TouchGFX**  
   – Giao diện sinh động, có hoạt ảnh khi đạn bay, trứng nổ hoặc khi chuyển giữa các màn hình.  
   – Cập nhật liên tục trong `tick()` với tốc độ ổn định, tạo trải nghiệm chơi game mượt mà.  
   – Bitmap của các quả trứng thay đổi tương ứng theo màu sắc.

5. **Logic trò chơi hoàn chỉnh**  
   – Tự động phát hiện nhóm trứng cùng màu khi đạn chạm và loại bỏ nếu đủ 3 quả.  
   – Quản lý trạng thái lưới, cập nhật điểm số, phát hiện game over.  
   – Có xử lý hiệu ứng phản xạ nếu trứng chạm mép trái/phải.

6. **ADC DMA và nút bắn bằng ngắt**
   – Trục X/Y được ADC lấy mẫu nền bằng DMA; giao diện chỉ đọc giá trị trung bình trong `handleTickEvent()`.
   – Nút SW của joystick dùng EXTI cạnh xuống, chống dội rồi gửi sự kiện bắn qua CMSIS-RTOS message queue.
   – ISR không gọi trực tiếp TouchGFX và không truyền UART blocking.


_Ảnh chụp minh họa:_
   - Screen1:
     ![z6779802892988_abbf0531b1280fcf7b581d81a7bfb1a6](https://github.com/user-attachments/assets/b541439d-704d-4d1a-8eeb-7e57409dce4c)
   - Screen2:
     ![z6779796516187_98677983efed2f49631b07eb1d42190b](https://github.com/user-attachments/assets/3f175188-a8fc-412a-b267-f47e819930a7)
   - Screen3:
     ![z6779796516963_8ce38e9d6363994f6a36c4069e4eb2d2](https://github.com/user-attachments/assets/8a099e5f-6a2b-4d3a-a6d3-c83c717da2f1)

     

     

 
## TÁC GIẢ

- Tên nhóm: Chập mạch
- Thành viên trong nhóm
  |STT|Họ tên|MSSV|Công việc|
  |--:|--|--|--|
  |1| Mai Quốc Đạt|20225277|hiện thị oled, hiệu ứng, và xử lý logic va chạm, xử lý logic RNG, xử lý logic xoá trứng và update điểm|
  |2| Dương Đăng Duy|20225185|Kết nối, cấu hình điều khiển Joystick, còi Buzzer; cài đặt góc bắn ; xử lý hiệu ứng viên đạn bay và va chạm|
  |1| Bùi Gia Lộc|20225142|hiện thị oled, hiệu ứng, và xử lý ngắt|
  

## MÔI TRƯỜNG HOẠT ĐỘNG

- **Kit sử dụng**: `STM32F429 Discovery Kit`

- **Module, linh kiện sử dụng**:
  - `Joystick 2 trục`: điều khiển hướng bắn của súng
  - `Màn hình TFT LCD`: hiển thị giao diện trò chơi và hiệu ứng
  - `Buzzer (còi)`: phát âm thanh khi bắn hoặc Game Over
  - `Nút nhấn`: dùng để bắt đầu lại game hoặc thực hiện bắn
  - `Nguồn USB 5V`: cấp nguồn cho kit hoạt động

## SO ĐỒ SCHEMATIC

### Ghép nối STM32 với joystick
|STM32F429| Joystick|
|--|--|
|VCC|3V|
|GND|GND|
|PC3 (ADC1 IN13)| X|
|PA5 (ADC2 IN5)| Y|
|PG3 (GPIO EXTI3, pull-up)|SW|

### Ghép nối  STM32 với còi Buzzer
|STM32F429| Còi Buzzer|
|--|--|
|PD12|3V|
|GND|GND|






### TÍCH HỢP HỆ THỐNG

#### Phần cứng

- **Kit điều khiển chính**: `STM32F429 Discovery Kit` – xử lý toàn bộ logic trò chơi, hiển thị và điều khiển ngoại vi.
- **Joystick 2 trục**: nhập điều khiển từ người chơi để xoay hướng bắn.
- **Màn hình TFT LCD**: hiển thị đồ họa trò chơi, trạng thái đạn, trứng và Game Over.
- **Buzzer (còi)**: tạo hiệu ứng âm thanh khi bắn hoặc kết thúc trò chơi.
- **Nút nhấn**: thực hiện thao tác bắn hoặc reset lại màn chơi.
- **Nguồn USB 5V**: cấp nguồn cho toàn hệ thống.

#### Phần mềm

 - **TouchGFX (STM32F429)**  
  Thiết kế giao diện người dùng (UI), bao gồm các màn hình game như `Screen2` (chơi game), `Screen3` (Game Over), xử lý hiệu ứng chuyển cảnh, cập nhật điểm số, và điều khiển logic game theo từng khung hình (`tick()`).

- **Firmware C/C++ (STM32 HAL)**  
  Điều khiển phần cứng như LCD, phát âm thanh bằng PWM hoặc DAC, xử lý va chạm giữa đạn và trứng, điều hướng đạn, chuyển đổi giữa các màn hình.  
  Chịu trách nhiệm:
  - Đọc giá trị joystick qua ADC.
  - Xác định hướng bắn từ giá trị joystick.
  - Điều khiển logic game như sinh trứng mới, kiểm tra va chạm, xử lý Game Over.

- **STM32CubeIDE (IDE phát triển firmware)**  
  - Viết và quản lý code C/C++ sử dụng HAL.
  - Tích hợp với mã được sinh ra từ TouchGFX (`ScreenView.cpp`, `Model.cpp`, `FrontendApplication.cpp`...).
  - Hỗ trợ build project, debug, nạp firmware vào MCU STM32F429.

- **Joystick + ADC DMA + EXTI**
  - Dùng 2 kênh ADC để đọc giá trị trục X và Y từ joystick.  
  - TIM3 kích ADC1/ADC2; DMA2 Stream0/Stream2 ghi liên tục vào buffer vòng tròn.
  - `handleTickEvent()` đọc giá trị trung bình từ buffer để cập nhật hướng bắn mà không chờ ADC.
  - Nút SW trên PG3 phát ngắt EXTI3, gửi event vào queue để TouchGFX thực hiện bắn an toàn trong GUI task.

- **RNG (Random Number Generator)**  
  - Tạo ra các đặc tính ngẫu nhiên cho từng màn chơi như màu sắc, vị trí trứng hoặc hướng rơi.  
  - Sử dụng HAL_RNG_GenerateRandomNumber() để tạo màu ngẫu nhiên (có fallback là dùng HAL_GetTick())  
  - Giúp tăng độ thử thách và tính đa dạng cho trò chơi.

- **Timer (TouchGFX tick)**  
  - Bộ định thời (timer) được cấu hình để tạo ra ngắt định kỳ.  
  - Trong ngắt, gọi `MX_TouchGFX_Process()` để kích hoạt hệ thống tick của TouchGFX.  
  - Mỗi tick thực hiện:
    - Đọc joystick.
    - Cập nhật đạn và vật thể.
    - Kiểm tra va chạm.
    - Vẽ lại giao diện.

### ĐẶC TẢ HÀM

- Giải thích một số hàm quan trọng: ý nghĩa của hàm, tham số vào, ra

  ```C
     /**
     * Hàm được gọi định kỳ mỗi khung hình (tick) của TouchGFX.
  * Chịu trách nhiệm cập nhật toàn bộ logic chính của màn chơi:
  *  - Đọc hướng bắn từ joystick.
  *  - Cập nhật vị trí đạn đang bay.
  *  - Hiển thị góc bắn để debug.
  *  - Nếu có nhóm trứng cần xóa, gọi hàm xử lý nhóm và chuẩn bị đạn mới.
  */
  void Screen2View::handleTickEvent();
  ```
   ```C
    /**
  * Kiểm tra xem đạn đang bay có va chạm với quả trứng nào trên lưới không.
  * Nếu va chạm, dừng đạn lại và xử lý va chạm tại vị trí phù hợp.
  * Nếu không va chạm trứng nào nhưng chạm trần, gắn đạn vào hàng trên cùng.
  *
  * Logic chính:
  * - Duyệt qua toàn bộ lưới trứng, tính khoảng cách từ tâm đạn đến từng trứng.
  * - Nếu khoảng cách nhỏ hơn bán kính va chạm -> xử lý va chạm.
  * - Nếu không trúng trứng nào mà đạn chạm trần -> xử lý gắn vào hàng đầu.
   *
  * Điều kiện đầu vào:
  * - `projectileActive == true`: nghĩa là đang có đạn bay.
  *
  * Dữ liệu liên quan:
  * - `eggGrid[row][col]`: ma trận lưu trạng thái quả trứng.
   * - `projectileX`, `projectileY`: vị trí hiện tại của đạn.
  *
  * Tác vụ:
  * - Ẩn đạn khi kết thúc va chạm.
  * - Gửi tín hiệu debug qua UART.
  * - Gọi `handleCollisionWithEgg(row, col)` để xử lý thêm.
  */
  void Screen2View::checkProjectileCollision();


  ```
   ```C
    /**
  * Tìm và loại bỏ nhóm trứng cùng màu liền kề từ vị trí chỉ định.
  *
   * @param row  Hàng bắt đầu kiểm tra.
  * @param col  Cột bắt đầu kiểm tra.
  *
  * Chức năng:
  * - Duyệt theo kiểu DFS để tìm tất cả các quả trứng cùng màu kết nối liền nhau (6 hướng).
  * - Nếu số lượng trứng >= 3 -> xóa toàn bộ nhóm này khỏi lưới.
  * - Cập nhật điểm, vẽ lại lưới, kiểm tra điều kiện Game Over.
  *
  * Chi tiết xử lý:
  * - Sử dụng mảng `visited` để đánh dấu các ô đã xét.
  * - Duyệt theo 6 hướng tùy theo hàng chẵn/lẻ (mô hình tổ ong).
  * - Gọi `updateScore()`, `renderEggGrid()`, `triggerGameOver()` nếu cần.
  * - Gửi chuỗi "Group cleared!" qua UART để debug.
  *
   * Điều kiện không xử lý:
  * - Vị trí không hợp lệ (`!isValidGridPosition()`).
  * - Trứng rỗng (`eggGrid[row][col] == EMPTY`).
  */
  void Screen2View::findAndRemoveMatchingGroup(int row, int col);


  ```
   ```C
     /**
   /**
  * Đọc giá trị joystick từ ADC, xử lý hướng và nút nhấn để điều khiển game.
  *
  * - Đọc giá trị trục X/Y từ ADC (channel 13 & 5).
  * - Lần đầu chạy sẽ calibrate vị trí trung tâm làm gốc.
  * - Tính rawX, rawY từ chênh lệch so với gốc.
  * - Áp dụng deadzone, giới hạn và làm mượt tín hiệu đầu vào.
  * - Ghi kết quả vào biến joystickX, joystickY để điều hướng bắn.
  * - Đọc trạng thái nút nhấn SW (GPIOC PIN_2), nếu nhấn thì gọi `shootEgg()` và bật buzzer.
  * - Gửi log qua UART (huart1) để debug calibration và nút nhấn.
   *
  * @note Giao tiếp với phần cứng: ADC, GPIO, UART, buzzer.
  */
  void Screen2View::updateJoystickInput();


  ```
   ```C
    /**
  * Bắn quả trứng từ đầu nòng súng theo hướng hiện tại của aimAngle.
  *
  * - Ẩn quả trứng tiếp theo (`nextEgg`) sau khi bắn.
  * - Tính toán góc bắn (radian) từ aimAngle.
  * - Xác định vị trí bắt đầu của viên đạn dựa trên góc và khoảng cách đầu nòng súng.
  * - Tính hướng bay (dirX, dirY) và tốc độ ban đầu.
   * - Gọi `createProjectile()` để tạo đạn bay theo hướng vừa tính.
  *
  * @note Phối hợp giữa giao diện TouchGFX và logic vật lý đạn.
  */
  void Screen2View::shootEgg();


  ```
   ```C
     /**
  * Khởi tạo viên đạn mới khi người chơi bắn trứng.
  *
  * - Lưu vị trí và vận tốc ban đầu của đạn (x, y, vx, vy).
  * - Đánh dấu đạn đang hoạt động (`projectileActive = true`).
  * - Hiển thị ảnh đạn tại vị trí tương ứng, căn giữa theo kích thước ảnh.
  * - Cập nhật màu và bitmap hiển thị của đạn theo màu quả trứng hiện tại (`currentEggColor`).
  *
  * @param x   Vị trí X khởi đầu của viên đạn.
    * @param y   Vị trí Y khởi đầu của viên đạn.
  * @param vx  Vận tốc theo trục X.
  * @param vy  Vận tốc theo trục Y.
  */
  void Screen2View::createProjectile(int x, int y, float vx, float vy);


  ```

    ```C
    /**
   * Vẽ lại toàn bộ lưới trứng trên màn hình dựa vào trạng thái eggGrid.
  *
  * - Lấy vị trí gốc của container2 để căn chỉnh toàn bộ lưới.
  * - Với mỗi ô chứa trứng, chọn bitmap tương ứng theo màu (RED, BLUE, GREEN, ...).
  * - Tính toán vị trí từng quả theo cấu trúc tổ ong (hexagonal pattern).
  * - Đặt vị trí và hiển thị ảnh quả trứng tại đúng vị trí.
  * - Ẩn các ô trống (EMPTY).
  * - Gọi `invalidate()` để cập nhật lại giao diện sau khi thay đổi.
  *
  * @note Sử dụng các hằng số như `EGG_SPACING_X`, `EGG_SPACING_Y`, `HEX_OFFSET` để tạo layout tổ ong.
  */
  void Screen2View::renderEggGrid();



  ```

     ```C
   /**
  * Di chuyển toàn bộ lưới trứng xuống 1 hàng và thêm hàng mới ở trên.
  *
  * - Dịch chuyển tất cả trứng từ hàng [0..ROWS-2] xuống hàng [1..ROWS-1].
  * - Gọi `addNewTopRow()` để thêm một hàng trứng mới ở hàng trên cùng (có thể sinh ngẫu nhiên).
  * - Gọi `renderEggGrid()` để cập nhật lại giao diện người dùng sau khi thay đổi.
  *
  * @note Dùng để tăng độ khó hoặc tạo thử thách sau một số lần bắn.
  */
  void Screen2View::dropEggGrid();




  ```
  
### KẾT QUẢ

[![Xem Video Demo](https://img.youtube.com/vi/S8OTtfaylVM/0.jpg)](https://www.youtube.com/watch?v=S8OTtfaylVM)  
📺 **Video demo**: Game bắn trứng khủng long điều khiển bằng joystick (STM32 + TouchGFX).
