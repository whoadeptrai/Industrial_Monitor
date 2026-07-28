#include "main.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    OFF = 0,
    ON = 1
} Sensor_Status;
Sensor_Status BSP_GetFireStatus(void);
Sensor_Status BSP_GetGasStatus(void);

void BSP_SetServoAngle(uint8_t angle);

void BSP_Init(void);

void BSP_GetJoystickXY(int8_t* x, int8_t* y);

void delay_us(uint16_t us);

void BSP_DHT11_Start(void);
bool BSP_DHT11_Read(uint8_t* temp, uint8_t* hum);