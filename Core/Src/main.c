/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include <stdio.h>

// آخرین صفحه Flash (بسته به MCU تغییر می‌کنه، این برای STM32F103C8 نمونه است)
#define FLASH_USER_START_ADDR 0x0801FC00
#define FLASH_USER_END_ADDR   0x0801FFFF


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
	STATE_MAIN_MENU,
	STATE_OPTION1_MENU,
	STATE_OPTION2_MENU,
	STATE_HANDTUNE1,
	STATE_HANDTUNE2,
	STATE_UART_TUNE,
	STATE_TEMP_MONITOR
} menuState_t;

menuState_t currentstate = STATE_MAIN_MENU;
uint8_t menuindex = 0;

const char *mainmenu[] = {"Desired", "offset", "Tune Temp"};
const char *option1menu[] = {"Hand Tune", "UART"};
const char *option2menu[] = {"Hand Tune", "UART"};

int temp;
int temp_edit;

int offset;
int offset_edit;

int tim1 = 0;
int tim2 = 0;
int blink_done = 0;
volatile uint8_t uart_flag = 0;
volatile uint8_t uart_rx_byte;
uint32_t uart_last_rx_time = 0;
uint32_t monitor_enter_time = 0;


uint16_t temp_uart = 0;   // دمای دریافتی از UART







uint8_t uart_state = 0;     // 0=wait start, 1=H, 2=L, 3=end
uint8_t temp_h, temp_l;
uint16_t temp_rx = 0;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim8;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
//uint8_t uart_rx_byte;         // بایت دریافتی UART
//uint8_t uart_state = 0;       // وضعیت دریافت
uint16_t uart_timeout = 0;    // شمارنده timeout
#define UART_START 0xEE

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM8_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


#include "math.h"

// I2C به صورت دستی
I2C_HandleTypeDef hi2c1;

void I2C1_Init(void)
{



    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

/* MPU6050 */
#define MPU_ADDR 0x68<<1
#define MPU_PWR_MGMT_1 0x6B
#define MPU_TEMP_OUT_H 0x41

void MPU6050_Init(void)
{
    uint8_t data = 0x00; // wake up
    HAL_I2C_Mem_Write(&hi2c1, MPU_ADDR, MPU_PWR_MGMT_1, 1, &data, 1, 100);
}

float MPU6050_ReadTemp(void)
{
    uint8_t buf[2];
    int16_t raw;

    HAL_I2C_Mem_Read(&hi2c1, MPU_ADDR, MPU_TEMP_OUT_H, 1, buf, 2, 100);
    raw = (int16_t)(buf[0]<<8 | buf[1]);
    return (raw / 340.0f) + 36.53f;
}












void LCD_showmainmenu(uint8_t index) {

	lcd_clear();
	lcd_gotoxy(0, 0);
	lcd_puts(">");
	lcd_puts(mainmenu[index]);
	lcd_gotoxy(0, 1);
	lcd_puts(" ");
	lcd_puts(mainmenu[(index+1) % 3]);

}
void LCD_showoption1menu(uint8_t index) {

	lcd_clear();
	lcd_gotoxy(0, 0);
	lcd_puts(">");
	lcd_puts(option1menu[index]);
	lcd_gotoxy(0, 1);
	lcd_puts(" ");
	lcd_puts(option1menu[(index+1) % 2]);


}
void LCD_showoption2menu(uint8_t index) {

	lcd_clear();
	lcd_gotoxy(0, 0);
	lcd_puts(">");
	lcd_puts(option2menu[index]);
	lcd_gotoxy(0, 1);
	lcd_puts(" ");
	lcd_puts(option2menu[(index+1) % 2]);

}

void menu_next(void) {

	if(currentstate == STATE_MAIN_MENU) {
		menuindex = (menuindex+1) % 3;
		 LCD_showmainmenu(menuindex);
	}

	else if(currentstate == STATE_OPTION1_MENU) {
		menuindex = (menuindex+1) % 2;
		LCD_showoption1menu(menuindex);

	}

	else if(currentstate == STATE_OPTION2_MENU) {
		menuindex = (menuindex+1) % 2;
		LCD_showoption2menu(menuindex);

	}
}


void menu_select(void) {

	if(currentstate == STATE_MAIN_MENU) {

		if(menuindex == 0) {

			currentstate = STATE_OPTION1_MENU;
			menuindex = 0;
			LCD_showoption1menu(menuindex);

			__HAL_TIM_SET_COUNTER(&htim2, 0);
			tim2 = 0;
}



		else if(menuindex == 1) {

			currentstate = STATE_OPTION2_MENU;
			menuindex = 0;
			LCD_showoption2menu(menuindex);

			__HAL_TIM_SET_COUNTER(&htim2, 0);
			tim2 = 0;

		}

		else if(menuindex == 2)
		{
		    currentstate = STATE_TEMP_MONITOR;
		    lcd_clear();
		    monitor_enter_time = HAL_GetTick();
		}



	}
	else if(currentstate == STATE_OPTION1_MENU) {

		if(menuindex == 0) {

			currentstate = STATE_HANDTUNE1;
			lcd_clear();

			temp_edit = temp;

			char buffer[16];
			sprintf(buffer, "%d", temp_edit);
			lcd_puts(buffer);


		}

		else if(menuindex == 1) // UART Mode
		{
			currentstate = STATE_UART_TUNE;
			lcd_clear();
			lcd_puts("UART Mode");
			uart_last_rx_time = HAL_GetTick();

		}



		//else if(menuindex == 1) {

			//lcd_clear();

		//}
	}
		else if(currentstate == STATE_OPTION2_MENU) {

			if(menuindex == 0) {

				currentstate = STATE_HANDTUNE2;
				lcd_clear();

				offset_edit = offset;
				char buffer2[16];
				sprintf(buffer2, "%d", offset_edit);
				lcd_puts(buffer2);


			}




			else if(menuindex == 1) // UART Mode
			{
				currentstate = STATE_UART_TUNE;
				lcd_clear();
				lcd_puts("UART Mode");
				uart_last_rx_time = HAL_GetTick();
			}

			//else if(menuindex == 1) {

			//	lcd_clear();

			//}
	}
}

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
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM8_Init();


  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  Load_Settings();  // بارگذاری تنظیمات ذخیره شده

  lcd_init();
  I2C1_Init();
  MPU6050_Init();
  HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);
  HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1); // شروع دریافت بایت غیر بلاک

  lcd_clear();
  GPIO_PinState next_button;
  GPIO_PinState select_button;
  GPIO_PinState confirm_switch;

  int flag1 = 0;
  int flag2 = 0;
  int flag3 = 0;
  //temp = 25;
  //offset = 2;

  LCD_showmainmenu(menuindex);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  uint32_t temp_tick = 0;



  while (1)
  {

	  next_button=HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11);
	  select_button=HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);
	  confirm_switch=HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13);

	  if(currentstate != STATE_HANDTUNE1 &&
	     currentstate != STATE_HANDTUNE2 &&
	     currentstate != STATE_UART_TUNE) {
		  if((next_button == 1) && (flag1 == 0)) {

			  menu_next();
			  flag1 = 1;

			  if(currentstate != STATE_MAIN_MENU){
				  __HAL_TIM_SET_COUNTER(&htim2, 0);
				  tim2 = 0;		  }
		  }

		  if((next_button == 0) && (flag1 == 1)) {

			  flag1 = 0;
		  }

		  if((select_button == 1) && (flag2 == 0)) {

			  menu_select();
			  flag2 = 1;

			  if(currentstate != STATE_MAIN_MENU){
				  __HAL_TIM_SET_COUNTER(&htim2, 0);
				  tim2 = 0;
			  }
		  }

		  if((select_button == 0) && (flag2 == 1)) {

			  flag2 = 0;
		  }

	  }

	  if(currentstate == STATE_HANDTUNE1)
	  {
	      if((next_button == 1) && (flag1 == 0))
	      {
	          temp_edit++;
	          flag1 = 1;

	          lcd_clear();
	          char buf[16];
	          sprintf(buf, "%d", temp_edit);
	          lcd_puts(buf);

	          __HAL_TIM_SET_COUNTER(&htim2, 0);
	          tim2 = 0;
	      }
	      if((next_button == 0) && (flag1 == 1)) {
	          flag1 = 0;
	      }

	      if((select_button == 1) && (flag2 == 0))
	      {
	          temp_edit--;
	          flag2 = 1;

	          lcd_clear();
	          char buf[16];
	          sprintf(buf, "%d", temp_edit);
	          lcd_puts(buf);

	          __HAL_TIM_SET_COUNTER(&htim2, 0);
	          tim2 = 0;
	      }

	      if((select_button == 0) && (flag2 == 1)) {
	          flag2 = 0;
	      }
	      if(confirm_switch == 1 && flag3 == 0) {

	    	  temp = temp_edit;
	    	  Save_Settings();  // ذخیره تنظیمات در Flash


	          __HAL_TIM_SET_COUNTER(&htim1, 0);
	          tim1 = 0;
	          HAL_TIM_Base_Start_IT(&htim1);

	          flag3 = 1;

	      }
	      if (confirm_switch == 0 && flag3 == 1) {
	    	  flag3 = 0;
	      }
	  }

	  if(currentstate == STATE_HANDTUNE2)
	  {
	      if((next_button == 1) && (flag1 == 0))
	      {
	          offset_edit++;
	          flag1 = 1;

	          lcd_clear();
	          char buf2[16];
	          sprintf(buf2, "%d", offset_edit);
	          lcd_puts(buf2);

	          __HAL_TIM_SET_COUNTER(&htim2, 0);
	          tim2 = 0;

	          if(offset_edit < 0)
	          {
	              HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
	              __HAL_TIM_SET_COUNTER(&htim3, 0);
	          }

	      }
	      if((next_button == 0) && (flag1 == 1)) {
	          flag1 = 0;
	      }

	      if((select_button == 1) && (flag2 == 0))
	      {
	    	  flag2 = 1;

	    	  if(offset_edit > 0)
	    	  {
	    	      offset_edit--;
	    	  }
	    	  else
	    	  {
	    	      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
	    	      __HAL_TIM_SET_COUNTER(&htim3, 0);
	    	      HAL_TIM_Base_Start_IT(&htim3);
	    	  }

	    	  lcd_clear();
	    	  char buf2[16];
	    	  sprintf(buf2, "%d", offset_edit);
	    	  lcd_puts(buf2);

	    	  __HAL_TIM_SET_COUNTER(&htim2, 0);
	    	  tim2 = 0;

	      }

	      if((select_button == 0) && (flag2 == 1)) {

	          flag2 = 0;
	      }

	      if(confirm_switch == 1 && flag3 == 0) {
	    	  offset = offset_edit;
	    	  Save_Settings();  // ذخیره تنظیمات در Flash


	          __HAL_TIM_SET_COUNTER(&htim1, 0);
	          tim1 = 0;
	          HAL_TIM_Base_Start_IT(&htim1);

	          flag3 = 1;

	      }

	      if (confirm_switch == 0 && flag3 == 1) {
	    	  flag3 = 0;
	      }
	  }



	  /* ===== UART TUNE MODE ===== */
	  if(currentstate == STATE_UART_TUNE)
	  {
	      if(uart_flag)
	      {
	          uart_flag = 0;

	          uart_last_rx_time = HAL_GetTick();

	          switch(uart_state)
	          {
	              // ===== منتظر START =====

	          case 0:
	              if(uart_rx_byte == 0xEE)
	                  {
	                      uart_state = 1;

	                      // ===== READY MESSAGE =====
	                      lcd_clear();
	                      lcd_puts("ready");

	                      char msg[] = "ready\r\n";
	                      HAL_UART_Transmit(&huart1, (uint8_t*)msg, sizeof(msg)-1, 100);
	                  }

	                  break;

	              // ===== دریافت HIGH =====
	              case 1:
	                  temp_h = uart_rx_byte;
	                  uart_state = 2;
	                  break;

	              // ===== دریافت LOW =====
	              case 2:
	                  temp_l = uart_rx_byte;
	                  uart_state = 3;
	                  break;

	              // ===== منتظر END =====
	              case 3:
	                  if(uart_rx_byte == 0xAA)
	                  {
	                      temp_rx = (temp_h << 8) | temp_l;
	                      temp_uart = temp_rx;
	                      char buf[16];
	                      sprintf(buf, "Temp:%d", temp_rx);

	                      lcd_clear();
	                      lcd_puts(buf);

	                      // ===== ACK =====
	                      char ok[] = "OK\r\n";
	                      HAL_UART_Transmit(&huart1, (uint8_t*)ok, sizeof(ok)-1, 100);
	                  }
	                  else
	                  {
	                      lcd_clear();
	                      lcd_puts("Wrong packet");

	                      char err[] = "ERR\r\n";
	                      HAL_UART_Transmit(&huart1, (uint8_t*)err, sizeof(err)-1, 100);

	                      HAL_Delay(1000);
	                  }

	                  uart_state = 0;
	                  break;

	          }
	      }
	      update_pwm_from_temp();  // PWM LED بروزرسانی شود

	      // ===== timeout =====
	      if(HAL_GetTick() - uart_last_rx_time > 10000)
	      {
	          uart_state = 0;

	          currentstate = STATE_MAIN_MENU;
	          menuindex = 0;
	          LCD_showmainmenu(menuindex);
	      }
	  }



	  if(blink_done == 1) {

		  blink_done = 0;

		  currentstate = STATE_MAIN_MENU;
		  menuindex = 0;
		  LCD_showmainmenu(menuindex);


	  }

	  if(tim2 == 1 && currentstate != STATE_MAIN_MENU)
	  {
	      tim2 = 0;
	      currentstate = STATE_MAIN_MENU;
	      menuindex = 0;
	      LCD_showmainmenu(menuindex);
	  }

	  /* ===== TEMP MONITOR ===== */
	  if(currentstate == STATE_TEMP_MONITOR)
	  {
		  float t = MPU6050_ReadTemp();

		      char buf[16];
		      sprintf(buf, "Temp: %.1f C", t);

		      if(HAL_GetTick() - temp_tick > 500)
		      {
		          temp_tick = HAL_GetTick();

		          lcd_gotoxy(0,0);

		          lcd_puts(buf);
		      }

		      // ===== خروج خودکار بعد 5 ثانیه =====
		      if(HAL_GetTick() - monitor_enter_time > 5000)
		      {
		          currentstate = STATE_MAIN_MENU;
		          menuindex = 0;
		          LCD_showmainmenu(menuindex);
		      }

	  }





    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM1)
    {

    	tim1++;
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8);
        if(tim1 >= 6)
        {
        	HAL_TIM_Base_Stop_IT(&htim1);
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
            blink_done = 1;
         }

    }

    if(htim->Instance == TIM2)
    {
        tim2++;
    }

    if(htim->Instance == TIM3) {

    	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
    }
}









/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 7999;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 499;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 7999;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 499;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 999;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 7;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim8, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim8, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */
  HAL_TIM_MspPostInit(&htim8);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_I2C1_CLK_ENABLE();


  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_8
                          |GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pins : PC0 PC1 PC2 PC3
                           PC4 PC5 PC6 PC8
                           PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_8
                          |GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PB11 PB12 PB13 */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* ===== I2C1 GPIO ===== */


  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;   // PB6=SCL , PB7=SDA
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;          // open-drain
  GPIO_InitStruct.Pull = GPIO_PULLUP;              // یا خارجی 4.7k
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        uart_flag = 1;
        HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
    }
}


void update_pwm_from_temp(void)
{
    int16_t error = temp_uart - temp;

    // فقط مساوی → خاموش
    if(error == 0)
    {
        __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, 0);
        return;
    }

    // قدر مطلق اختلاف
    if(error < 0)
        error = -error;

    // ضریب تقویت (تنظیم شدت)
    int k = 4;

    int16_t value = error * k;

    // محدودسازی 0..100 درصد
    if(value > 100)
        value = 100;

    uint32_t pulse = (htim8.Init.Period + 1) * value / 100;

    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, pulse);
}


void Save_Settings(void)
{
    HAL_FLASH_Unlock(); // باز کردن Flash برای نوشتن

    // پاک کردن صفحه Flash
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FLASH_USER_START_ADDR;
    EraseInitStruct.NbPages = 1;

    HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

    // نوشتن temp و offset
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_USER_START_ADDR, temp);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_USER_START_ADDR + 2, offset);

    HAL_FLASH_Lock(); // قفل کردن Flash بعد از نوشتن
}

void Load_Settings(void)
{
    uint16_t temp_flash = *(uint16_t*)FLASH_USER_START_ADDR;
    uint16_t offset_flash = *(uint16_t*)(FLASH_USER_START_ADDR + 2);

    if(temp_flash == 0xFFFF) temp = 25;
    else temp = temp_flash;

    if(offset_flash == 0xFFFF) offset = 2;
    else offset = offset_flash;
}





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
