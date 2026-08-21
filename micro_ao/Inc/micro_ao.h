#ifndef MICRO_AO_H
#define MICRO_AO_H

#include <stdint.h>
#include <stdbool.h>

// định nghĩa signals
typedef enum {
    //tín hiệu hệ thống
    INIT_SIG,       //khởi tạo state
    ENTRY_SIG,      //vừa bước vào state
    EXIT_SIG,       //sắp thoát khỏi state
    
    //tín hiệu ứng dụng của dự án
    FIRE_DETECTED_SIG,      //tín hiệu có cháy
    GAS_DETECTED_SIG,       //tín hiệu rò gas
    TEMP_HIGH_SIG,          //tín hiệu nhiệt độ tăng cao
    JOYSTICK_MOVED_SIG,     //tín hiệu joystick chuyển động
    MODE_SWITCH_SIG,        //tín hiệu đổi state
    RESET_SIG,              //tín hiệu reset từ alarm về idle
    UART_RX_SIG,       //tín hiệu nhận 1 byte uart qua esp8266
} Signal;

// struct Event
typedef struct {
    Signal sig;
    uint32_t param; //tham số đi kèm (VD: chứa nhiệt độ 35 độ, hoặc góc servo)
} Event;

/* KiỂU CON TRỎ HÀM TRẠNG THÁI */
struct Active; //Forward declaration
typedef void (*StateHandler)(struct Active * const me, const Event * const e);

/* HÀNG ĐỢI VÒNG (cấu trúc chính)*/
#define QUEUE_SIZE 16 //kích thước hàng đợi (phải là lũy thừa của 2 để tối ưu)

typedef struct Active {
    StateHandler state;         //trạng thái hiện tại
    Event queue[QUEUE_SIZE];    //mảng chứa các sự kiện
    uint8_t head;               //con trỏ ghi (push event vào)
    uint8_t tail;               //con trỏ đọc (pop event ra để xử lý)
    uint8_t count;              //số lượng sự kiện đang tồn đọng
} Active;

/* HÀM API GIAO TIẾP */
void Active_ctor(Active * const me, StateHandler initial); // hàm khởi tạo (constructor)
void Active_init(Active * const me);                       // kích nổ State Machine
void Active_post(Active * const me, Event e);              // bỏ sự kiện vào hàng đợi (dùng trong Ngắt)
void Active_dispatch(Active * const me);                   // lấy sự kiện ra xử lý (dùng trong while(1))
void Active_tran(Active * const me, StateHandler target);  // chuyển trạng thái

#endif // MICRO_AO_H