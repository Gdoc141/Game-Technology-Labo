/**
  ******************************************************************************
  * @file    stm32f0xx_it.c
  * @author  MCD Application Team
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and
  *          peripherals interrupt service routine.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f0xx_it.h"

/** @addtogroup IR_REMOTE
  * @{
  */

/** @addtogroup Global
  * @{
  */

/** @defgroup Global_Private_Variables
  * @{
  */
__IO uint8_t Index = 0;

/**
  * @}
  */

/** @addtogroup Global_Exported_Functions
  * @{
  */

/******************************************************************************/
/*            Cortex-M0 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

/******************************************************************************/
/*                 STM32F0xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f0xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles External lines 0-1 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI0_1_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(SEL_JOY_PIN);
}

/**
  * @brief  This function handles External lines 2 to 3 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI2_3_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(LEFT_JOY_PIN | RIGHT_JOY_PIN);
}

/**
  * @brief  This function handles External line 4 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI4_15_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(UP_JOY_PIN | DOWN_JOY_PIN | TAMPER_BUTTON_PIN);
}

/**
  * @brief  This function handles TIM2 overflow and update interrupt request.
  * @param  None
  * @retval None
  */
void TIM2_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&TimHandleLED);

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
  * @brief  This function handles TIM3 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM3_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&TimHandleDEC);
}


/**
  * @brief  This function handles TIM16 interrupt request.
  * @param  None
  * @retval None
  */
void TIM16_IRQHandler(void)
{
  /* Call the handler to clear TIM16 update interrupt flag */
  HAL_TIM_IRQHandler(&TimHandleLF);

}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
