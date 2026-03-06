/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main.h
  * @author  MCD Application Team
  * @brief   Header for main.c module
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx.h"
#include "menu.h"
#include "ir_common.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#ifdef USE_STM32373C_EVAL
#include "stm32f3xx_hal.h"
#include "stm32373c_eval.h"
#include "stm32373c_eval_lcd.h"
#endif /* USE_STM32373C_EVAL */
#ifdef USE_STM32303C_EVAL 
#include "stm32f3xx_hal.h"
#include "stm32303c_eval.h"
#include "stm32303c_eval_lcd.h"
#endif /* USE_STM32303C_EVAL */
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/

void Error_Handler(void);

/* USER CODE BEGIN EFP */
void Demo_LedShow(FunctionalState NewState);
uint32_t Demo_Get_LedShowStatus(void);
void Demo_LedShowCallback(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */
extern TIM_HandleTypeDef    TimHandleHF;
extern TIM_HandleTypeDef    TimHandleLF;
extern TIM_HandleTypeDef    TimHandleLED;
extern TIM_HandleTypeDef    TimHandleDEC;

extern __IO FlagStatus DownStatus;
extern __IO FlagStatus UpStatus;
extern __IO FlagStatus SelStatus;
extern __IO FlagStatus LeftStatus;
extern __IO FlagStatus RightStatus;
extern __IO FlagStatus KeyStatus;

#ifdef USE_STM32373C_EVAL
#define BUTTON_BACK BUTTON_TAMPER
#else
#define BUTTON_BACK BUTTON_KEY
#endif /* USE_STM32373C_EVAL */
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
