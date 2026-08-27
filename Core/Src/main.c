/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_main.h"
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern uint8_t rx_byte;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  // Khai báo biến lưu trạng thái cũ để bắt sườn (Fix Bug 2)
  bool last_fire = false;
  bool last_gas = false;
  bool last_sw_mode = true; //nút chưa bấm thì chân ở mức 1 (PULLUP)
  bool last_btn_reset = true;
  uint32_t last_poll_time = 0;
  uint32_t last_dht11_time = 0;

  BSP_Init();
  App_AO_Init();
  Active_init(&App_AO);

  HAL_UART_Receive_IT(&huart1, &rx_byte, 1); //gọi trước lần đầu tiên bên ngoài ngắt

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1){ 
    //50ms thì đọc các cảm biến lửa, gas và joystick 1 lần
    if (HAL_GetTick() - last_poll_time >= 50) {
      last_poll_time = HAL_GetTick(); //cập nhật lại mốc thời gian

      //quét cảm biến lửa
      bool current_fire = BSP_GetFireStatus();
      if (current_fire == true && last_fire == false) { 
          Event e; 
          e.sig = FIRE_DETECTED_SIG;
          Active_post(&App_AO, e); //chỉ chỉ gửi event cháy đúng 1 lần lúc lửa mới bùng lên
      }
      last_fire = current_fire; //lưu lại để vòng sau so sánh

      //quét cảm biến gas
      bool current_gas = BSP_GetGasStatus();
      if (current_gas == true && last_gas == false) {
          Event e; 
          e.sig = GAS_DETECTED_SIG;
          Active_post(&App_AO, e); //chỉ gửi event gas đúng 1 lần lúc phát hiện rò gas
      }
      last_gas = current_gas;

      static int8_t last_joy_x = 0; 
      int8_t current_joy_x, current_joy_y;
      BSP_GetJoystickXY(&current_joy_x, &current_joy_y);// khai báo joy_y nhưng không sử dụng vì chỉ dùng có 1 servo->dùng trục x để map
      if (current_joy_x != last_joy_x) { //chỉ đưa event vào buffer khi thấy x thay đổi
        //x:      -100  =>  100
        //servo:  0'    =>  180'
        //=> độ = (x+100)*180/200
          Event e; 
          e.sig = JOYSTICK_MOVED_SIG;
          e.param = (uint32_t)((current_joy_x + 100) * 180 / 200); 
          Active_post(&App_AO, e);
          
          last_joy_x = current_joy_x;//cập nhật last_joy_x
      }

      // quét button SW_MODE
      //đọc chân PB1 (Active-Low: bấm xuống thì chân bị nối GND -> Mức 0)
        bool current_sw_mode = (HAL_GPIO_ReadPin(GPIOB, SW_MODE_Pin) == GPIO_PIN_SET); 
        
        //nếu trạng thái cũ là 1 (Chưa bấm) và hiện tại là 0 (Đang bấm)
        if (current_sw_mode == false && last_sw_mode == true) { 
            Event e; e.sig = MODE_SWITCH_SIG;
            Active_post(&App_AO, e); // gửi thư yêu cầu chuyển chế độ
        }
        last_sw_mode = current_sw_mode; // Lưu lại cho vòng sau

        // quét BTN_RESET
        bool current_btn_reset = (HAL_GPIO_ReadPin(GPIOB, BTN_RESET_Pin) == GPIO_PIN_SET);
        if (current_btn_reset == false && last_btn_reset == true) { 
            Event e; 
            e.sig = RESET_SIG;
            Active_post(&App_AO, e); 
        }
        last_btn_reset = current_btn_reset;
    }
    //đọc cảm biến dht11 mỗi 2 giây
    if (HAL_GetTick() - last_dht11_time >= 2000) {
          last_dht11_time = HAL_GetTick();
          
          uint8_t temp = 0, hum = 0;
          if (BSP_ReadDHT11(&temp, &hum) == 1) { //nếu đọc thành công
              if (temp > 45) { //khi nhiệt độ quá cao
                  Event e;
                  e.sig = TEMP_HIGH_SIG; //gửi tín hiệu warning sắp có cháy
                  e.param = (temp << 8) | hum;//parm 32 bit có thể chứa cả temp và hum (mỗi cái 8 bit)
                  Active_post(&App_AO, e);
              }
          }
    }
    /* USER CODE BEGIN 3 */
    //liên tục lấy sự kiện từ buffer ra để xử lý
    Active_dispatch(&App_AO);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
