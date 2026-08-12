#ifndef APP_MAIN_H
#define APP_MAIN_H

#include "micro_ao.h" 
#include "main.h"


extern Active App_AO; 


void State_Idle(Active * const me, const Event * const e);
// Khai báo thêm hai state còn lại để State_Idle biết đến sự tồn tại của 2 trạng thái này
void State_Alarm(Active * const me, const Event * const e);
void State_Manual_Control(Active * const me, const Event * const e);
// Hàm nhận và ghép chuỗi (đã được viết ở app_main.c)
void App_Process_UART_Byte(uint8_t received_byte);

#endif /* APP_MAIN_H */