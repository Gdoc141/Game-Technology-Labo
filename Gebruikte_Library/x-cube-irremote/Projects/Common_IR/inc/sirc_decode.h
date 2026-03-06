/**
  ******************************************************************************
  * @file    sirc_decode.h
  * @author  MCD Application Team
  * @brief   This file contains all the functions prototypes for the SIRC
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
#ifndef __SIRC_DECODE_H
#define __SIRC_DECODE_H


/** @addtogroup IR_REMOTE
  * @brief Infra-red remote control
  * @{
  */

/** @addtogroup SIRC_DECODE
  * @brief SIRC decode driver modules
  * @{
  */

/** @defgroup SIRC_Exported_Types
  * @{
  */

/**
  * @brief  SIRC frame structure
  */
typedef struct
{
  __IO uint8_t Command;   /*!< Command field */
  __IO uint8_t Address;   /*!< Address field */
}
SIRC_Frame_t;

/**
  * @brief  SIRC packet structure
  */
typedef struct
{
  uint8_t count;  /*!< Bit count */
  uint8_t status; /*!< Status */
  uint32_t data;  /*!< Data */
}
SIRC_packet_t;

/**
  * @}
  */

/** @defgroup SIRC_Exported_Functions
  * @{
  */
void Menu_SIRCDecode_Func(void);
void SIRC_ResetPacket(void);
void SIRC_DataSampling(uint32_t lowPulseLength, uint32_t wholePulseLength);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif /* __SIRC_DECODE_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
