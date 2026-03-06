/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics. 
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the 
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
TIM_HandleTypeDef    TimHandleHF;
TIM_HandleTypeDef    TimHandleLF;
TIM_HandleTypeDef    TimHandleLED;
TIM_HandleTypeDef    TimHandleDEC;

__IO FlagStatus DownStatus = RESET;
__IO FlagStatus UpStatus = RESET;
__IO FlagStatus SelStatus = RESET;
__IO FlagStatus LeftStatus = RESET;
__IO FlagStatus RightStatus = RESET;
__IO FlagStatus KeyStatus = RESET;

static __IO uint32_t TimingDelay = 0;
static __IO uint32_t LedShowStatus = 0;
static __IO uint8_t Index = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* USER CODE BEGIN PFP */
static void SystemClock_Config(void);
static void Demo_LED_Init(void);
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
  /* USER CODE BEGIN 2 */
  /* Configure Joystick pins as input floating with EXTI */
  BSP_PB_Init(BUTTON_BACK, BUTTON_MODE_GPIO);
  BSP_JOY_Init(JOY_MODE_EXTI);

  /* Enable Leds toggling */
  Demo_LED_Init();
  Demo_LedShow(ENABLE);

  /* Set the LCD Back Color */
  BSP_LCD_SetBackColor(LCD_COLOR_BLUE);

  /* Set the LCD Text Color */
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

  /* Initialize the Menu */
  Menu_Init();
  Menu_DisplayMenu();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  /* The DOWNStatus, SELStatus or UPStatus will be set in interrupt
  callback when the joystick button(DOWN, SEL or UP) is pressed */
  while (1)
  {
    if (DownStatus != RESET)
    {
      /* Reset DownStatus value */
      DownStatus = RESET;
      /* Execute DOWN Function */
      Menu_DownFunc();
    }
    else if (SelStatus != RESET)
    {
      /* Reset SelStatus value */
      SelStatus = RESET;
      /* Execute SEL Function */
      Menu_SelFunc();
    }
    else if (UpStatus != RESET)
    {
      /* Reset UpStatus value */
      UpStatus = RESET;
      /* Execute UP Function */
      Menu_UpFunc();
    }
    /* USER CODE END WHILE */
   
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
* @retval None
*/
static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef osc_init;
  RCC_ClkInitTypeDef clk_init;

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  osc_init.HSEState = RCC_HSE_ON;
  osc_init.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  osc_init.PLL.PLLState = RCC_PLL_ON;
  osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  osc_init.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&osc_init) != HAL_OK)
  {
    Error_Handler();
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
  clocks dividers */
  clk_init.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk_init.APB1CLKDivider = RCC_HCLK_DIV2;
  clk_init.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
  * @brief  Configure the LEDs pins as output push-pull: LED1, LED2, LED3
  *         and LED4
  * @param  None
  * @retval None
  */
static void Demo_LED_Init(void)
{
  /* Configure LEDs GPIO: as output push-pull */
  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);
  BSP_LED_Init(LED4);

  /* TIM2 clocks enable */
  __HAL_RCC_TIM2_CLK_ENABLE();

  TimHandleLED.Instance = TIM2;
  TimHandleLED.Init.Period = 7200;
  TimHandleLED.Init.Prescaler = 1000;
  TimHandleLED.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  TimHandleLED.Init.CounterMode = TIM_COUNTERMODE_UP;
  HAL_TIM_Base_Init(&TimHandleLED);

  HAL_NVIC_SetPriority(TIM2_IRQn, 0x0F, 0x00);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

/**
  * @brief  Enables or disables LED1, LED2, LED3 and LED4 toggling.
  * @param  NewState: FunctionalState ENABLE or DISABLE
  * @retval None
  */
void Demo_LedShow(FunctionalState NewState)
{
  /* Enable LEDs toggling */
  if (NewState == ENABLE)
  {
    LedShowStatus = 1;
    /* Enable the TIM Update Interrupt */
    HAL_TIM_Base_Start_IT(&TimHandleLED);

    /* TIM2 counter enable */
    __HAL_TIM_ENABLE(&TimHandleLED);
  }
  /* Disable LEDs toggling */
  else
  {
    LedShowStatus = 0;
    /* Disable the TIM Update Interrupt */
    HAL_TIM_Base_Stop_IT(&TimHandleLED);

    /* TIM2 counter disable */
    __HAL_TIM_ENABLE(&TimHandleLED);
  }
}
/**
  * @brief  Get the LedShowStatus value.
  * @param  None
  * @retval LedShowStatus Value.
  */
uint32_t Demo_Get_LedShowStatus(void)
{
  return LedShowStatus;
}

/**
  * @brief  Modify the LED state (blink).
  * @param  None
  * @retval None
  */
void Demo_LedShowCallback(void)
{
  /* If LedShowStatus is TRUE: enable LEDs toggling */
  if (Demo_Get_LedShowStatus() != 0)
  {
    switch (Index)
    {
        /* LD1 turned on, LD4 turned off */
      case 0:
      {
        BSP_LED_Off(LED1);
        BSP_LED_On(LED4);
        Index++;
        break;
      }
      /* LD2 turned on, LD1 turned off */
      case 1:
      {
        BSP_LED_Off(LED2);
        BSP_LED_On(LED1);
        Index++;
        break;
      }
      /* LD3 turned on, LD2 turned off */
      case 2:
      {
        BSP_LED_Off(LED3);
        BSP_LED_On(LED2);
        Index++;
        break;
      }
      /* LD4 turned on, LD3 turned off */
      case 3:
      {
        BSP_LED_Off(LED4);
        BSP_LED_On(LED3);
        Index++;
        break;
      }
      default:
        break;
    }
    /* Reset Index to replay LEDs switch on sequence  */
    if (Index == 4)
    {
      Index = 0;
    }
  }
}

/**
  * @brief  Keystroke callback function.
  * @param  GPIO_Pin: Activated GPIO pin
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  JOYState_TypeDef joykey = BSP_JOY_GetState();

  /* key press de-bounce */
  if (joykey != JOY_NONE)
  {
    while ( BSP_JOY_GetState() != JOY_NONE)
    {}
  }

  /* Setting global status */
  if ( joykey == JOY_SEL)
  {
    SelStatus = SET;
  }
  else if ( joykey == JOY_UP)
  {
    UpStatus = SET;
  }
  else if ( joykey == JOY_DOWN)
  {
    DownStatus = SET;
  }
  else if ( joykey == JOY_LEFT)
  {
    LeftStatus = SET;
  }
  else if ( joykey == JOY_RIGHT)
  {
    RightStatus = SET;
  }
  else if (BSP_PB_GetState(BUTTON_BACK) != GPIO_PIN_SET)
  {
    KeyStatus = SET;
  }
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
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @retval None
  */
void assert_failed(void)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* Infinite loop */
  while (1)
  {
  }

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
