/**
  ******************************************************************************
  * @file    rc5_decode.c
  * @author  MCD Application Team
  * @brief   This file provides all the RC5 firmware functions.
  *
  * 1. How to use this driver
  * -------------------------
  *      - Call the function RC5_Init() to configure the Timer and GPIO hardware
  *        resources needed for RC5 decoding.
  *
  *      - TIM3 Capture Compare and Update interrupts are used to decode the RC5
  *        frame, if a frame is received correctly a global variable RC5FrameReceived
  *        will be set to inform the application.
  *
  *      - Then the application should call the function RC5_Decode() to retrieve
  *        the received RC5 frame.
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
#include "rc5_decode.h"

/* Public_Constants ----------------------------------------------------------*/
const uint8_t* aRC5Devices[32] =
  {
    (uint8_t*)"       TV1          ",                  /*  0 */
    (uint8_t*)"       TV2          ",                  /*  1 */
    (uint8_t*)"    Video Text      ",                  /*  2 */
    (uint8_t*)"   Extension TV     ",                  /*  3 */
    (uint8_t*)"  LaserVideoPlayer  ",                  /*  4 */
    (uint8_t*)"       VCR1         ",                  /*  5 */
    (uint8_t*)"       VCR2         ",                  /*  6 */
    (uint8_t*)"      Reserved      ",                  /*  7 */
    (uint8_t*)"       Sat1         ",                  /*  8 */
    (uint8_t*)"   Extension VCR    ",                  /*  9 */
    (uint8_t*)"       Sat2         ",                  /* 10 */
    (uint8_t*)"     Reserved       ",                  /* 11 */
    (uint8_t*)"     CD Video       ",                  /* 12 */
    (uint8_t*)"      Reserved      ",                  /* 13 */
    (uint8_t*)"     CD Photo       ",                  /* 14 */
    (uint8_t*)"      Reserved      ",                  /* 15 */
    (uint8_t*)" Preampli Audio 1   ",                  /* 16 */
    (uint8_t*)"      Tuner         ",                  /* 17 */
    (uint8_t*)"  Analog Magneto    ",                  /* 18 */
    (uint8_t*)" Preampli Audio 2   ",                  /* 19 */
    (uint8_t*)"       CD           ",                  /* 20 */
    (uint8_t*)"    Rack Audio      ",                  /* 21 */
    (uint8_t*)" Audio Sat Receiver ",                  /* 22 */
    (uint8_t*)"   DDC Magneto      ",                  /* 23 */
    (uint8_t*)"     Reserved       ",                  /* 24 */
    (uint8_t*)"     Reserved       ",                  /* 25 */
    (uint8_t*)"      CDRW          ",                  /* 26 */
    (uint8_t*)"     Reserved       ",                  /* 27 */
    (uint8_t*)"     Reserved       ",                  /* 28 */
    (uint8_t*)"     Reserved       ",                  /* 29 */
    (uint8_t*)"     Reserved       ",                  /* 30 */
    (uint8_t*)"     Reserved       "                   /* 31 */
  };

/* RC5 commands table*/
const uint8_t* aRC5Commands[128] =
  {
    (uint8_t*)"       Num0         ",                                       /* 0 */
    (uint8_t*)"       Num1         ",                                       /* 1 */
    (uint8_t*)"       Num2         ",                                       /* 2 */
    (uint8_t*)"       Num3         ",                                       /* 3 */
    (uint8_t*)"       Num4         ",                                       /* 4 */
    (uint8_t*)"       Num5         ",                                       /* 5 */
    (uint8_t*)"       Num6         ",                                       /* 6 */
    (uint8_t*)"       Num7         ",                                       /* 7 */
    (uint8_t*)"       Num8         ",                                       /* 8 */
    (uint8_t*)"       Num9         ",                                       /* 9 */
    (uint8_t*)"     TV Digits      ",                                       /* 10 */
    (uint8_t*)"      TV Freq       ",                                       /* 11 */
    (uint8_t*)"     TV StandBy     ",                                       /* 12 */
    (uint8_t*)"  TV Mute On-Off    ",                                       /* 13 */
    (uint8_t*)"   TV Preference    ",                                       /* 14 */
    (uint8_t*)"    TV Display      ",                                       /* 15 */
    (uint8_t*)"    Volume Up       ",                                       /* 16 */
    (uint8_t*)"    Volume Down     ",                                       /* 17 */
    (uint8_t*)"    Brightness Up   ",                                       /* 18 */
    (uint8_t*)"   Brightness Down  ",                                       /* 19 */
    (uint8_t*)" Color Saturation Up",                                       /* 20 */
    (uint8_t*)"ColorSaturation Down",                                       /* 21 */
    (uint8_t*)"      Bass Up       ",                                       /* 22 */
    (uint8_t*)"      Bass Down     ",                                       /* 23 */
    (uint8_t*)"      Treble Up     ",                                       /* 24 */
    (uint8_t*)"     Treble Down    ",                                       /* 25 */
    (uint8_t*)"    Balance Right   ",                                       /* 26 */
    (uint8_t*)"    BalanceLeft     ",                                       /* 27 */
    (uint8_t*)"   TV Contrast Up   ",                                       /* 28 */
    (uint8_t*)"  TV Contrast Down  ",                                       /* 29 */
    (uint8_t*)"   TV Search Up     ",                                       /* 30 */
    (uint8_t*)"  TV tint-hue Down  ",                                       /* 31 */
    (uint8_t*)"   TV CH Prog Up    ",                                       /* 32 */
    (uint8_t*)"   TV CH ProgDown   ",                                       /* 33 */
    (uint8_t*)"TV Last prog-channel",                                       /* 34 */
    (uint8_t*)" TV Select language ",                                       /* 35 */
    (uint8_t*)" TV Spacial Stereo  ",                                       /* 36 */
    (uint8_t*)"  TV Stereo Mono    ",                                       /* 37 */
    (uint8_t*)"  TV Sleep Timer    ",                                       /* 38 */
    (uint8_t*)"   TV tint-hue Up   ",                                       /* 39 */
    (uint8_t*)"   TV RF Switch     ",                                       /* 40 */
    (uint8_t*)"   TV Store-VOTE    ",                                       /* 41 */
    (uint8_t*)"      TV Time       ",                                       /* 42 */
    (uint8_t*)"   TV Scan Fwd Inc  ",                                       /* 43 */
    (uint8_t*)"    TV Decrement    ",                                       /* 44 */
    (uint8_t*)"      Reserved      ",                                       /* 45 */
    (uint8_t*)"   TV control-menu  ",                                       /* 46 */
    (uint8_t*)"    TV Show Clock   ",                                       /* 47 */
    (uint8_t*)"      TV Pause      ",                                       /* 48 */
    (uint8_t*)"   TV Erase Entry   ",                                       /* 49 */
    (uint8_t*)"     TV Rewind      ",                                       /* 50 */
    (uint8_t*)"     TV Goto        ",                                       /* 51 */
    (uint8_t*)"     TV Wind        ",                                       /* 52 */
    (uint8_t*)"     TV Play        ",                                       /* 53 */
    (uint8_t*)"     TV Stop        ",                                       /* 54 */
    (uint8_t*)"     TV Record      ",                                       /* 55 */
    (uint8_t*)"    TV External 1   ",                                       /* 56 */
    (uint8_t*)"    TV External 2   ",                                       /* 57 */
    (uint8_t*)"     Reserved       ",                                       /* 58 */
    (uint8_t*)"     TV Advance     ",                                       /* 59 */
    (uint8_t*)"   TV TXT-TV toggle ",                                       /* 60 */
    (uint8_t*)"  TV System StandBy ",                                       /* 61 */
    (uint8_t*)"TV Picture Crispener",                                       /* 62 */
    (uint8_t*)"    System Select   ",                                       /* 63 */
    (uint8_t*)"     Reserved       ",                                       /* 64 */
    (uint8_t*)"     Reserved       ",                                       /* 65 */
    (uint8_t*)"     Reserved       ",                                       /* 66 */
    (uint8_t*)"     Reserved       ",                                       /* 67 */
    (uint8_t*)"     Reserved       ",                                       /* 68 */
    (uint8_t*)"     Reserved       ",                                       /* 69 */
    (uint8_t*)"  TV Speech Music   ",                                       /* 70 */
    (uint8_t*)"  DIM Local Display ",                                       /* 71 */
    (uint8_t*)"     Reserved       ",                                       /* 72 */
    (uint8_t*)"     Reserved       ",                                       /* 73 */
    (uint8_t*)"     Reserved       ",                                       /* 74 */
    (uint8_t*)"     Reserved       ",                                       /* 75 */
    (uint8_t*)"     Reserved       ",                                       /* 76 */
    (uint8_t*)"Inc Control Setting ",                                       /* 77 */
    (uint8_t*)"Dec Control Setting ",                                       /* 78 */
    (uint8_t*)"   TV Sound Scroll  ",                                       /* 79 */
    (uint8_t*)"      Step Up       ",                                       /* 80 */
    (uint8_t*)"     Step Down      ",                                       /* 81 */
    (uint8_t*)"      Menu On       ",                                       /* 82 */
    (uint8_t*)"      Menu Off      ",                                       /* 83 */
    (uint8_t*)"     AV Status      ",                                       /* 84 */
    (uint8_t*)"      Step Left     ",                                       /* 85 */
    (uint8_t*)"      Step Right    ",                                       /* 86 */
    (uint8_t*)"     Acknowledge    ",                                       /* 87 */
    (uint8_t*)"      PIP On Off    ",                                       /* 88 */
    (uint8_t*)"      PIP Shift     ",                                       /* 89 */
    (uint8_t*)"    PIP Main Swap   ",                                       /* 90 */
    (uint8_t*)"    Strobe On Off   ",                                       /* 91 */
    (uint8_t*)"     Multi Strobe   ",                                       /* 92 */
    (uint8_t*)"      Main Frozen   ",                                       /* 93 */
    (uint8_t*)"  3div9 Multi scan  ",                                       /* 94 */
    (uint8_t*)"      PIPSelect     ",                                       /* 95 */
    (uint8_t*)"      MultiPIP      ",                                       /* 96 */
    (uint8_t*)"     Picture DNR    ",                                       /* 97 */
    (uint8_t*)"     Main Stored    ",                                       /* 98 */
    (uint8_t*)"     PIP Strobe     ",                                       /* 99 */
    (uint8_t*)"    Stored Picture  ",                                       /* 100 */
    (uint8_t*)"      PIP Freeze    ",                                       /* 101 */
    (uint8_t*)"      PIP Step Up   ",                                       /* 102 */
    (uint8_t*)"    PIP Step Down   ",                                       /* 103 */
    (uint8_t*)"    TV PIP Size     ",                                       /* 104 */
    (uint8_t*)"  TV Picture Scroll ",                                       /* 105 */
    (uint8_t*)" TV Actuate Colored ",                                       /* 106 */
    (uint8_t*)"       TV Red       ",                                       /* 107 */
    (uint8_t*)"      TV Green      ",                                       /* 108 */
    (uint8_t*)"      TV Yellow     ",                                       /* 109 */
    (uint8_t*)"      TV Cyan       ",                                       /* 110 */
    (uint8_t*)"    TV Index White  ",                                       /* 111 */
    (uint8_t*)"      TV Next       ",                                       /* 112 */
    (uint8_t*)"     TV Previous    ",                                       /* 113 */
    (uint8_t*)"      Reserved      ",                                       /* 114 */
    (uint8_t*)"      Reserved      ",                                       /* 115 */
    (uint8_t*)"      Reserved      ",                                       /* 116 */
    (uint8_t*)"      Reserved      ",                                       /* 117 */
    (uint8_t*)"      Sub Mode      ",                                       /* 118 */
    (uint8_t*)"   Option Sub Mode  ",                                       /* 119 */
    (uint8_t*)"      Reserved      ",                                       /* 120 */
    (uint8_t*)"      Reserved      ",                                       /* 121 */
    (uint8_t*)"TV Store Open Close ",                                       /* 122 */
    (uint8_t*)"      Connect       ",                                       /* 123 */
    (uint8_t*)"     Disconnect     ",                                       /* 124 */
    (uint8_t*)"      Reserved      ",                                       /* 125 */
    (uint8_t*)"  TV Movie Expand   ",                                       /* 126 */
    (uint8_t*)"  TV Parental Access"                                        /* 127 */
  };

/* Private_Constants ---------------------------------------------------------*/

#define RC5_1T_TIME                          ((uint8_t)0x00)
#define RC5_2T_TIME                          ((uint8_t)0x01)
#define RC5_WRONG_TIME                       ((uint8_t)0xFF)
#define RC5_TIME_OUT_US                      ((uint32_t)3600)
#define RC5_MIN_1T_US                        ((uint32_t)640)
#define RC5_MAX_1T_US                        ((uint32_t)1140)
#define RC5_MIN_2T_US                        ((uint32_t)1340)
#define RC5_MAX_2T_US                        ((uint32_t)2220)
#define RC5_NUMBER_OF_VALID_PULSE_LENGTH     ((uint8_t)2)
#define RC5_PACKET_BIT_COUNT                 ((uint8_t)13)      /*!< Field+toggle+address+command bits */
#define RC5_PACKET_STATUS_EMPTY              ((uint8_t)(1 << 0))

/* Private_Variables ---------------------------------------------------------*/

/* Logic table for rising edge: every line has values corresponding to previous bit.
   In columns are actual bit values for given bit time. */
const RC5_lastBit_t RC5_logicTableRisingEdge[2][2] =
  {
    {RC5_ZER, RC5_INV}, /* lastbit = ZERO */
    {RC5_NAN, RC5_ZER}, /* lastbit = ONE */
  };

/* Logic table for falling edge: every line has values corresponding to previous bit.
   In columns are actual bit values for given bit time. */
const RC5_lastBit_t RC5_logicTableFallingEdge[2][2] =
  {
    {RC5_NAN, RC5_ONE},  /* lastbit = ZERO */
    {RC5_ONE, RC5_INV},  /* lastbit = ONE */
  };

__IO StatusYesOrNo_t RC5FrameReceived = NO; /*!< RC5 Frame state */
__IO RC5_Packet_t   RC5TmpPacket;          /*!< First empty packet */

/* RC5  bits time definitions */
uint32_t  RC5MinT = 0;
uint32_t  RC5MaxT = 0;
uint32_t  RC5Min2T = 0;
uint32_t  RC5Max2T = 0;
static uint32_t TIMCLKValueKHz = 0; /*!< Timer clock */
uint32_t RC5TimeOut = 0;
__IO uint32_t RC5_Data = 0;
RC5_Frame_t RC5_FRAME;

/* Private_Function_Prototypes -----------------------------------------------*/
static uint8_t RC5_GetPulseLength (uint32_t pulseLength);
static void RC5_modifyLastBit(RC5_lastBit_t bit);
static void RC5_WriteBit(uint8_t bitVal);

/* Private_Functions ---------------------------------------------------------*/

/* Menu_RC5Decode_Func verwijderd: was bedoeld voor eval board met LCD.
 * Gebruik RC5_Init_Timing() + RC5_Decode() in je eigen main.c. */

/* RC5_DeInit verwijderd: timer deinit wordt beheerd door CubeMX/HAL. */

/**
  * @brief  Initialiseer de RC5 timing variabelen voor Nucleo-32 (32 MHz, prescaler 31).
  *         Aanroepen in main() na MX_TIM2_Init() en voor HAL_TIM_IC_Start_IT().
  *         Timer klok na prescaler: 32 MHz / (31+1) = 1 MHz -> 1 tick = 1 us.
  * @param  None
  * @retval None
  */
void RC5_Init_Timing(void)
{
  /* Timer clock na prescaler in kHz: 32MHz / (31+1) = 1MHz = 1000 kHz */
  TIMCLKValueKHz = 1000;
  RC5TimeOut = TIMCLKValueKHz * RC5_TIME_OUT_US / 1000; /* = 3600 ticks = 3.6 ms */

  /* Bit time range from assignment table (1 tick = 1 us bij 1 MHz timer klok). */
  RC5MinT  = RC5_MIN_1T_US * TIMCLKValueKHz / 1000;  /* 640  ticks */
  RC5MaxT  = RC5_MAX_1T_US * TIMCLKValueKHz / 1000;  /* 1140 ticks */
  RC5Min2T = RC5_MIN_2T_US * TIMCLKValueKHz / 1000;  /* 1340 ticks */
  RC5Max2T = RC5_MAX_2T_US * TIMCLKValueKHz / 1000;  /* 2220 ticks */

  /* Standaard begintoestand */
  RC5_ResetPacket();
}

/**
  * @brief  Decodeer het ontvangen RC5 frame in adres, commando en toggle bit.
  *         Aanroepen wanneer RC5FrameReceived == YES.
  *         Reset RC5FrameReceived automatisch na decoderen.
  * @param  pIRFrame: pointer naar RC5_Frame_t voor het resultaat
  */
void RC5_Decode(RC5_Frame_t *pIRFrame)
{
  /* Alleen decoderen als er een volledig frame ontvangen is */
  if (RC5FrameReceived != NO)
  {
    RC5_Data = RC5TmpPacket.data;
    /* RC5 frame velden decoderen */
    pIRFrame->FieldBit  = (RC5TmpPacket.data >> 12) & 0x1;
    pIRFrame->ToggleBit = (RC5TmpPacket.data >> 11) & 0x1;
    pIRFrame->Address   = (RC5TmpPacket.data >>  6) & 0x1F;
    pIRFrame->Command   = (uint8_t)((RC5TmpPacket.data) & (uint8_t)0x3F);

    /* Als field bit = 0: commando ligt in het bovenste bereik (64-127) */
    if (((RC5TmpPacket.data >> 12) & 0x1) != 0x01)
    {
      pIRFrame->Command = (1 << 6) | pIRFrame->Command;
    }
    /* Reset vlag en pakket voor volgend frame */
    RC5FrameReceived = NO;
    RC5_ResetPacket();

    /* Sla ook op in globale struct */
    RC5_FRAME = *pIRFrame;
  }
}

/**
  * @brief  Set the incoming packet structure to default state.
  * @param  None
  * @retval None
  */
void RC5_ResetPacket(void)
{
  RC5TmpPacket.data = 0;
  RC5TmpPacket.bitCount = RC5_PACKET_BIT_COUNT - 1;
  RC5TmpPacket.lastBit = RC5_ONE;
  RC5TmpPacket.status = RC5_PACKET_STATUS_EMPTY;
}

/**
  * @brief  Identify the RC5 data bits.
  * @param  rawPulseLength: low/high pulse duration
  * @param  edge: '1' for Rising  or '0' for falling edge
  * @retval None
  */
void RC5_DataSampling(uint32_t rawPulseLength, uint32_t edge)
{
  uint8_t pulse;
  RC5_lastBit_t tmp_bit;

  /* Decode the pulse length in protocol units */
  pulse = RC5_GetPulseLength(rawPulseLength);

  /* On Rising Edge */
  if (edge == 1)
  {
    if (pulse <= RC5_2T_TIME)
    {
      /* Bit determination by the rising edge */
      tmp_bit = RC5_logicTableRisingEdge[RC5TmpPacket.lastBit][pulse];
      RC5_modifyLastBit (tmp_bit);
    }
    else
    {
      RC5_ResetPacket();
    }
  }
  else     /* On Falling Edge */
  {
    /* If this is the first falling edge - don't compute anything */
    if (RC5TmpPacket.status & RC5_PACKET_STATUS_EMPTY)
    {
      RC5TmpPacket.status &= (uint8_t)~RC5_PACKET_STATUS_EMPTY;
    }
    else
    {
      if (pulse <= RC5_2T_TIME)
      {
        /* Bit determination by the falling edge */
        tmp_bit = RC5_logicTableFallingEdge[RC5TmpPacket.lastBit][pulse];
        RC5_modifyLastBit (tmp_bit);
      }
      else
      {
        RC5_ResetPacket();
      }
    }
  }
}

/**
  * @brief  Convert raw pulse length expressed in timer ticks to protocol bit times.
  * @param  pulseLength:pulse duration
  * @retval bit time value
  */
static uint8_t RC5_GetPulseLength (uint32_t pulseLength)
{
  /* Valid bit time */
  if ((pulseLength > RC5MinT) && (pulseLength < RC5MaxT))
  {
    /* We've found the length */
    return (RC5_1T_TIME); /* Return the correct value */
  }
  else if ((pulseLength > RC5Min2T) && (pulseLength < RC5Max2T))
  {
    /* We've found the length */
    return (RC5_2T_TIME);/* Return the correct value */
  }
  return RC5_WRONG_TIME;/* Error */
}

/**
  * @brief  perform checks if the last bit was not incorrect.
  * @param  bit: where bit can be  RC5_NAN or RC5_INV or RC5_ZER or RC5_ONE
  * @retval None
  */
static void RC5_modifyLastBit(RC5_lastBit_t bit)
{
  if (bit != RC5_NAN)
  {
    if (RC5TmpPacket.lastBit != RC5_INV)
    {
      /* Restore the last bit */
      RC5TmpPacket.lastBit = bit;

      /* Insert one bit into the RC5 Packet */
      RC5_WriteBit(RC5TmpPacket.lastBit);
    }
    else
    {
      RC5_ResetPacket();
    }
  }
}

/**
  * @brief  Insert one bit into the final data word.
  * @param  bitVal: bit value 'RC5_ONE' or 'RC5_ZER'
  * @retval None
  */
static void RC5_WriteBit(uint8_t bitVal)
{
  /* First convert RC5 symbols to ones and zeros */
  if (bitVal == RC5_ONE)
  {
    bitVal = 1;
  }
  else if (bitVal == RC5_ZER)
  {
    bitVal = 0;
  }
  else
  {
    RC5_ResetPacket();
    return;
  }

  /* Write this particular bit to data field */
  RC5TmpPacket.data |=  bitVal;

  /* Test the bit number determined */
  if (RC5TmpPacket.bitCount != 0)  /* If this is not the last bit */
  {
    /* Shift the data field */
    RC5TmpPacket.data = RC5TmpPacket.data << 1;
    /* And decrement the bitCount */
    RC5TmpPacket.bitCount--;
  }
  else
  {
    RC5FrameReceived = YES;
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
