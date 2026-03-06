/**
  ******************************************************************************
  * @file    menu.h
  * @author  MCD Application Team
  * @brief   This file contains all the functions prototypes for the menu navigation
  *          firmware driver.
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
#ifndef __MENU_H
#define __MENU_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/** @addtogroup IR_REMOTE
  * @{
  */

/** @addtogroup MENU
  * @{
  */

/** @defgroup MENU_Public_Types
  * @{
  */
typedef enum
{
  NOKEY = 0,
  SEL,
  RIGHT,
  LEFT,
  UP,
  DOWN,
  KEY
}MenuKeyStroke_t;

/**
  * @}
  */

/** @defgroup MENU_Exported_Functions
  * @{
  */
void Menu_Init(void);
void Menu_DisplayMenu(void);
void Menu_SelFunc(void);
void Menu_UpFunc(void);
void Menu_DownFunc(void);
void Menu_ReturnFunc(void);
MenuKeyStroke_t Menu_ReadKey(void);
void Menu_IdleFunc(void);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __MENU_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
