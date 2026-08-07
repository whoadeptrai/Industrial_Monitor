#ifndef MICRO_AO_H
#define MICRO_AO_H

#include <stdint.h>
#include <stdbool.h>

/* --- 1. ĐỊNH NGHĨA TÍN HIỆU (SIGNALS) --- */
typedef enum {
    INIT_SIG,       // Khởi tạo State
    ENTRY_SIG,      // Vừa bước vào State
    EXIT_SIG,       // Sắp thoát khỏi State
    
    // --- Các tín hiệu hệ thống của dự án ---
    FIRE_DETECTED_SIG,
    GAS_DETECTED_SIG,
    TEMP_HIGH_SIG,
    JOYSTICK_MOVED_SIG,
    MODE_SWITCH_SIG,
    RESET_SIG,
    SIG_UART_RX_BYTE,
    

    UART_RX_SIG     // tín hiệu nhận của uart
} Signal;

/* --- 2. CẤU TRÚC SỰ KIỆN (EVENT) --- */
typedef struct {
    Signal sig;
    uint32_t param; // Tham số đi kèm (VD: chứa nhiệt độ 35 độ, hoặc góc servo)
} Event;

/* --- 3. KIỂU CON TRỎ HÀM TRẠNG THÁI --- */
struct Active; // Khai báo trước (Forward declaration)
typedef void (*StateHandler)(struct Active * const me, const Event * const e);

/* --- 4. HÀNG ĐỢI VÒNG (RING BUFFER) --- */
#define QUEUE_SIZE 16 // Kích thước hàng đợi (phải là lũy thừa của 2 để tối ưu)

typedef struct Active {
    StateHandler state;         // Trạng thái hiện tại
    Event queue[QUEUE_SIZE];    // Mảng chứa sự kiện
    uint8_t head;               // Con trỏ ghi (Push)
    uint8_t tail;               // Con trỏ đọc (Pop)
    uint8_t count;              // Số lượng sự kiện đang tồn đọng
} Active;

/* --- 5. HÀM API GIAO TIẾP --- */
void Active_ctor(Active * const me, StateHandler initial); // Hàm khởi tạo (Constructor)
void Active_init(Active * const me);                       // Kích nổ State Machine
void Active_post(Active * const me, Event e);              // Bỏ sự kiện vào hàng đợi (Dùng trong Ngắt)
void Active_dispatch(Active * const me);                   // Lấy sự kiện ra xử lý (Dùng trong while 1)
void Active_tran(Active * const me, StateHandler target);  // Chuyển trạng thái

#endif // MICRO_AO_H