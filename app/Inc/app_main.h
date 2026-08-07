#ifndef APP_MAIN_H
#define APP_MAIN_H

#include "stdint.h"
#include "stdbool.h"
#include "micro_ao.h" 
#include "bsp_hw.h"   


extern Active App_AO; 

void App_AO_Init(void);
void App_Process_UART_Byte(uint8_t received_byte);

#endif // APP_MAIN_H