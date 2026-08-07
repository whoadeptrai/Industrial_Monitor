#ifndef BSP_HW_H
#define BSP_HW_H

#include <stdint.h>
#include <stdbool.h>


void BSP_HW_Init(void);

bool BSP_GetFireStatus(void);
bool BSP_GetGasStatus(void);
uint8_t BSP_ReadDHT11(uint8_t *temp, uint8_t *hum);
void BSP_GetJoystickXY(uint16_t *x, uint16_t *y);
void BSP_SetServoAngle(uint8_t angle);

#endif