/**
  @page TIM_Infrared  Description of the TIM_Infrared example
  
  @verbatim
  ******************************************************************************
  * @file    Templates/readme.txt 
  * @author  MCD Application Team
  * @brief   Description of the TIM_Infrared example.
  ******************************************************************************
  *
  * Copyright (c) 2018 STMicroelectronics. All rights reserved.
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                       opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  @endverbatim

@par Firmware Description

This projects provides a reference implementation of an infra-red transmitter and receiver using the 
IRTIM feature available on selected STM32 microcontrollers.

This directory provides a reference template project that can be used to build any firmware application for
STM32F072VB devices using STM32CubeF0 HAL and running on STM32072B-EVAL board from STMicroelectronics. 

@note Care must be taken when using HAL_Delay(), this function provides accurate delay (in milliseconds)
      based on variable incremented in SysTick ISR. This implies that if HAL_Delay() is called from
      a peripheral ISR process, then the SysTick interrupt must have higher priority (numerically lower)
      than the peripheral interrupt. Otherwise the caller ISR process will be blocked.
      To change the SysTick interrupt priority you have to use HAL_NVIC_SetPriority() function.
      
@note The application needs to ensure that the SysTick time base is always set to 1 millisecond
      to have correct HAL operation.

@par Directory contents 

  - TIM-Infrared/Src/main.c                           Main program
  - TIM-Infrared/Src/menu.c                           Menu navigation implementation
  - TIM-Infrared/Common_IR/Src/ir_common.c            Functions shared for both IR protocols.
  - TIM-Infrared/Common_IR/Src/rc5_decode.c           RC5 receiver implementation
  - TIM-Infrared/Common_IR/Src/sirc_decode.c          SIRC receiver implementation
  - TIM-Infrared/Common_IR/Src/rc5_encode.c           RC5 transmitter implementation
  - TIM-Infrared/Common_IR/Src/sirc_encode.c          SIRC transmitter implementation
  - TIM-Infrared/Src/system_stm32f0xx.c               STM32F0xx system clock configuration file
  - TIM-Infrared/Src/stm32f0xx_it.c                   Interrupt handlers 
  - TIM-Infrared/Src/stm32f0xx_hal_msp.c              HAL MSP module
  - TIM-Infrared/Inc/main.h                           Main program header file  
  - TIM-Infrared/Inc/menu.h                           Menu navigation header file  
  - TIM-Infrared/Inc/stm32f0xx_hal_conf.h             HAL Configuration file
  - TIM-Infrared/Inc/stm32f0xx_it.h                   Interrupt handlers header file
  - TIM-Infrared/Common_IR/Inc/ir_common.h            Shared definitions
  - TIM-Infrared/Common_IR/Inc/rc5_decode.h           RC5 receiver header file
  - TIM-Infrared/Common_IR/Inc/sirc_decode.h          SIRC receiver header file
  - TIM-Infrared/Common_IR/Inc/rc5_encode.h           RC5 transmitter header file
  - TIM-Infrared/Common_IR/Inc/sirc_encode.h          SIRC transmitter header file
        
@par Hardware and Software environment  

  - This example runs on STM32F072VB devices.
    
  - This example has been tested with STMicroelectronics STM32072B-EVAL boards
    and can be easily tailored to any other supported device and development board.


@par How to use it ? 

In order to make the program work, you must do the following :
 - Open your preferred toolchain 
 - Rebuild all files and load your image into target memory
 - Run the example
 - Use the joystick to navigate the menu and select the options.
 - Best way to demonstrate the functionality is by using two EVAL boards, communicating with each other.

 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */
