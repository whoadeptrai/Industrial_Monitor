#include "bsp_hw.h"
#include "main.h"
#include "stm32f103xb.h"
#include "stm32f1xx_hal_adc.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_uart.h"
#include <stdint.h>
#include <app_main.h>
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim3;

Sensor_Status BSP_GetFireStatus(void){
    if (HAL_GPIO_ReadPin(FIRE_SENSOR_GPIO_Port, FIRE_SENSOR_Pin) == GPIO_PIN_SET){
        return ON;
    } else return OFF;
}

Sensor_Status BSP_GetGasStatus(void){
    if (HAL_GPIO_ReadPin(GAS_SENSOR_GPIO_Port, GAS_SENSOR_Pin) == GPIO_PIN_SET){
        return ON;
    } else return OFF;
}

void BSP_SetServoAngle(uint8_t angle){
    if (angle > 180) angle = 180;

    uint16_t ccr_value = 500 + ((angle * 2000) / 180);

    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, ccr_value);
}

static volatile uint16_t joystick_adc[2];


extern ADC_HandleTypeDef hadc1;
void BSP_Init(void){
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)joystick_adc, 2);
}

void BSP_GetJoystickXY(int8_t* x, int8_t* y){
    int32_t raw_x, raw_y;
    raw_x = joystick_adc[0];
    raw_y = joystick_adc[1];
    int8_t percent_x = (int8_t)(((raw_x - 2048)*100)/2048);
    int8_t percent_y = (int8_t)(((raw_y - 2048)*100)/2048);
    if (percent_x <= 5 && percent_x >= -5) percent_x = 0;
    if (percent_y <= 5 && percent_y >= -5) percent_y = 0;
    if (x != NULL) *x = percent_x;
    if (y != NULL) *y = percent_y;
}

void delay_us(uint16_t us){
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    HAL_TIM_Base_Start(&htim3);
    while(__HAL_TIM_GET_COUNTER(&htim3) < us){

    }
    HAL_TIM_Base_Stop(&htim3);
}

void BSP_DHT11_Start(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DHT11_PIN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(DHT11_PIN_GPIO_Port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_RESET);

}

bool BSP_DHT11_Read(uint8_t* temp, uint8_t* hum){
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pin = DHT11_PIN_Pin;

    HAL_GPIO_Init(DHT11_PIN_GPIO_Port, &GPIO_InitStruct);

    uint8_t time_out = 0;
    while(HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_RESET ){
        delay_us(1);
        time_out++;
        if(time_out > 100) return false;
    }
    time_out = 0;
    while(HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET ){
        delay_us(1);
        time_out++;
        if(time_out > 100) return false;
    }
    
    uint8_t data[5] = {0};

    // 1. Vòng lặp đếm Byte (5 hộp)
    for (uint8_t i = 0; i < 5; i++) {

        // 2. Vòng lặp đếm Bit (8 mảnh vỡ)
        for (uint8_t j = 0; j < 8; j++) {
        
            // Bước A: Chờ qua pha LOW (Khoảng 50us)
            // Dùng 1 vòng while(đọc chân == RESET) có timeout y chang phân cảnh 2
            time_out = 0;
            while(HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_RESET ){
                delay_us(1);
                time_out++;
                if(time_out > 100) return false;
            }
            // Bước B: Vừa thoát LOW lên HIGH xong -> delay_us(40);
            delay_us(40);
            // Bước C: Dọn chỗ cho bit mới vào mảng
            data[i] = data[i] << 1; 
        
            // Bước D: Mở mắt ra chốt hạ bit 0 hay bit 1
            if (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
                // Nếu 40us rồi mà nó vẫn HIGH -> Đây là xung 70us -> Chốt bit 1
                data[i] = data[i] | 1; 
            
                // CỰC KỲ QUAN TRỌNG: Vì nó là xung dài, m phải dùng thêm 1 vòng while(đọc chân == SET)
                // (kèm timeout) ở đây để chờ nó rớt xuống LOW, nếu không vòng lặp tiếp theo sẽ đọc sai.
                time_out = 0;
                while(HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET ){
                    delay_us(1);
                    time_out++;
                    if(time_out > 100) return false;
                }
            }   
            // (Nếu nó là LOW rồi thì m không cần làm gì, vì lệnh << 1 ở Bước C đã mặc định đuôi nó là 0 rồi).
        }
    }

    if((uint8_t)(data[0] + data[1] + data[2] + data[3] == data[4])){
        *hum = data[0];
        *temp = data[2];
        return true;
    } else return false;

}

extern UART_HandleTypeDef huart1;
extern uint8_t rx_byte;
void BSP_UART_Start_IT(void){
    HAL_UART_Receive_IT(&huart1, &rx_byte, sizeof(rx_byte));
}



