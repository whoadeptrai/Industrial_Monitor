#ifndef BSP_HW_H
#define BSP_HW_H

#include "main.h"    // Lấy định nghĩa chân GPIO (FIRE_SENSOR_Pin, SERVO_PWM_Pin...)
#include <stdbool.h> // Hỗ trợ kiểu trả về true/false
#include <stdint.h>

// --------------------------------------------------------
// CÁC HÀM KHỞI TẠO VÀ ĐIỀU KHIỂN PHẦN CỨNG
// --------------------------------------------------------

// Khởi động các ngoại vi (DMA, PWM)
void BSP_Init(void);

// Đọc trạng thái cảm biến (Trùng khớp với Task 1)
bool BSP_GetFireStatus(void);
bool BSP_GetGasStatus(void);

// Điều khiển cơ cấu chấp hành
void BSP_SetServoAngle(uint8_t angle);
void BSP_SetPump(bool state);

// Đọc giá trị Joystick qua ADC + DMA
void BSP_GetJoystickXY(uint16_t* x, uint16_t* y);

// Đọc cảm biến nhiệt độ DHT11
bool BSP_ReadDHT11(uint8_t* temp, uint8_t* hum);

#endif /* BSP_HW_H */