#ifndef BSP_HW_H
#define BSP_HW_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>


bool BSP_GetFireStatus(void);
bool BSP_GetGasStatus(void);

void BSP_SetServoAngle(uint8_t angle);

void BSP_Init(void);

void BSP_GetJoystickXY(int8_t* x, int8_t* y);

void Delay_us(uint16_t us);

uint8_t BSP_ReadDHT11(uint8_t *temp, uint8_t *hum);



#define LED_COLOR_RED    0
#define LED_COLOR_GREEN  1
#define LED_COLOR_YELLOW 2

#define LED_ON           1
#define LED_OFF          0

// Nguyên mẫu hàm Đầu ra (Outputs)
void BSP_Pump_Start(void);
void BSP_Pump_Stop(void);
void BSP_LED_Control(uint8_t color, uint8_t state);
void BSP_Buzzer_On(void);
void BSP_Buzzer_Off(void);

#endif
