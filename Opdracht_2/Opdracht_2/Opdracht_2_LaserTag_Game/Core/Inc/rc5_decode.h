/**
  ******************************************************************************
  * @file    rc5_decode.h
  * @author  MCD Application Team
  * @brief   This file contains all the functions prototypes for the RC5
  *          firmware library.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RC5_DECODE_H
#define __RC5_DECODE_H

/** @addtogroup IR_REMOTE
  * @{
  */

/** @addtogroup RC5_DECODE
  * @brief RC5 driver modules
  * @{
  */

/** @defgroup RC5_Public_Types
  * @{
  */

/**
  * @brief  RC5 frame structure
  */
typedef struct
{
  __IO uint8_t FieldBit;   /*!< Field bit */
  __IO uint8_t ToggleBit;  /*!< Toggle bit field */
  __IO uint8_t Address;    /*!< Address field */
  __IO uint8_t Command;    /*!< Command field */
}
RC5_Frame_t;

/**
  * @brief  RC5 packet structure
  */
typedef struct
{
  __IO uint16_t data;     /*!< RC5 data */
  __IO uint8_t  status;   /*!< RC5 status */
  __IO uint8_t  lastBit;  /*!< RC5 last bit */
  __IO uint8_t  bitCount; /*!< RC5 bit count */
}
RC5_Packet_t;

/**
  * @brief  RC5 previous bit state type
  */
typedef enum
{
  RC5_ZER = 0,
  RC5_ONE,
  RC5_NAN,
  RC5_INV
} RC5_lastBit_t;

/**
  * @}
  */

/** @defgroup RC5_Exported_Functions
  * @{
  */

/* Oorspronkelijke publieke functies */
void RC5_ResetPacket(void);
void RC5_DataSampling(uint32_t rawPulseLength, uint32_t edge);

/* Toegevoegde functies voor Nucleo-32 integratie */
void RC5_Init_Timing(void);
void RC5_Decode(RC5_Frame_t *pIRFrame);

/* Globale variabelen (extern) */
extern __IO StatusYesOrNo_t RC5FrameReceived; /*!< YES als een compleet RC5 frame ontvangen is */
extern RC5_Frame_t RC5_FRAME;                 /*!< Laatste gedecodeerde RC5 frame */
extern __IO RC5_Packet_t RC5TmpPacket;        /*!< Lopende packet-opbouw voor debug */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif /* __RC5_DECODE_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
