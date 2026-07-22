# 📋 INDUSTRIAL MONITOR - PROJECT TASK DESCRIPTIONS

## 1. TỔNG QUAN DỰ ÁN (PROJECT OVERVIEW)
Hệ thống Giám sát Công nghiệp (Industrial Monitor) sử dụng vi điều khiển trung tâm **STM32F103C8Tx** (ARM Cortex-M3). Dự án được thiết kế theo kiến trúc phần mềm phân lớp, sử dụng **Active Object (State Machine)** để quản lý luồng thực thi và **Ring Buffer $O(1)$** để xử lý dữ liệu bất đồng bộ.

### Kiến trúc thư mục (Không được phép tự ý thay đổi):
*   `Core/` & `Drivers/`: Code khởi tạo phần cứng do STM32CubeMX quản lý (KHÔNG CHỈNH SỬA).
*   `micro_ao/`: Chứa bộ khung lõi Active Object và Hàng đợi (Đã hoàn thiện).
*   `app/`: Chứa logic ứng dụng (`app_main.c`, `bsp_hw.c`). Đây là không gian làm việc chính của team.
*   `scripts/`: Chứa công cụ tự động hóa biên dịch và nạp code.

---

## 2. QUY TẮC LÀM VIỆC (DEVELOPMENT RULES)
Để tránh Merge Conflict và lỗi môi trường, toàn bộ thành viên **BẮT BUỘC** tuân thủ các quy tắc sau:
1.  **Không chạm vào CMake:** Tuyệt đối không chỉnh sửa file `CMakeLists.txt`. Các file `.c` và `.h` cần thiết đã được Tech Lead liên kết sẵn.
2.  **Chỉ code trong phạm vi Task:** Chỉ viết logic vào đúng file được phân công.
3.  **Quy trình Build & Flash:**
    *   Gõ code xong, nhấp đúp vào file `scripts/build_and_flash.bat` để tự động biên dịch bằng **Ninja** và nạp xuống chip qua ST-Link.
    *   Nếu Terminal báo chữ đỏ `[ERROR] Build failed!`, tự quay lại sửa lỗi Cú pháp/Logic, tuyệt đối không push code rác lên Git.

---

## 3. PHÂN CÔNG CÔNG VIỆC (TASK BREAKDOWN)

### 📌 TASK 1: Xây dựng Lớp bọc Phần cứng (Board Support Package - BSP)
*   **Người phụ trách:** [Điền tên thành viên]
*   **File làm việc:** `app/inc/bsp_hw.h` và `app/src/bsp_hw.c`
*   **Mục tiêu:** Cách ly toàn bộ thư viện HAL (Hardware Abstraction Layer) khỏi tầng thuật toán. Tầng trên chỉ gọi hàm giao tiếp, không quan tâm chip chạy như thế nào.
*   **Yêu cầu chi tiết:**
    *   **Cảm biến Lửa & Gas:** Viết hàm `BSP_GetFireStatus()` và `BSP_GetGasStatus()` trả về `true/false` dựa trên trạng thái GPIO.
    *   **Cảm biến DHT11:** Viết hàm `BSP_ReadDHT11(uint8_t* temp, uint8_t* hum)` để lấy dữ liệu nhiệt độ, độ ẩm.
    *   **Joystick (ADC + DMA):** Viết hàm `BSP_GetJoystickXY(uint16_t* x, uint16_t* y)`. Đảm bảo đọc giá trị từ mảng DMA thay vì block CPU chờ ADC.
    *   **Servo Motor:** Viết hàm `BSP_SetServoAngle(uint8_t angle)` thực hiện tính toán và ghi giá trị vào thanh ghi CCR của Timer 4 để băm xung PWM.

### 📌 TASK 2: Xử lý Giao tiếp Mạng & Hàng đợi Dữ liệu (Comms & Buffer)
*   **Người phụ trách:** [Điền tên thành viên]
*   **File làm việc:** `Core/Src/stm32f1xx_it.c` (Phần ngắt UART) và `app/src/app_main.c`
*   **Mục tiêu:** Nhận lệnh điều khiển (AT Commands) từ ESP8266 an toàn, không làm ngắt quãng hệ thống.
*   **Yêu cầu chi tiết:**
    *   **UART Interrupt:** Kích hoạt ngắt nhận UART1 (RX). Mỗi khi có 1 byte truyền về, bọc nó vào struct `Event` và gọi hàm `Active_post()` đẩy vào Ring Buffer.
    *   **ESP8266 Parser:** Viết hàm parse chuỗi ký tự nhận được để trích xuất lệnh. (VD: Nếu nhận được chuỗi báo động, trigger tín hiệu `FIRE_DETECTED_SIG`).
    *   *Lưu ý:* Xử lý đẩy dữ liệu trong ngắt (ISR) phải cực kỳ ngắn gọn (chỉ dùng hàm Push $O(1)$), không dùng các hàm delay hay in printf trong ngắt.

### 📌 TASK 3: Lập trình Luồng Điều khiển Trung tâm (State Machine Logic)
*   **Người phụ trách:** [Tên Tech Lead / Thành viên]
*   **File làm việc:** `app/inc/app_main.h` và `app/src/app_main.c`
*   **Mục tiêu:** Ráp nối phần cứng và truyền thông lại với nhau thông qua bộ não Active Object.
*   **Yêu cầu chi tiết:**
    *   Khởi tạo bộ State Machine bằng hàm `Active_init()`.
    *   Đặt hàm `Active_dispatch()` vào trong vòng lặp vô tận `while(1)` của file `main.c` để liên tục tiêu thụ sự kiện từ Queue.
    *   Viết các hàm trạng thái (State Handlers):
        *   `State_Idle`: Quét định kỳ cảm biến (gọi hàm từ Task 1).
        *   `State_Alarm`: Kích hoạt khi có tín hiệu `FIRE_DETECTED_SIG` hoặc `GAS_DETECTED_SIG`, xoay Servo chốt cửa, đẩy lệnh AT lên server (qua Task 2).
        *   `State_Manual_Control`: Điều khiển Servo trực tiếp bằng tọa độ Joystick.

---
*Mọi thắc mắc về luồng hệ thống hoặc bị lỗi môi trường Build, vui lòng liên hệ trực tiếp Tech Lead trước khi tự ý sửa cấu hình.*