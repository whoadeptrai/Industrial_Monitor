
#include "app_main.h"
#include "micro_ao.h"
#include <stdbool.h>
#include <string.h> //dùng cho hàm strstr
#include <stdint.h>
#include <stdio.h> //dùng cho sprintf
/* ====================================================================
 * KHỞI TẠO STATE MACHINE
 * ==================================================================== */
// đây là nơi cấp phát bộ nhớ thật sự cho App_AO 
// File stm32f1xx_it.c sẽ gọi sang đây để xài ké biến này.

Active App_AO; 

/* ====================================================================
 * HÀM PARSER BÓC TÁCH LỆNH VÀ RA QUYẾT ĐỊNH
 * ==================================================================== */
//hàm này được gọi khi đã nhận đủ 1 câu lệnh từ ESP8266 (kết thúc bằng \n)
void ESP8266_ParseCommand(const char* cmd_str)
{
    // tìm chữ "ALARM_FIRE" trong chuỗi nhận được
    if (strstr(cmd_str, "ALARM_FIRE") != NULL) 
    {
        Event evt;
        evt.sig = FIRE_DETECTED_SIG; 
        evt.param = 0;               
        Active_post(&App_AO, evt);   // báo cho hệ thống biết có cháy!
    }
    // tìm chữ "ALARM_GAS" trong chuỗi nhận được
    else if (strstr(cmd_str, "ALARM_GAS") != NULL) 
    {
        Event evt;
        evt.sig = GAS_DETECTED_SIG;  
        evt.param = 0;
        Active_post(&App_AO, evt);   // báo cho hệ thống biết xì gas!
    }
    else if (strstr(cmd_str, "RESET_ALARM") != NULL) {
        Event evt = { .sig = RESET_SIG, .param = 0 };
        Active_post(&App_AO, evt); // tắt còi, tắt bơm từ xa
    }
    else if (strstr(cmd_str, "SWITCH_MODE") != NULL) {
        Event evt = { .sig = MODE_SWITCH_SIG, .param = 0 };
        Active_post(&App_AO, evt); // chuyển đổi auto / manual từ xa
    }
}
static char tx_buffer[128]; //bộ đệm riêng để chứa chuỗi gửi đi
//hàm gửi thông tin lên esp8266
void App_Send_Alert(const char* message) {
    extern UART_HandleTypeDef huart1; 
    //copy message vào bộ đệm tĩnh để giữ dữ liệu sống sót khi thoát hàm
    strncpy(tx_buffer, message, sizeof(tx_buffer) - 1);
    tx_buffer[sizeof(tx_buffer) - 1] = '\0'; 
    //yêu cầu phần cứng tự động gửi đi mà không bắt CPU phải đứng chờ
    HAL_UART_Transmit_IT(&huart1, (uint8_t*)tx_buffer, strlen(tx_buffer));
}

static void State_IDLE(Active * const me, const Event * const e);
static void State_Fire_Alarm(Active * const me, const Event * const e);
static void State_Smoke_Alarm(Active * const me, const Event * const e);
static void State_Manual(Active * const me, const Event * const e);

/* ====================================================================
 * LOGIC STATE MACHINE
 * ==================================================================== */
static void State_IDLE(Active * const me, const Event * const e) {
    switch (e->sig) {
        case ENTRY_SIG:
            BSP_LED_Control(LED_COLOR_GREEN, LED_ON); //state idle thì led green sáng
            App_Send_Alert("STATE: IDLE\r\n");  
            return;

        case TEMP_HIGH_SIG:  //warning trước khi có cháy thật
            uint8_t temp = (e->param >> 8) & 0xFF; 
            uint8_t hum = e->param & 0xFF;

            //dùng sprintf để nhét cả 2 số vào chuỗi, %% để in ra ký tự %
            char alert_msg[64];
            sprintf(alert_msg, "WARNING: OVERHEAT! TEMP = %u C, HUM = %u %%\r\n", temp, hum);
            App_Send_Alert(alert_msg); //gửi lên ESP8266
            
            //chuyển sang trạng thái báo khói (chỉ hú còi, không xịt nước)
            Active_tran(me, State_Smoke_Alarm); 
            return;
        
        case FIRE_DETECTED_SIG: 
            Active_tran(me, State_Fire_Alarm); //có lửa -> vào trạng thái Cháy
            return; 

        case GAS_DETECTED_SIG: 
            Active_tran(me, State_Smoke_Alarm); //có khói -> vào trạng thái Khói
            return;

        case MODE_SWITCH_SIG:
            Active_tran(me, State_Manual); 
            return;
            
        case EXIT_SIG:
            BSP_LED_Control(LED_COLOR_GREEN, LED_OFF); 
            return;
            
        default:
            return;
    }
}

static void State_Manual(Active * const me, const Event * const e) {
    //state manual thì led yellow sáng
    switch (e->sig) {
        case ENTRY_SIG:
            BSP_LED_Control(LED_COLOR_YELLOW, LED_ON); 
            return;
            
        case JOYSTICK_MOVED_SIG:
            BSP_Servo_Start();
            BSP_SetServoAngle(e->param); 
            return;
            
        case MODE_SWITCH_SIG:
            Active_tran(me, State_IDLE); 
            return;

        case TEMP_HIGH_SIG:  //warning trước khi có cháy thật
            uint8_t temp = (e->param >> 8) & 0xFF; 
            uint8_t hum = e->param & 0xFF;

            //dùng sprintf để nhét cả 2 số vào chuỗi, %% để in ra ký tự %
            char alert_msg[64];
            sprintf(alert_msg, "WARNING: OVERHEAT! TEMP = %u C, HUM = %u %%\r\n", temp, hum);
            
            //chuyển sang trạng thái báo khói (chỉ hú còi, không xịt nước)
            Active_tran(me, State_Smoke_Alarm); 
            return;

        case FIRE_DETECTED_SIG: 
            Active_tran(me, State_Fire_Alarm);
            return;
            
        case GAS_DETECTED_SIG: 
            Active_tran(me, State_Smoke_Alarm);
            return;
            
        case EXIT_SIG:
            BSP_LED_Control(LED_COLOR_YELLOW, LED_OFF); 
            BSP_Servo_Stop();
            return;
            
        default:
            return;
    }
}

static void State_Fire_Alarm(Active * const me, const Event * const e) {
    switch (e->sig) {
        case ENTRY_SIG:  
            BSP_LED_Control(LED_COLOR_RED, LED_ON);    
            BSP_Pump_Start();       //kích hoạt máy bơm nước dập lửa
            BSP_Buzzer_On();    //còi
            BSP_Servo_Start();
            BSP_SetServoAngle(0);   //khóa cửa ngăn cháy lan
            App_Send_Alert("WARNING: FIRE_DETECTED\r\n");  
            return;

        case GAS_DETECTED_SIG: //nếu lửa hết rồi mà còn khói thì vẫn phải báo khói
            Active_tran(me, State_Smoke_Alarm);
            return;

        case RESET_SIG: 
            if (BSP_GetFireStatus() == true) { //vẫn còn lửa thì không cho reset
                return; 
            } 
            if (BSP_GetGasStatus() == true) { //nếu tắt lửa rồi mà vẫn còn khói
                Active_tran(me, State_Smoke_Alarm);
                return;
            }
            Active_tran(me, State_IDLE); 
            return;
            
        case EXIT_SIG:   
            BSP_LED_Control(LED_COLOR_RED, LED_OFF);   
            BSP_Pump_Stop();     //tắt máy bơm khi reset
            BSP_Buzzer_Off();   //tắt còi
            BSP_Servo_Stop();   //tắt servo
            return;

        default:
            return;
    }
}

static void State_Smoke_Alarm(Active * const me, const Event * const e) {
    switch (e->sig) {
        case ENTRY_SIG:
            BSP_LED_Control(LED_COLOR_RED, LED_ON);    
            BSP_Buzzer_On();   //chỉ kích hoạt relay khói (còi hú), ko gọi lệnh bật bơm
            App_Send_Alert("WARNING: SMOKE_DETECTED\r\n");  
            return;

        case FIRE_DETECTED_SIG: //đang có khói mà có lửa thì phải ưu tiên lửa 
            Active_tran(me, State_Fire_Alarm); // Đang khói mà thấy lửa là chuyển thẳng sang bật bơm dập lửa!
            return;

        case RESET_SIG: 
            if (BSP_GetGasStatus() == true) { //vẫn còn khói thì không cho reset
                return; 
            } 
            Active_tran(me, State_IDLE); 
            return;
            
        case EXIT_SIG:  
            BSP_LED_Control(LED_COLOR_RED, LED_OFF);   
            BSP_Buzzer_Off();   //tắt còi
            return;
            
        default:
            return;
    }
}
//mặc định khi khởi tạo sẽ vào state idle
void App_AO_Init(void){
    Active_ctor(&App_AO, State_IDLE);
}