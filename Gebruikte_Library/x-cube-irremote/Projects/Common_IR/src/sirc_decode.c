/**
  ******************************************************************************
  * @file    sirc_decode.c
  * @author  MCD Application Team
  * @brief   This file provides all the sirc decode firmware functions.
  *
  * 1. How to use this driver
  * -------------------------
  *      - TIM3 Capture Compare and Update interrupts are used to decode the IR
  *        frame, if a frame is received correctly a global variable IRFrameReceived
  *        will be set to inform the application.
  *
  *      - Then the application should call the function SIRC_Decode() to retrieve
  *        the received IR frame.
  *
  *      - You can easily tailor this driver to any other InfraRed protocol,
  *        by just adapting the defines from sirc_decode.h to the InfraRed
  *        protocol specification(Bit Duration, Header Duration, Marge Tolerance,
  *        Number of bits...).
  *
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

/* Public_Constants ----------------------------------------------------------*/

/* Table of SIRC address */
const uint8_t* aSIRCDevices[16] =
  {
    (uint8_t*)"        SAT         ",  /* 0 */
    (uint8_t*)"         TV         ",  /* 1 */
    (uint8_t*)"        VTR1        ",  /* 2 */
    (uint8_t*)"      Teletext      ",  /* 3 */
    (uint8_t*)"     Widescreen     ",  /* 4 */
    (uint8_t*)"      Reserved      ",  /* 5 */
    (uint8_t*)"     laser Disk     ",  /* 6 */
    (uint8_t*)"        VTR2        ",  /* 7 */
    (uint8_t*)"      Reserved      ",  /* 8 */
    (uint8_t*)"      Reserved      ",  /* 9 */
    (uint8_t*)"      Reserved      ",  /* 10 */
    (uint8_t*)"        VTR3        ",  /* 11 */
    (uint8_t*)"Surround Sound Proc ",  /* 12 */
    (uint8_t*)"      Reserved      ",  /* 13 */
    (uint8_t*)"      Reserved      ",  /* 14 */
    (uint8_t*)"      Reserved      ",  /* 15 */
  };

/* Table of SIRC commands */
const uint8_t* aSIRCCommands[128] =
  {
    (uint8_t*)"        Num1        ",   /* 0 */
    (uint8_t*)"        Num2        ",   /* 1 */
    (uint8_t*)"        Num3        ",   /* 2 */
    (uint8_t*)"        Num4        ",   /* 3 */
    (uint8_t*)"        Num5        ",   /* 4 */
    (uint8_t*)"        Num6        ",   /* 5 */
    (uint8_t*)"        Num7        ",   /* 6 */
    (uint8_t*)"        Num8        ",   /* 7 */
    (uint8_t*)"        Num9        ",   /* 8 */
    (uint8_t*)"        Num0        ",   /* 9 */
    (uint8_t*)"       Reserved     ",   /* 10 */
    (uint8_t*)"        Enter       ",   /* 11 */
    (uint8_t*)"      Reserved      ",   /* 12 */
    (uint8_t*)"      Reserved      ",   /* 13 */
    (uint8_t*)"      Reserved      ",   /* 14 */
    (uint8_t*)"      Reserved      ",   /* 15 */
    (uint8_t*)"     Channel Up     ",   /* 16 */
    (uint8_t*)"     Channel Down   ",   /* 17 */
    (uint8_t*)"       Volume Up    ",   /* 18 */
    (uint8_t*)"      Volume Down   ",   /* 19 */
    (uint8_t*)"         Mute       ",   /* 20 */
    (uint8_t*)"        Power       ",   /* 21 */
    (uint8_t*)"        Reset       ",   /* 22 */
    (uint8_t*)"      Audio Mode    ",   /* 23 */
    (uint8_t*)"      Picture Up    ",   /* 24 */
    (uint8_t*)"     Picture Down   ",   /* 25 */
    (uint8_t*)"     Colour Up      ",   /* 26 */
    (uint8_t*)"     Colour Down    ",   /* 27 */
    (uint8_t*)"      Reserved      ",   /* 28 */
    (uint8_t*)"      Reserved      ",   /* 29 */
    (uint8_t*)"    Brightness Up   ",   /* 30 */
    (uint8_t*)"   Brightness Down  ",   /* 31 */
    (uint8_t*)"       Hue Up       ",   /* 32 */
    (uint8_t*)"      Hue Down      ",   /* 33 */
    (uint8_t*)"    Sharpness Up    ",   /* 34 */
    (uint8_t*)"   Sharpness Down   ",   /* 35 */
    (uint8_t*)"   Select TV Tuner  ",   /* 36 */
    (uint8_t*)"   Balance Left     ",   /* 37 */
    (uint8_t*)"   Balance Right    ",   /* 38 */
    (uint8_t*)"      Reserved      ",   /* 39 */
    (uint8_t*)"      Reserved      ",   /* 40 */
    (uint8_t*)"    Surround on/off ",   /* 41 */
    (uint8_t*)"      Reserved      ",   /* 42 */
    (uint8_t*)"      Reserved      ",   /* 43 */
    (uint8_t*)"      Reserved      ",   /* 44 */
    (uint8_t*)"      Reserved      ",   /* 45 */
    (uint8_t*)"      Reserved      ",   /* 46 */
    (uint8_t*)"     Power off      ",   /* 47 */
    (uint8_t*)"     Time Display   ",   /* 48 */
    (uint8_t*)"      Reserved      ",   /* 49 */
    (uint8_t*)"      Reserved      ",   /* 50 */
    (uint8_t*)"      Reserved      ",   /* 51 */
    (uint8_t*)"      Reserved      ",   /* 52 */
    (uint8_t*)"      Reserved      ",   /* 53 */
    (uint8_t*)"     Sleep Timer    ",   /* 54 */
    (uint8_t*)"      Reserved      ",   /* 55 */
    (uint8_t*)"      Reserved      ",   /* 56 */
    (uint8_t*)"      Reserved      ",   /* 57 */
    (uint8_t*)"   Channel Display  ",   /* 58 */
    (uint8_t*)"    Channel Jump    ",   /* 59 */
    (uint8_t*)"      Reserved      ",   /* 60 */
    (uint8_t*)"      Reserved      ",   /* 61 */
    (uint8_t*)"      Reserved      ",   /* 62 */
    (uint8_t*)"      Reserved      ",   /* 63 */
    (uint8_t*)" Select Input video1",   /* 64 */
    (uint8_t*)" Select Input video2",   /* 65 */
    (uint8_t*)" Select Input video3",   /* 66 */
    (uint8_t*)"      Reserved      ",   /* 67 */
    (uint8_t*)"      Reserved      ",   /* 68 */
    (uint8_t*)"      Reserved      ",   /* 69 */
    (uint8_t*)"      Reserved      ",   /* 70 */
    (uint8_t*)"      Reserved      ",   /* 71 */
    (uint8_t*)"      Reserved      ",   /* 72 */
    (uint8_t*)"      Reserved      ",   /* 73 */
    (uint8_t*)"noiseReductionon/off",   /* 74 */
    (uint8_t*)"      Reserved      ",   /* 75 */
    (uint8_t*)"      Reserved      ",   /* 76 */
    (uint8_t*)"      Reserved      ",   /* 77 */
    (uint8_t*)"  Cable/Broadcast   ",   /* 78 */
    (uint8_t*)" Notch Filter on/off",   /* 79 */
    (uint8_t*)"      Reserved      ",   /* 80 */
    (uint8_t*)"      Reserved      ",   /* 81 */
    (uint8_t*)"      Reserved      ",   /* 82 */
    (uint8_t*)"      Reserved      ",   /* 83 */
    (uint8_t*)"      Reserved      ",   /* 84 */
    (uint8_t*)"      Reserved      ",   /* 85 */
    (uint8_t*)"      Reserved      ",   /* 86 */
    (uint8_t*)"      Reserved      ",   /* 87 */
    (uint8_t*)"   PIP Channel Up   ",   /* 88 */
    (uint8_t*)"   PIP Channel Down ",   /* 89 */
    (uint8_t*)"      Reserved      ",   /* 90 */
    (uint8_t*)"      PIP on        ",   /* 91 */
    (uint8_t*)"    Freeze Screen   ",   /* 92 */
    (uint8_t*)"      Reserved      ",   /* 93 */
    (uint8_t*)"     PIP Position   ",   /* 94 */
    (uint8_t*)"      PIP Swap      ",   /* 95 */
    (uint8_t*)"        Guide       ",   /* 96 */
    (uint8_t*)"     Video Setup    ",   /* 97 */
    (uint8_t*)"     Audio Setup    ",   /* 98 */
    (uint8_t*)"     Exit Setup     ",   /* 99 */
    (uint8_t*)"      Reserved      ",   /* 100 */
    (uint8_t*)"      Reserved      ",   /* 101 */
    (uint8_t*)"      Reserved      ",   /* 102 */
    (uint8_t*)"      Reserved      ",   /* 103 */
    (uint8_t*)"      Reserved      ",   /* 104 */
    (uint8_t*)"      Reserved      ",   /* 105 */
    (uint8_t*)"      Reserved      ",   /* 106 */
    (uint8_t*)"    Auto Program    ",   /* 107 */
    (uint8_t*)"      Reserved      ",   /* 108 */
    (uint8_t*)"      Reserved      ",   /* 109 */
    (uint8_t*)"      Reserved      ",   /* 110 */
    (uint8_t*)"       PIP off      ",   /* 111 */
    (uint8_t*)"       Treble Up    ",   /* 112 */
    (uint8_t*)"      Treble Down   ",   /* 113 */
    (uint8_t*)"       Bass Up      ",   /* 114 */
    (uint8_t*)"      Bass Down     ",   /* 115 */
    (uint8_t*)"       + Key        ",   /* 116 */
    (uint8_t*)"       - Key        ",   /* 117 */
    (uint8_t*)"      Reserved      ",   /* 118 */
    (uint8_t*)"    Slipt Screen    ",   /* 119 */
    (uint8_t*)"    Add Channel     ",   /* 120 */
    (uint8_t*)"   Delete Channel   ",   /* 121 */
    (uint8_t*)"      Reserved      ",   /* 122 */
    (uint8_t*)"      Reserved      ",   /* 123 */
    (uint8_t*)"      Reserved      ",   /* 124 */
    (uint8_t*)"  Trinitone on/off  ",   /* 125 */
    (uint8_t*)"      Reserved      ",   /* 126 */
    (uint8_t*)"Display a red Rtests"    /* 127 */
  };

/* Private_Constants ---------------------------------------------------------*/
#define SIRC_TIME_OUT_US                ((uint32_t)4050)

#define SIRC_STATUS_HEADER              ((uint8_t)(1 << 1))
#define SIRC_STATUS_RX                  ((uint8_t)(1 << 0))
#define INITIAL_STATUS                  SIRC_STATUS_HEADER

#define SIRC_BIT_ERROR                  ((uint8_t)0xFF)
#define SIRC_HEADER_ERROR               ((uint8_t)0xFF)
#define SIRC_HEADER_OK                  ((uint8_t)0x00)

#define SIRC_BITS_COUNT                 ((uint8_t)11)
#define SIRC_TOTAL_BITS_COUNT           ((uint8_t)11)

#define SIRC_ONTIME_MIN_US              ((uint32_t)(600 - 60))
#define SIRC_ONTIME_MAX_US              ((uint32_t)(1200 + 60))

#define SIRC_HEADER_LOW_MIN_US          ((uint32_t)(2400 - 150))
#define SIRC_HEADER_LOW_MAX_US          ((uint32_t)(2400 + 150))
#define SIRC_HEADER_WHOLE_MIN_US        ((uint32_t)(2400 + 600 - 60))
#define SIRC_HEADER_WHOLE_MAX_US        ((uint32_t)(2400 + 600 + 60))

#define SIRC_VALUE_STEP_US              ((uint32_t)600)
#define SIRC_VALUE_MARGIN_US            ((uint32_t)100)
#define SIRC_VALUE_00_US                ((uint32_t)1200)

/* Private_Variables ---------------------------------------------------------*/

__IO StatusYesOrNo_t IRFrameReceived = NO;   /*!< IR frame status */

/* IR bit definitions */
uint16_t IROnTimeMin = 0;
uint16_t IROnTimeMax = 0;
uint16_t IRValueStep = 0;
uint16_t IRValueMargin = 0;
uint16_t IRValue00 = 0;

/* Header time definitions */
uint16_t IRHeaderLowMin = 0;
uint16_t IRHeaderLowMax = 0;
uint16_t IRHeaderWholeMin = 0;
uint16_t IRHeaderWholeMax = 0;
SIRC_packet_t IRTmpPacket; /*!< IR packet*/

uint16_t IRTimeOut = 0;
static uint32_t TIMCLKValueKHz = 0;/*!< Timer clock */

SIRC_Frame_t SIRC_Frame;

/* Private_Function_Prototypes -----------------------------------------------*/

static uint8_t SIRC_DecodeHeader(uint32_t lowPulseLength, uint32_t wholePulseLength);
static uint8_t SIRC_DecodeBit(uint32_t lowPulseLength, uint32_t wholePulseLength);
static uint32_t SIRC_ReverseBit(uint32_t data);
static void SIRC_DeInit(void);
static void SIRC_Init(void);
static void SIRC_Decode(SIRC_Frame_t *ir_frame);

/* Private_Functions ---------------------------------------------------------*/
/**
  * @brief  SIRC receiver demo exec.
  * @param  None
  * @retval None
  */
void Menu_SIRCDecode_Func(void)
{
  while (Menu_ReadKey() != NOKEY)
  {}
  /* Clear the LCD */
  BSP_LCD_Clear(LCD_COLOR_WHITE);

  BSP_LCD_SetFont(&Font24);
  /* Set the LCD Back Color */
  BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
  /* Set the LCD Text Color */
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_DisplayStringAtLine(0, (uint8_t*)"   SIRC Receiver  ");
  BSP_LCD_DisplayStringAtLine(9, (uint8_t*)"To exit press KEY ");

  /* Set the LCD Back Color */
  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
  /* Set the LCD Text Color */
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  /* Initialize the InfraRed application: SIRC */
  RFDemoStatus = SIRC_DEC;
  SIRC_Init();

  while (Menu_ReadKey() != KEY)
  {
    /* SIRC frame decode */
    SIRC_Decode(&SIRC_Frame);
  }

  /* Clear LCD */
  BSP_LCD_Clear(LCD_COLOR_WHITE);

  /* Exit the SIRC demo */
  RFDemoStatus = NONE;
  SIRC_DeInit();

  /* Enable the JoyStick interrupt */
  HAL_NVIC_EnableIRQ(LEFT_JOY_EXTI_IRQn);

  /* Display menu */
  Menu_DisplayMenu();
}

/**
  * @brief  De-initializes the peripherals (RCC, GPIO, TIM)
  * @param  None
  * @retval None
  */
void SIRC_DeInit(void)
{
  HAL_TIM_IC_DeInit(&TimHandleDEC);
  HAL_GPIO_DeInit(IR_GPIO_PORT, IR_GPIO_PIN);
}

/**
  * @brief  Initialize the IR bit time range.
  * @param  None
  * @retval None
  */
void SIRC_Init(void)
{
  GPIO_InitTypeDef gpio_init_struct;
  TIM_IC_InitTypeDef tim_ic_init;
  TIM_SlaveConfigTypeDef tim_slave_conf;
  TIM_MasterConfigTypeDef tim_master_conf;

  /*  Clock Configuration for TIMER */
  IR_TIM_CLK();

  /* Enable Button GPIO clock */
  IR_GPIO_PORT_CLK();

  /* Pin configuration: input floating */
  gpio_init_struct.Pin = IR_GPIO_PIN;
  gpio_init_struct.Mode = GPIO_MODE_AF_OD;
  gpio_init_struct.Pull = GPIO_NOPULL;
  gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio_init_struct.Alternate = IR_GPIO_AF_REC;
  HAL_GPIO_Init(IR_GPIO_PORT, &gpio_init_struct);

  /* Enable the TIMx global Interrupt */
  HAL_NVIC_SetPriority(IR_TIM_IRQn, 0, 1);
  HAL_NVIC_EnableIRQ(IR_TIM_IRQn);

  /* Timer Clock */
  TIMCLKValueKHz = TIM_GetCounterCLKValue() / 1000;
  IRTimeOut = TIMCLKValueKHz * SIRC_TIME_OUT_US / 1000;

  TimHandleDEC.Instance = IR_TIM;
  TimHandleDEC.Init.ClockDivision = 0;
  TimHandleDEC.Init.CounterMode = 0;
  TimHandleDEC.Init.Period = IRTimeOut;
  TimHandleDEC.Init.Prescaler = TIM_PRESCALER;
  TimHandleDEC.Init.RepetitionCounter = 0;
  HAL_TIM_IC_Init(&TimHandleDEC);

  /* TIM configuration */
  tim_ic_init.ICPolarity = TIM_ICPOLARITY_RISING;
  tim_ic_init.ICSelection = TIM_ICSELECTION_INDIRECTTI;
  tim_ic_init.ICPrescaler = TIM_ICPSC_DIV1;
  tim_ic_init.ICFilter = 0x0;
  HAL_TIM_IC_ConfigChannel(&TimHandleDEC, &tim_ic_init, IR_TIM_DEC_CHANNEL_B);
  tim_ic_init.ICPolarity = TIM_ICPOLARITY_FALLING;
  tim_ic_init.ICSelection = TIM_ICSELECTION_DIRECTTI;
  HAL_TIM_IC_ConfigChannel(&TimHandleDEC, &tim_ic_init, IR_TIM_DEC_CHANNEL_A);

  tim_slave_conf.InputTrigger = IR_TIM_DEC_TRIGGER;
  tim_slave_conf.SlaveMode = TIM_SLAVEMODE_RESET;
  tim_slave_conf.TriggerPolarity = TIM_TRIGGERPOLARITY_FALLING;
  tim_slave_conf.TriggerFilter = 0;
  HAL_TIM_SlaveConfigSynchro( &TimHandleDEC, &tim_slave_conf);

  /* Enable the Master/Slave Mode */
  tim_master_conf.MasterSlaveMode = TIM_SMCR_MSM;
  tim_master_conf.MasterOutputTrigger = 0;
#ifdef USE_STM32303C_EVAL
  tim_master_conf.MasterOutputTrigger2 = 0;
#endif
  HAL_TIMEx_MasterConfigSynchronization( &TimHandleDEC, &tim_master_conf);

  /* Configures the TIM Update Request Interrupt source: counter overflow */
  __HAL_TIM_URS_ENABLE(&TimHandleDEC);

  /* Clear update flag */
  __HAL_TIM_CLEAR_FLAG( &TimHandleDEC, TIM_FLAG_UPDATE);

  /* Enable TIM Update Event Interrupt Request */
  /* Enable the CC2/CC1 Interrupt Request */
  __HAL_TIM_ENABLE_IT( &TimHandleDEC, TIM_FLAG_UPDATE | TIM_IT_CC1 | TIM_IT_CC2);

  /* Enable the timer */
  __HAL_TIM_ENABLE(&TimHandleDEC);

  if (HAL_TIM_IC_Start_IT(&TimHandleDEC, IR_TIM_DEC_CHANNEL_A) != HAL_OK)
  {
    /* Starting Error */
    while (1)
    {}
  }

  if (HAL_TIM_IC_Start_IT(&TimHandleDEC, IR_TIM_DEC_CHANNEL_B) != HAL_OK)
  {
    /* Starting Error */
    while (1)
    {}
  }

  /* Header */
  IRHeaderLowMin = TIMCLKValueKHz * SIRC_HEADER_LOW_MIN_US / 1000;
  IRHeaderLowMax = TIMCLKValueKHz * SIRC_HEADER_LOW_MAX_US / 1000;
  IRHeaderWholeMin = TIMCLKValueKHz * SIRC_HEADER_WHOLE_MIN_US / 1000;
  IRHeaderWholeMax = TIMCLKValueKHz * SIRC_HEADER_WHOLE_MAX_US / 1000;

  /* Bit time range*/
  IROnTimeMax = TIMCLKValueKHz * SIRC_ONTIME_MAX_US / 1000;
  IROnTimeMin = TIMCLKValueKHz * SIRC_ONTIME_MIN_US / 1000;
  IRValueStep = TIMCLKValueKHz * SIRC_VALUE_STEP_US / 1000;
  IRValueMargin = TIMCLKValueKHz * SIRC_VALUE_MARGIN_US / 1000;
  IRValue00 = TIMCLKValueKHz * SIRC_VALUE_00_US / 1000;

  /* Default state */
  SIRC_ResetPacket();
}

/**
  * @brief  Decode the IR frame (ADDRESS, COMMAND) when all the frame is
  *         received, the IRFrameReceived will equal to YES.
  *         The IRFrameReceived is set to YES inside the  SIRC_DataSampling()
  *         function when the whole IR frame is received without any error.
  *         If any received bit timing is out of the defined range, the IR packet
  *         is reset and the IRFrameReceived is set to NO.
  *         After the IR received display, the IRFrameReceived is set NO.
  *         User can check the IRFrameReceived variable status to verify if there
  *         is a new IR frame already received.
  * @param  pIRFrame: pointer to SIRC_Frame_TypeDef structure that contains the
  *         the IR protocol fields (Address, Command,...).
  * @retval None
  */
void SIRC_Decode(SIRC_Frame_t *pIRFrame)
{
  uint32_t irframedata = 0;

  if (IRFrameReceived != NO)
  {
    /* Disable IR_TIM */
    __HAL_TIM_DISABLE(&TimHandleDEC);

    /* Extract address and command from the received bit sequence */
    irframedata = SIRC_ReverseBit(IRTmpPacket.data);
    pIRFrame->Command = (irframedata >> 20) & 0x7F;
    pIRFrame->Address = (irframedata >> 27) & 0x1F;

    /* Default state */
    IRFrameReceived = NO;
    SIRC_ResetPacket();

    /* Display command and address */
    BSP_LCD_SetFont(&Font20);
    BSP_LCD_DisplayStringAtLine(6, (uint8_t*)aSIRCCommands[pIRFrame->Command]);
    BSP_LCD_DisplayStringAtLine(7, (uint8_t*)aSIRCDevices[pIRFrame->Address]);
    BSP_LCD_SetFont(&Font24);

    /* Enable IR_TIM */
    __HAL_TIM_ENABLE(&TimHandleDEC);
  }
}

/**
  * @brief  Put the packet to default state, ready to be filled in by incoming
  *         data.
  * @param  None
  * @retval None
  */
void SIRC_ResetPacket(void)
{
  IRTmpPacket.count = 0;
  IRTmpPacket.status = INITIAL_STATUS;
  IRTmpPacket.data = 0;
}

/**
  * @brief  Identify the IR data.
  * @param  lowPulseLength: low pulse duration.
  * @param  wholePulseLength: whole pulse duration.
  * @retval None
  */
void SIRC_DataSampling(uint32_t lowPulseLength, uint32_t wholePulseLength)
{
  uint8_t  tmp_bit = 0;

  /* If the pulse timing is correct */
  if ((IRTmpPacket.status == SIRC_STATUS_RX))
  {
    /* Convert raw pulse timing to data value */
    tmp_bit = SIRC_DecodeBit(lowPulseLength, wholePulseLength);
    if (tmp_bit != SIRC_BIT_ERROR) /* If the pulse timing is correct */
    {
      /* This operation fills in the incoming bit to the correct position in
      32 bit word*/
      IRTmpPacket.data |= tmp_bit;
      IRTmpPacket.data <<= 1;
      /* Increment the bit count  */
      IRTmpPacket.count++;
    }
    /* If all bits identified */
    if (IRTmpPacket.count == SIRC_TOTAL_BITS_COUNT)
    {
      /* Frame received*/
      IRFrameReceived = YES;
    }
    /* Bit 15:the idle time between IR message and the inverted one */
    else if (IRTmpPacket.count == SIRC_BITS_COUNT)
    {
      IRTmpPacket.status = SIRC_STATUS_HEADER;
    }
  }
  else if ((IRTmpPacket.status == SIRC_STATUS_HEADER))
  {
    /* Convert raw pulse timing to data value */
    tmp_bit = SIRC_DecodeHeader(lowPulseLength, wholePulseLength);

    /* If the Header timing is correct */
    if ( tmp_bit != SIRC_HEADER_ERROR)
    {
      /* Reception Status for the inverted message */
      IRTmpPacket.status = SIRC_STATUS_RX;
    }
    else
    {
      /* Wrong header time */
      SIRC_ResetPacket();
    }
  }
}

/**
  * @brief  Check the header pulse if it has correct pulse time.
  * @param  lowPulseLength: low pulse header duration.
  * @param  wholePulseLength: whole pulse header duration.
  * @retval SIRC_HEADER_OK or SIRC_HEADER_ERROR
  */
static uint8_t SIRC_DecodeHeader(uint32_t lowPulseLength, uint32_t wholePulseLength)
{
  /* First check if low pulse time is according to the specs */
  if ((lowPulseLength >= IRHeaderLowMin) && (lowPulseLength <= IRHeaderLowMax))
  {
    /* Check if the high pulse is OK */
    if ((wholePulseLength >= IRHeaderWholeMin) && (wholePulseLength <= IRHeaderWholeMax))
    {
      return SIRC_HEADER_OK; /* Valid Header */
    }
  }
  return SIRC_HEADER_ERROR;  /* Wrong Header */
}

/**
  * @brief  Convert raw time to data value.
  * @param  lowPulseLength: low pulse bit duration.
  * @param  wholePulseLength: whole pulse bit duration.
  * @retval BitValue( data0 or data1) or SIRC_BIT_ERROR
  */
static uint8_t SIRC_DecodeBit(uint32_t lowPulseLength , uint32_t wholePulseLength)
{
  uint8_t i = 0;

  /* First check if low pulse time is according to the specs */
  if ((lowPulseLength >= IROnTimeMin) && (lowPulseLength <= IROnTimeMax))
  {
    for (i = 0; i < 2; i++) /* There are data0 to data1 */
    {
      /* Check if the length is in given range */
      if ((wholePulseLength >= IRValue00 + (IRValueStep * i) - IRValueMargin)
          && (wholePulseLength <= IRValue00 + (IRValueStep * i) + IRValueMargin))
        return i; /* Return bit value */
    }
  }
  return SIRC_BIT_ERROR; /* No correct pulse length was found */
}

/**
  * @brief  Reverse bit
  * @param  data: data to be inverted
  * @retval None
  */
static uint32_t SIRC_ReverseBit(uint32_t data)
{
  uint32_t i = 0;
  uint32_t j = 0;

  for (j = i = 0; i < 32; i++)
  {
    j = (j << 1) + (data & 1);
    data >>= 1;
  }

  return j;
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
