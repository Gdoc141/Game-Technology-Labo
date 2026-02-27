/**
  ******************************************************************************
  * @file    rc5_encode.c
  * @author  MCD Application Team
  * @brief   This file provides all the rc5 encode firmware functions
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
#include "rc5_encode.h"
#include "ir_common.h"

/* Private_Defines -----------------------------------------------------------*/
#define  RC5HIGHSTATE     ((uint8_t )0x02)   /* RC5 high level definition*/
#define  RC5LOWSTATE      ((uint8_t )0x01)   /* RC5 low level definition*/

/* Private_Function_Prototypes -----------------------------------------------*/
static uint16_t RC5_BinFrameGeneration(uint8_t RC5_Address, uint8_t RC5_Instruction, RC5_Ctrl_t RC5_Ctrl);
static uint32_t RC5_ManchesterConvert(uint16_t RC5_BinaryFrameFormat);
static void RC5_Encode_DeInit(void);

/* External Variables ---------------------------------------------------------*/
extern TIM_HandleTypeDef htim15;  /* Low Frequency Timer for RC5 bit timing */
extern TIM_HandleTypeDef htim16;  /* High Frequency Timer for 38kHz carrier */

/* Private macros for easier access */
#define TimHandleLF htim15
#define TimHandleHF htim16

/* Private_Variables ---------------------------------------------------------*/
uint8_t RC5RealFrameLength = 14;
uint8_t RC5GlobalFrameLength = 64;
uint16_t RC5BinaryFrameFormat = 0;
uint32_t RC5ManchesterFrameFormat = 0;
__IO uint32_t RC5SendOpCompleteFlag = 1;
__IO uint32_t RC5SendOpReadyFlag = 0;
RC5_Ctrl_t RC5Ctrl1 = RC5_CTRL_RESET;
uint8_t BitsSentCounter = 0;
uint8_t AddressIndex = 0;
uint8_t InstructionIndex = 0;
__IO StatusOperation_t RFDemoStatus = NONE;

/* Exported_Functions--------------------------------------------------------*/


/**
  * @brief  De-initializes the peripherals (GPIO, TIM)
  * @param  None
  * @retval None
  */
static void RC5_Encode_DeInit(void)
{
  HAL_TIM_OC_DeInit(&TimHandleLF);
  HAL_TIM_OC_DeInit(&TimHandleHF);
  HAL_GPIO_DeInit(IR_GPIO_PORT_HF, IR_GPIO_PIN_HF);
}

/**
  * @brief  RC5 encoder simplified demo (without LCD interface)
  * @param  None
  * @retval None
  */
void Menu_RC5_Encode_Func(void)
{
  RC5_Encode_Init();
  AddressIndex = 0;
  RFDemoStatus = RC5_ENC;
  InstructionIndex = 0;
  
  /* System is ready to send RC5 frames */
  /* Use RC5_Encode_SendFrame() to send commands */
}

/**
  * @brief Init Hardware (IPs used) for RC5 generation
  * @param None
  * @retval  None
  */
void RC5_Encode_Init(void)
{
  TIM_OC_InitTypeDef ch_config;
  GPIO_InitTypeDef gpio_init_struct;

  /* GPIO clocks already enabled in main.c MX_GPIO_Init() */
  
  /* Configure PA2 (TIM15_CH1 - Envelope) */
  gpio_init_struct.Pin = IR_GPIO_PIN_LF;
  gpio_init_struct.Mode = GPIO_MODE_AF_PP;
  gpio_init_struct.Pull = GPIO_NOPULL;
  gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio_init_struct.Alternate = IR_GPIO_AF_LF;
  HAL_GPIO_Init(IR_GPIO_PORT_LF, &gpio_init_struct);
  
  /* Configure PA6 (TIM16_CH1 - Carrier) */
  gpio_init_struct.Pin = IR_GPIO_PIN_HF;
  gpio_init_struct.Mode = GPIO_MODE_AF_PP;
  gpio_init_struct.Pull = GPIO_NOPULL;
  gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio_init_struct.Alternate = IR_GPIO_AF_HF;
  HAL_GPIO_Init(IR_GPIO_PORT_HF, &gpio_init_struct);

  /* Configure TIM16 Channel 1 for 38kHz carrier PWM */
  ch_config.OCMode = TIM_OCMODE_PWM1;
  ch_config.Pulse = IR_ENC_HPERIOD_RC5 / 4; /* 25% duty cycle */
  ch_config.OCPolarity = TIM_OCPOLARITY_HIGH;
  ch_config.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  ch_config.OCFastMode = TIM_OCFAST_DISABLE;
  ch_config.OCIdleState = TIM_OCIDLESTATE_RESET;
  ch_config.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&TimHandleHF, &ch_config, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  /* Start the High Frequency Timer PWM */
  if (HAL_TIM_PWM_Start(&TimHandleHF, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure TIM15 Channel 1 for Manchester envelope (Timing mode) */
  ch_config.OCMode = TIM_OCMODE_TIMING;
  ch_config.Pulse = IR_ENC_LPERIOD_RC5;
  ch_config.OCPolarity = TIM_OCPOLARITY_HIGH;
  ch_config.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  ch_config.OCFastMode = TIM_OCFAST_DISABLE;
  ch_config.OCIdleState = TIM_OCIDLESTATE_RESET;
  ch_config.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_OC_ConfigChannel(&TimHandleLF, &ch_config, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure and enable interrupt for Low Frequency Timer */
  HAL_NVIC_SetPriority(IR_TIM_LF_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(IR_TIM_LF_IRQn);

  /* TIM15 initially disabled - will be started by RC5_Encode_SendFrame() */
}

/**
  * @brief Generate and Send the RC5 frame.
  * @param RC5_Address : the RC5 Device destination
  * @param RC5_Instruction : the RC5 command instruction
  * @param RC5_Ctrl : the RC5 Control bit.
  * @retval  None
  */
void RC5_Encode_SendFrame(uint8_t RC5_Address, uint8_t RC5_Instruction, RC5_Ctrl_t RC5_Ctrl)
{
  /* Generate a binary format of the Frame */
  RC5BinaryFrameFormat = RC5_BinFrameGeneration(RC5_Address, RC5_Instruction, RC5_Ctrl);

  /* Generate a Manchester format of the Frame */
  RC5ManchesterFrameFormat = RC5_ManchesterConvert(RC5BinaryFrameFormat);

  /* Set the Send operation Ready flag to indicate that the frame is ready to be sent */
  RC5SendOpReadyFlag = 1;

  /* Reset the counter to ensure accurate timing of sync pulse */
  __HAL_TIM_SET_COUNTER( &TimHandleLF, 0);

  /* TIM IT Enable */
  HAL_TIM_Base_Start_IT(&TimHandleLF);
}

/**
  * @brief Send by hardware Manchester Format RC5 Frame.
  * @retval None
  */
void RC5_Encode_SignalGenerate(void)
{
  uint32_t bit_msg = 0;

  if ((RC5SendOpReadyFlag == 1) && (BitsSentCounter <= (RC5GlobalFrameLength * 2)))
  {
    RC5SendOpCompleteFlag = 0x00;
    bit_msg = (uint8_t)((RC5ManchesterFrameFormat >> BitsSentCounter) & 1);

    if (bit_msg == 1)
    {
      TIM_ForcedOC1Config(TIM_FORCED_ACTIVE);
    }
    else
    {
      TIM_ForcedOC1Config(TIM_FORCED_INACTIVE);
    }
    BitsSentCounter++;
  }
  else
  {
    RC5SendOpCompleteFlag = 0x01;

    /* TIM IT Disable */
    HAL_TIM_Base_Stop_IT(&TimHandleLF);
    RC5SendOpReadyFlag = 0;
    BitsSentCounter = 0;
    TIM_ForcedOC1Config(TIM_FORCED_INACTIVE);

    /* TIM Disable */
    __HAL_TIM_DISABLE(&TimHandleLF);
  }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief Generate the binary format of the RC5 frame.
  * @param RC5_Address : Select the device address.
  * @param RC5_Instruction : Select the device instruction.
  * @param RC5_Ctrl : Select the device control bit status.
  * @retval Binary format of the RC5 Frame.
  */
static uint16_t RC5_BinFrameGeneration(uint8_t RC5_Address, uint8_t RC5_Instruction, RC5_Ctrl_t RC5_Ctrl)
{
  uint16_t star1 = 0x2000;
  uint16_t star2 = 0x1000;
  uint16_t addr = 0;

  while (RC5SendOpCompleteFlag == 0x00)
  {}

  /* Check if Instruction is 128-bit length */
  if (RC5_Instruction >= 64)
  {
    /* Reset field bit: command is 7-bit length */
    star2 = 0;
    /* Keep the lowest 6 bits of the command */
    RC5_Instruction &= 0x003F;
  }
  else /* Instruction is 64-bit length */
  {
    /* Set field bit: command is 6-bit length */
    star2 = 0x1000;
  }

  RC5SendOpReadyFlag = 0;
  RC5ManchesterFrameFormat = 0;
  RC5BinaryFrameFormat = 0;
  addr = ((uint16_t)(RC5_Address)) << 6;
  RC5BinaryFrameFormat =  (star1) | (star2) | (RC5_Ctrl) | (addr) | (RC5_Instruction);
  return (RC5BinaryFrameFormat);
}

/**
  * @brief Convert the RC5 frame from binary to Manchester Format.
  * @param RC5_BinaryFrameFormat : the RC5 frame in binary format.
  * @retval the RC5 frame in Manchester format.
  */
static uint32_t RC5_ManchesterConvert(uint16_t RC5_BinaryFrameFormat)
{
  uint8_t i = 0;
  uint16_t Mask = 1;
  uint16_t bit_format = 0;
  uint32_t ConvertedMsg = 0;

  for (i = 0; i < RC5RealFrameLength; i++)
  {
    bit_format = ((((uint16_t)(RC5_BinaryFrameFormat)) >> i) & Mask) << i;
    ConvertedMsg = ConvertedMsg << 2;

    if (bit_format != 0 ) /* Manchester 1 -|_  */
    {
      ConvertedMsg |= RC5HIGHSTATE;
    }
    else /* Manchester 0 _|-  */
    {
      ConvertedMsg |= RC5LOWSTATE;
    }
  }
  return (ConvertedMsg);
}

/**
  * @brief  Force the TIM16 output compare channel to active or inactive
  * @param  action: TIM_FORCED_ACTIVE or TIM_FORCED_INACTIVE
  * @retval None
  */
void TIM_ForcedOC1Config(uint32_t action)
{
  TIM_TypeDef *TIMx = TimHandleHF.Instance;
  
  /* Disable the Channel 1 */
  TIMx->CCER &= ~TIM_CCER_CC1E;
  
  /* Set or Reset the Output Compare Mode */
  MODIFY_REG(TIMx->CCMR1, TIM_CCMR1_OC1M, action);
  
  /* Enable the Channel 1 */
  TIMx->CCER |= TIM_CCER_CC1E;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
