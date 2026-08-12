
#include "app_main.h"
#include "bsp_hw.h"
#include "micro_ao.h"
#include <string.h> // Dùng cho hàm strstr
#include <stdint.h>

/* ====================================================================
 * 1. KHỞI TẠO BỘ NÃO (STATE MACHINE)
 * ==================================================================== */
// Đây là nơi cấp phát bộ nhớ thật sự cho App_AO (ative object)
// File stm32f1xx_it.c sẽ gọi sang đây để xài ké biến này.

Active App_AO; 


/* ====================================================================
 * 2. CÁC BIẾN CỤC BỘ CHO BỘ ĐỆM (BUFFER)
 * ==================================================================== */
#define MAX_CMD_LENGTH 64 //kích thước buffer chứa các ký tự đọc được
static char rx_buffer[MAX_CMD_LENGTH]; //static khai báo các biến chỉ sử dụng trong file .c này -> tránh bị thay đổi bởi các file khác
static uint16_t rx_index = 0;


/* ====================================================================
 * 3. HÀM PARSER: BÓC TÁCH LỆNH VÀ RA QUYẾT ĐỊNH
 * ==================================================================== */
// hàm này được gọi khi đã nhận đủ 1 câu lệnh từ ESP8266 (kết thúc bằng \n)
static void ESP8266_ParseCommand(const char* cmd_str)
{
    // tìm chữ "ALARM_FIRE" trong chuỗi nhận được
    if (strstr(cmd_str, "ALARM_FIRE") != NULL) 
    {
        Event evt;
        evt.sig = FIRE_DETECTED_SIG; 
        evt.param = 0;               
        Active_post(&App_AO, evt);   // báo cho hệ thống biết có cháy!
    }
    // tìm chữ "ALARM_GAS" trong chuỗi nhận được
    else if (strstr(cmd_str, "ALARM_GAS") != NULL) 
    {
        Event evt;
        evt.sig = GAS_DETECTED_SIG;  
        evt.param = 0;
        Active_post(&App_AO, evt);   // báo cho hệ thống biết xì gas!
    }
    // các case khác nếu có 
}


/* ====================================================================
 * 4. HÀM GHÉP BYTE (GỌI TỪ STATE MACHINE HOẶC VÒNG LẶP CHÍNH)
 * ==================================================================== */
// hàm này có nhiệm vụ nhận từng byte lẻ tẻ do ngắt gửi tới và ghép thành chuỗi
void App_Process_UART_Byte(uint8_t received_byte)
{
    // vượt quá kích thước buffer thì reset về 0
    if (rx_index >= MAX_CMD_LENGTH - 1) 
        rx_index = 0; 
    

    //kiểm tra xem ký tự nhận được có phải là Enter (kết thúc chuỗi) không
    if (received_byte == '\n' || received_byte == '\r')
    {
        //chỉ xử lý nếu chuỗi có dữ liệu (lớn hơn 0)
        if (rx_index > 0)
        {
            rx_buffer[rx_index] = '\0';       // kết thúc chuỗi
            ESP8266_ParseCommand(rx_buffer);  // đưa chuỗi nguyên vẹn đi kiểm tra xem là tín hiệu gì
            rx_index = 0;                     // reset con trỏ để chuẩn bị nhận câu lệnh mới
        }
    }
    else//nếu là ký tự bình thường (A, B, C...), lưu vào mảng và tăng index
    {
        rx_buffer[rx_index] = (char)received_byte;
        rx_index++;
    }
}
/* ====================================================================
 * 5. CÁC TRẠNG THÁI (STATE HANDLERS)
 * ==================================================================== */
/* 
 * Hàm: State_Idle
 * Chức năng: Trạng thái bình thường của hệ thống. Đứng đợi sự kiện mạng và cảm biến.
 */
void State_Idle(Active * const me, const Event * const e)
{
    switch (e->sig)
    {
        case ENTRY_SIG:
            // Sẽ chạy 1 lần duy nhất khi hệ thống vừa bước vào State_Idle
            // TODO: In ra màn hình console (qua UART) là "System is Normal"

            break;

        case UART_RX_SIG:
            /*
             * lý do sử dụng: Tín hiệu này do ngắt HAL_UART_RxCpltCallback bắn ra mỗi khi nhận 1 byte.
             * Ở file ngắt, ta đã ép byte nhận được vào e.param (evt.param = rx_byte).
             * Vì vậy ở đây, ta lôi e->param ra, ép kiểu về uint8_t, và đưa vào hàm ghép chuỗi.
             */
            App_Process_UART_Byte((uint8_t)e->param);
            break;

        case FIRE_DETECTED_SIG:
        case GAS_DETECTED_SIG:
            /*
             * lý do sử dụng: Tín hiệu này do hàm ESP8266_ParseCommand (nằm ngay phía trên của file này)
             * bắn ra khi nó đọc được chữ "ALARM_FIRE" hoặc "ALARM_GAS" từ ESP8266.
             * Khi nhận được, ta gọi hàm Active_tran (nằm ở micro_ao.c) để ép hệ thống 
             * nhảy sang trạng thái báo động (State_Alarm).
             */
            Active_tran(me, State_Alarm);
            break;
            
        case MODE_SWITCH_SIG:
            /*
             * Chuyển sang chế độ thủ công (sẽ kích hoạt khi người dùng bấm nút)
             */
            Active_tran(me, State_Manual_Control);
            break;

        default:
            break;
    }
}
/* 
 * Hàm: State_Alarm
 * Chức năng: Chốt cửa (Servo), bật bơm (Pump), khóa mọi thao tác cho tới khi reset.
 */
void State_Alarm(Active * const me, const Event * const e)
{
    switch (e->sig)
    {
        case ENTRY_SIG:
            //Chạy 1 lần ngay khi vừa từ State_Idle nhảy sang đây.    
            BSP_SetServoAngle(90); // Mở chốt cửa 90 độ cho người thoát hiểm
            BSP_SetPump(true);     // Bật bơm chữa cháy
            break;

        case UART_RX_SIG:

            //vẫn phải nhận UART để nhỡ ESP8266 có gửi lệnh RESET thì còn biết
            App_Process_UART_Byte((uint8_t)e->param);
            break;

        case RESET_SIG:
            /*
             * Giả sử nút bấm hoặc lệnh từ web yêu cầu Reset->chuyển nó về lại State_Idle.
             */
            Active_tran(me, State_Idle);
            break;

        case EXIT_SIG:
             //Chạy 1 lần trước khi thoát khỏi State_Alarm để về Idle
            BSP_SetServoAngle(0);  // Đóng chốt cửa lại
            BSP_SetPump(false);    // Tắt bơm
            break;

        default:
            // ignore mọi sự kiện khác (ví dụ phớt lờ Joystick khi đang có cháy)
            break;
    }
}
/* 
 * Hàm: State_Manual_Control
 * Chức năng: Cho phép người dùng dùng Joystick để điều khiển Servo.
 */
void State_Manual_Control(Active * const me, const Event * const e)
{
    switch (e->sig)
    {
        case ENTRY_SIG:
            // TODO: In ra màn hình "Manual Mode ON"
            break;

        case JOYSTICK_MOVED_SIG:
            /*
             * Sẽ được gọi khi phát hiện Joystick thay đổi tọa độ
             * Biến e->param sẽ chứa góc quay mới.
             * TODO: Gọi BSP_SetServoAngle((uint8_t)e->param)
             */
            BSP_SetServoAngle((uint8_t)e->param);
            break;
            
        case MODE_SWITCH_SIG:
            //nhấn nút lần nữa thì quay về Idle
            Active_tran(me, State_Idle);
            break;

            //vẫn phải bắt tín hiệu cháy nổ, bởi vì đang ở chế độ thủ công mà có cháy
            //thì vẫn phải nhảy sang Alarm ngay lập tức
        case FIRE_DETECTED_SIG:
        case GAS_DETECTED_SIG:
            Active_tran(me, State_Alarm);
            break;

        default:
            break;
    }
}