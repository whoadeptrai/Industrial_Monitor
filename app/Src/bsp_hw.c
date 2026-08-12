#include "bsp_hw.h"

/* 
 * biến: htim4 (Kiểu: TIM_HandleTypeDef)
 * nằm ở: tim.c 
 * lý do: Cần để điều khiển thanh ghi băm xung PWM cho Servo.
 */
extern TIM_HandleTypeDef htim4;

/* 
 * biến: hadc1 (Kiểu: ADC_HandleTypeDef)
 * nằm ở: adc.c 
 * lý do: Cần để kích hoạt bộ đọc ADC bằng DMA cho Joystick.
 */
extern ADC_HandleTypeDef hadc1;

/* 
 * biến: adc_buffer (Mảng 2 phần tử kiểu uint16_t)
 * nằm ở: bsp_hw.c (Khai báo tĩnh tại đây)
 * lý do: DMA sẽ tự động đẩy giá trị điện áp trục X vào adc_buffer[0] và trục Y vào adc_buffer[1] 
 * mà không cần CPU can thiệp.
 */
static uint16_t adc_buffer[2];


// hàm khởi động ngoại vi (sẽ được gọi 1 lần trong main.c)
void BSP_Init(void)
{
    //kích hoạt Timer 4 Kênh 1 để xuất xung PWM cho Servo
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    
    //kích hoạt ADC chạy nền bằng DMA, lưu dữ liệu thẳng vào mảng adc_buffer
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, 2);
}

//đọc cảm biến Lửa (trả về true nếu phát hiện lửa)
bool BSP_GetFireStatus(void)
{
    // Các Macro FIRE_SENSOR_GPIO_Port và FIRE_SENSOR_Pin được lấy từ main.h
    // Giả sử cảm biến báo mức 1 (High) khi có cháy
    if (HAL_GPIO_ReadPin(FIRE_SENSOR_GPIO_Port, FIRE_SENSOR_Pin) == GPIO_PIN_SET) {
        return true;
    }
    return false;
}

// đọc cảm biến Gas (trả về true nếu có khí Gas)
bool BSP_GetGasStatus(void)
{
    if (HAL_GPIO_ReadPin(GAS_SENSOR_GPIO_Port, GAS_SENSOR_Pin) == GPIO_PIN_SET) {
        return true;
    }
    return false;
}

// Điều khiển Bơm nước
void BSP_SetPump(bool state)
{
    if (state) {
        HAL_GPIO_WritePin(PUMP_RELAY_GPIO_Port, PUMP_RELAY_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(PUMP_RELAY_GPIO_Port, PUMP_RELAY_Pin, GPIO_PIN_RESET);
    }
}

// Đọc tọa độ Joystick
void BSP_GetJoystickXY(uint16_t* x, uint16_t* y)
{
    // Lấy dữ liệu trực tiếp từ RAM (do DMA tự điền), cực kỳ nhanh và không tốn CPU
    *x = adc_buffer[0];
    *y = adc_buffer[1];
}

// Điều khiển góc xoay Servo SG90 (0 - 180 độ)
void BSP_SetServoAngle(uint8_t angle)
{
    /*
     * BÀI TOÁN XUẤT XUNG PWM CHO SERVO SG90:
     * - Trong tim.c, Timer 4 được cấu hình Period = 19999, Prescaler = 71 -> Chu kỳ xung = 20ms (50Hz).
     * - Servo SG90 hoạt động với độ rộng xung từ 0.5ms (0 độ) đến 2.5ms (180 độ).
     * - Tương ứng với thanh ghi CCR: 0.5ms = 500 tick, 2.5ms = 2500 tick.
     * - Công thức quy đổi: CCR = 500 + (angle * (2500 - 500) / 180).
     */
    
    // giới hạn góc quay tránh kẹt động cơ
    if (angle > 180) angle = 180;
    
    // tính toán giá trị thanh ghi Compare
    uint32_t ccr_val = 500 + (angle * 2000 / 180);
    
    // ghi trực tiếp vào thanh ghi CCR1 của Timer 4
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, ccr_val);
}

// hàm giao tiếp cảm biến nhiệt độ DHT11 (Chưa viết ruột)
bool BSP_ReadDHT11(uint8_t* temp, uint8_t* hum)
{
    // TODO: Triển khai giao thức 1-wire Bit-banging
    // Khá phức tạp vì yêu cầu trễ mức micro-giây (microsecond delay)
    return false; 
}