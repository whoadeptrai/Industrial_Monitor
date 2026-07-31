#include "app_main.h"
#include <stdint.h>
#include <string.h>


static char information[128];
static uint8_t information_tracker = 0;
Active App_AO;
void ESP8266_ParseByte(char c){
    if(c == '\n' || c == '\r'){
        information[information_tracker] = '\0'; 
        if (strstr(information, "ALARM_FIRE") != NULL) {
        Event e;
        e.sig = FIRE_DETECTED_SIG;
        Active_post(&App_AO, e); // Ném phiếu vào rổ
        } else if (strstr(information, "SYSTEM_RESET") != NULL) {
            Event e;
            e.sig = RESET_SIG;
            Active_post(&App_AO, e); // Ném phiếu vào rổ
        }
        information_tracker = 0; 
    } 
    else {
        if(information_tracker < 127){
            information[information_tracker] = c;
            information_tracker++;
        }
    }
}