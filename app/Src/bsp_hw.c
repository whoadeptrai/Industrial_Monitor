#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include "bsp_hw.h"
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim2;
// dùng bộ DWT để tạo hàm delay theo micro giây ()
#define DWT_CTRL    (*(volatile uint32_t *)0xE0001000) //DWT_control
#define CYCCNTENA   (1<<0)  //CYCle CouNT ENAble
#define DWT_CYCCNT  (*(volatile uint32_t *)0xE0001004) //DWT_CYCle CouNT Register.
#define DEMCR       (*(volatile uint32_t *)0xE000EDFC) //Debug Exception and Monitor Control Register.
#define TRCENA      (1<<24) //TRaCe ENAble

static bool dwt_initialized = false;

static void DWT_Init(void) {
    if (!dwt_initialized) {
        DEMCR |= TRCENA; //kích hoạt phần cứng Trace (chứa DWT)
        DWT_CTRL |= CYCCNTENA; //kích hoạt bộ đếm
        DWT_CYCCNT = 0;
        dwt_initialized = true; //chỉ init một lần trong while(1)
    }
}
//hàm đọc tín hiệu lửa
bool BSP_GetFireStatus(void){
    //cảm biến HW-484 V0.2 mặc định là 0, có lửa là 1
    if (HAL_GPIO_ReadPin(FIRE_SENSOR_GPIO_Port, FIRE_SENSOR_Pin) == GPIO_PIN_SET)
        return true;
    else
        return false;
}
//hàm đọc tín hiệu gas
bool BSP_GetGasStatus(void){
    
    if (HAL_GPIO_ReadPin(GAS_SENSOR_GPIO_Port, GAS_SENSOR_Pin) == GPIO_PIN_RESET)
        return true; 
    else 
        return false;
}
//hàm thiết lập góc servo
void BSP_SetServoAngle(uint8_t angle){
    if (angle > 180) 
        angle = 180;

    uint16_t ccr_value = 500 + ((angle * 2000) / 180);

    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, ccr_value);//timer4 chu kỳ 50ms
}

static volatile uint16_t joystick_adc[2];
extern ADC_HandleTypeDef hadc1;

void BSP_Init(void){
    //bật chế độ tự động đọc dữ liệu từ ADC đưa vào mảng
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)joystick_adc, 2); 
    //khởi động bộ phát xung PWM trên kênh 1 của Timer 4
    BSP_Servo_Start();
}

//hàm quy đổi toạ độ joystick sang %
void BSP_GetJoystickXY(int8_t* x, int8_t* y){
    int32_t raw_x, raw_y;
    raw_x = joystick_adc[0];
    raw_y = joystick_adc[1];
    int8_t percent_x = (int8_t)(((raw_x - 2048)*100)/2048);
    int8_t percent_y = (int8_t)(((raw_y - 2048)*100)/2048);
    //hạn chế dội
    if (percent_x <= 5 && percent_x >= -5) percent_x = 0;
    if (percent_y <= 5 && percent_y >= -5) percent_y = 0;
    //cập nhật kết quả quy đổi
    if (x != NULL) *x = percent_x;
    if (y != NULL) *y = percent_y;
}
//hàm delay theo micro giây
void Delay_us(uint16_t us){
    DWT_Init();
    uint32_t startTick = DWT_CYCCNT;
    uint32_t delayTicks = us * (SystemCoreClock / 1000000); 
    while ((DWT_CYCCNT - startTick) < delayTicks);
}

//hàm chuyển đổi chế độ input/output cho cảm biến DHT11
static void DHT11_Set_Pin_Dir(uint32_t Mode) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN_Pin; 
    GPIO_InitStruct.Mode = Mode;
    GPIO_InitStruct.Pull = (Mode == GPIO_MODE_INPUT) ? GPIO_PULLUP : GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PIN_GPIO_Port, &GPIO_InitStruct);
}

//hàm đọc tín hiệu từ cảm biến DHT11
uint8_t BSP_ReadDHT11(uint8_t* temp, uint8_t* hum){
    uint8_t data[5] = {0};
    uint32_t timeout;

    //gửi tín hiệu Start: kéo mức thấp ít nhất 18ms sau đó lên mức cao từ 20-40us để chờ phản hồi từ DHT11
    DHT11_Set_Pin_Dir(GPIO_MODE_OUTPUT_PP);
    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_RESET);
    HAL_Delay(18); 
    
    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_SET);
    Delay_us(30);  
    
    //đổi sang chế độ Input
    DHT11_Set_Pin_Dir(GPIO_MODE_INPUT);
    //lúc này DHT11 sẽ gửi tín hiệu phản hồi (mức thấp) và giữ nó trong 80us, nếu là mức cao thì là lỗi tín hiệu
    //đợi cho đến khi tín hiệu kéo xuống sau delay ở trên, nếu vượt quá 100us giây thì là lỗi(thời gian phản hồi tối đa của cảm biến là 80us)
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
        Delay_us(1); timeout++;
        if (timeout > 100) return 0; 
    }

    //đợi cho đến khi tín hiệu kéo lên (cỡ 80us)
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_RESET) {
        Delay_us(1); timeout++;
        if (timeout > 100) return 0;
    }
    //đợi cho đến khi tín hiệu kéo xuống (cỡ 80us)
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
        Delay_us(1); timeout++;
        if (timeout > 100) return 0;
    }
    
    //lúc này mới bắt đầu đọc 40 bits, đọc bit 0 hay 1 thì luôn bắt đầu ở mức thấp 50us trước
    //sau đó nếu thời gian ở mức cao là 26-28us-> mức 0, 70us->mức 1
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 8; j++) {
            //đợi cho đến khi tín hiệu kéo lên (cỡ 50us)
            timeout = 0;
            while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_RESET) {
                Delay_us(1); timeout++;
                if (timeout > 100) return 0;
            }
            
            Delay_us(40); //delay đến khoảng giữa, nếu vẫn còn ở mức 1 thì là tín hiệu 1, ngược lại là 0
            
            if (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) { //bit 1
                data[i] |= (1 << (7 - j)); //truyền từ MSB -> LSB
                timeout = 0;
                while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
                    Delay_us(1); timeout++;
                    if (timeout > 100) return 0;
                }
            } else { // bit 0
                data[i] &= ~(1 << (7 - j));
            }
        }
    }
    
    if (data[4] == (uint8_t)(data[0] + data[1] + data[2] + data[3])) { //kiểm tra xem byte checksum có bằng tổng 4 bit trước đó không
        //do DHT11 chỉ trả về số nguyên nên phần thực của temp và hum (data[1] và data[3]) = 0
        *hum = data[0];
        *temp = data[2];
        return 1;
    }
    return 0;
}

//bật/tắt máy bơm
void BSP_Pump_Start(void){
    HAL_GPIO_WritePin(PUMP_RELAY_GPIO_Port, PUMP_RELAY_Pin, GPIO_PIN_SET);
}
void BSP_Pump_Stop(void){
    HAL_GPIO_WritePin(PUMP_RELAY_GPIO_Port, PUMP_RELAY_Pin, GPIO_PIN_RESET);
}
//bật/tắt còi báo động
void BSP_Buzzer_On(void){
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);}
void BSP_Buzzer_Off(void){
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);}
//bật/tắt servo
void BSP_Servo_Start(void){
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
}
void BSP_Servo_Stop(void){
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
}
//bật/tắt 3 led
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



