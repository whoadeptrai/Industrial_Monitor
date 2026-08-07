#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include "bsp_hw.h"
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim2;

#define DWT_CTRL    (*(volatile uint32_t *)0xE0001000)
#define CYCCNTENA   (1<<0)
#define DWT_CYCCNT  (*(volatile uint32_t *)0xE0001004)
#define DEMCR       (*(volatile uint32_t *)0xE000EDFC)
#define TRCENA      (1<<24)

static bool dwt_initialized = false;

static void DWT_Init(void) {
    if (!dwt_initialized) {
        DEMCR |= TRCENA;
        DWT_CTRL |= CYCCNTENA;
        DWT_CYCCNT = 0;
        dwt_initialized = true;
    }
}

bool BSP_GetFireStatus(void){
    if (HAL_GPIO_ReadPin(FIRE_SENSOR_GPIO_Port, FIRE_SENSOR_Pin) == GPIO_PIN_RESET){
        return true;
    } else return false;
}

bool BSP_GetGasStatus(void){
    if (HAL_GPIO_ReadPin(GAS_SENSOR_GPIO_Port, GAS_SENSOR_Pin) == GPIO_PIN_RESET){
        return true;
    } else return false;
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

void Delay_us(uint16_t us){
    DWT_Init();
    uint32_t startTick = DWT_CYCCNT;
    uint32_t delayTicks = us * (SystemCoreClock / 1000000); 
    while ((DWT_CYCCNT - startTick) < delayTicks);
}


static void DHT11_Set_Pin_Dir(uint32_t Mode) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN_Pin; 
    GPIO_InitStruct.Mode = Mode;
    GPIO_InitStruct.Pull = (Mode == GPIO_MODE_INPUT) ? GPIO_PULLUP : GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PIN_GPIO_Port, &GPIO_InitStruct);
}

uint8_t BSP_ReadDHT11(uint8_t* temp, uint8_t* hum){
    uint8_t data[5] = {0};
    uint32_t timeout;

    // 1. Gửi tín hiệu Start
    DHT11_Set_Pin_Dir(GPIO_MODE_OUTPUT_PP);
    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_RESET);
    HAL_Delay(18); 
    
    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_SET);
    Delay_us(30);  
    
    // Đổi sang chế độ Input
    DHT11_Set_Pin_Dir(GPIO_MODE_INPUT);
    
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
        Delay_us(1); timeout++;
        if (timeout > 100) return 0; 
    }
    
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_RESET) {
        Delay_us(1); timeout++;
        if (timeout > 100) return 0;
    }
    
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
        Delay_us(1); timeout++;
        if (timeout > 100) return 0;
    }
    
    // 3. Đọc 40 bits
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 8; j++) {
            timeout = 0;
            while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_RESET) {
                Delay_us(1); timeout++;
                if (timeout > 100) return 0;
            }
            
            Delay_us(40); 
            
            if (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
                data[i] |= (1 << (7 - j)); 
                timeout = 0;
                while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
                    Delay_us(1); timeout++;
                    if (timeout > 100) return 0;
                }
            } else {
                data[i] &= ~(1 << (7 - j));
            }
        }
    }
    
    if (data[4] == (uint8_t)(data[0] + data[1] + data[2] + data[3])) {
        *hum = data[0];
        *temp = data[2];
        return 1;
    }
    
    return 0;

}

void BSP_Pump_Start(void){
    HAL_GPIO_WritePin(PUMP_RELAYB15_GPIO_Port, PUMP_RELAYB15_Pin, GPIO_PIN_SET);
}
void BSP_Pump_Stop(void){
    HAL_GPIO_WritePin(PUMP_RELAYB15_GPIO_Port, PUMP_RELAYB15_Pin, GPIO_PIN_RESET);
}
void BSP_Buzzer_On(void){
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
}
void BSP_Buzzer_Off(void){
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);
}
void BSP_LED_Control(uint8_t color, uint8_t state){
    GPIO_PinState hal_state = (state == LED_ON) ? GPIO_PIN_SET : GPIO_PIN_RESET;

    switch(color){
        case LED_COLOR_RED:
            HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, hal_state);
            break;
        case LED_COLOR_GREEN:
            HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, hal_state);
            break;
        case LED_COLOR_YELLOW:
            HAL_GPIO_WritePin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin, hal_state);
            break;
        default:
            break; 
    }
}



