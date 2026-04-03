/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ir_transceiver.h"
#include "rc5_decode.h"
#include "rc5_encode.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim15;
TIM_HandleTypeDef htim16;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
static uint8_t toggle_bit = 0;
static uint8_t tx_address = 1;
static uint8_t tx_command = 0;
static volatile uint8_t button_pressed = 0;
static uint32_t last_button_tick = 0;
#define DEBOUNCE_MS 300  /* Covers press + release bounce */

typedef enum
{
  WEAPON_MODE_SINGLE = 0,
  WEAPON_MODE_SHOTGUN,
  WEAPON_MODE_BURST
} WeaponMode_t;

static char player_name[24] = "PLAYER1";
static WeaponMode_t weapon_mode = WEAPON_MODE_SINGLE;
static uint8_t burst_count = 3;
static uint32_t burst_gap_ms = 120U;
static uint32_t auto_fire_interval_ms = 3000U;
static uint8_t auto_fire_enabled = 0;

static volatile uint8_t uart_rx_byte = 0;
static volatile uint8_t uart_line_ready = 0;
static char uart_line[80];
static uint8_t uart_line_len = 0;

static uint8_t fire_request = 0;
static uint8_t firing_sequence_active = 0;
static uint8_t shots_remaining = 0;
static uint32_t next_fire_due_tick = 0;
static uint32_t next_cycle_tick = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM15_Init(void);
static void MX_TIM16_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static char *SkipSpaces(char *text)
{
  while ((*text == ' ') || (*text == '\t'))
  {
    text++;
  }

  return text;
}

static void TrimTrailingSpaces(char *text)
{
  size_t length = strlen(text);

  while ((length > 0U) &&
         ((text[length - 1U] == ' ') ||
          (text[length - 1U] == '\t') ||
          (text[length - 1U] == '\r') ||
          (text[length - 1U] == '\n')))
  {
    text[length - 1U] = '\0';
    length--;
  }
}

static uint8_t EqualsIgnoreCase(const char *left, const char *right)
{
  while ((*left != '\0') && (*right != '\0'))
  {
    if ((char)toupper((unsigned char)*left) != (char)toupper((unsigned char)*right))
    {
      return 0;
    }

    left++;
    right++;
  }

  return (*left == '\0') && (*right == '\0');
}

static const char *WeaponModeToString(WeaponMode_t mode)
{
  switch (mode)
  {
    case WEAPON_MODE_SHOTGUN:
      return "SHOTGUN";
    case WEAPON_MODE_BURST:
      return "BURST";
    default:
      return "SINGLE";
  }
}

static void SendStatusLine(void)
{
  char status[160];

  snprintf(status, sizeof(status),
           "[CFG] Name:%s Addr:%u Cmd:%u Mode:%s Burst:%u Gap:%lu ms Interval:%lu ms Auto:%s\r\n",
           player_name,
           tx_address,
           tx_command,
           WeaponModeToString(weapon_mode),
           burst_count,
           (unsigned long)burst_gap_ms,
           (unsigned long)auto_fire_interval_ms,
           auto_fire_enabled ? "ON" : "OFF");
  HAL_UART_Transmit(&huart1, (uint8_t *)status, strlen(status), 50);
}

static void SendHelpLine(void)
{
  const char *help =
      "[CMD] NAME <text> | PLAYER <0-31> | ADDR <0-31> | CMD <0-63> | BULLET <0-63>\r\n"
      "[CMD] MODE SINGLE|SHOTGUN|BURST | BURST <count> | GAP <ms> | INTERVAL <ms> | AUTO ON|OFF | FIRE\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t *)help, strlen(help), 50);
}

static void QueueFireSequence(uint8_t burstShots)
{
  firing_sequence_active = 1;
  shots_remaining = burstShots;
  fire_request = 1;
  next_fire_due_tick = 0;
}

static void StartManualFire(void)
{
  uint8_t burstShots = 1;

  if ((weapon_mode == WEAPON_MODE_SHOTGUN) || (weapon_mode == WEAPON_MODE_BURST))
  {
    burstShots = (burst_count == 0U) ? 1U : burst_count;
  }

  QueueFireSequence(burstShots);
}

static void TryTransmitPendingShot(void)
{
  uint32_t now = HAL_GetTick();
  uint8_t frame_toggle = toggle_bit;
  char txbuf[128];

  if (!fire_request)
  {
    return;
  }

  if (now < next_fire_due_tick)
  {
    return;
  }

  if (IR_GetState() != IR_STATE_IDLE)
  {
    return;
  }

  if (IR_StartTransmit(frame_toggle, tx_address, tx_command) != 0)
  {
    next_fire_due_tick = now + 5U;
    return;
  }

  toggle_bit ^= 1U;

  snprintf(txbuf, sizeof(txbuf),
           "[TX] Player:%s Addr:%u Cmd:%u Mode:%s Toggle:%u Shot:%u\r\n",
           player_name,
           tx_address,
           tx_command,
           WeaponModeToString(weapon_mode),
           frame_toggle,
           (unsigned int)((shots_remaining == 0U) ? 1U : shots_remaining));
  HAL_UART_Transmit(&huart1, (uint8_t *)txbuf, strlen(txbuf), 50);

  if (shots_remaining > 0U)
  {
    shots_remaining--;
  }

  if (shots_remaining > 0U)
  {
    next_fire_due_tick = now + burst_gap_ms;
  }
  else
  {
    fire_request = 0;
    firing_sequence_active = 0;
    next_cycle_tick = now + auto_fire_interval_ms;
  }
}

static void ProcessAutoFireSchedule(void)
{
  uint32_t now = HAL_GetTick();

  if (!fire_request)
  {
    if (auto_fire_enabled && (now >= next_cycle_tick))
    {
      StartManualFire();
    }
    else
    {
      return;
    }
  }

  TryTransmitPendingShot();
}

static uint8_t ParseU32(const char *text, uint32_t *value)
{
  char *end_ptr = NULL;
  unsigned long parsed = strtoul(text, &end_ptr, 0);

  if ((text == end_ptr) || (end_ptr == NULL))
  {
    return 0;
  }

  while ((*end_ptr == ' ') || (*end_ptr == '\t'))
  {
    end_ptr++;
  }

  if (*end_ptr != '\0')
  {
    return 0;
  }

  *value = (uint32_t)parsed;
  return 1;
}

static void PrintCommandAck(const char *label)
{
  char ack[128];

  snprintf(ack, sizeof(ack), "[CFG] %s\r\n", label);
  HAL_UART_Transmit(&huart1, (uint8_t *)ack, strlen(ack), 50);
}

static void ProcessCommandLine(char *line)
{
  char *command;
  char *argument;

  command = SkipSpaces(line);
  TrimTrailingSpaces(command);

  if (*command == '\0')
  {
    return;
  }

  argument = command;
  while ((*argument != '\0') && (*argument != ' ') && (*argument != '\t'))
  {
    argument++;
  }

  if (*argument != '\0')
  {
    *argument++ = '\0';
  }
  argument = SkipSpaces(argument);

  for (char *cursor = command; *cursor != '\0'; cursor++)
  {
    *cursor = (char)toupper((unsigned char)*cursor);
  }

  if (EqualsIgnoreCase(command, "HELP") || EqualsIgnoreCase(command, "?"))
  {
    SendHelpLine();
    return;
  }

  if (EqualsIgnoreCase(command, "STATUS"))
  {
    SendStatusLine();
    return;
  }

  if (EqualsIgnoreCase(command, "NAME"))
  {
    if (*argument == '\0')
    {
      PrintCommandAck("NAME requires a value");
      return;
    }

    strncpy(player_name, argument, sizeof(player_name) - 1U);
    player_name[sizeof(player_name) - 1U] = '\0';
    TrimTrailingSpaces(player_name);
    PrintCommandAck("Name updated");
    SendStatusLine();
    return;
  }

  if (EqualsIgnoreCase(command, "PLAYER") || EqualsIgnoreCase(command, "ADDR"))
  {
    uint32_t parsed = 0;

    if (!ParseU32(argument, &parsed) || (parsed > 31U))
    {
      PrintCommandAck("Address must be 0..31");
      return;
    }

    tx_address = (uint8_t)parsed;
    PrintCommandAck("Player address updated");
    SendStatusLine();
    return;
  }

  if (EqualsIgnoreCase(command, "CMD") || EqualsIgnoreCase(command, "BULLET"))
  {
    uint32_t parsed = 0;

    if (!ParseU32(argument, &parsed) || (parsed > 63U))
    {
      PrintCommandAck("Command must be 0..63");
      return;
    }

    tx_command = (uint8_t)parsed;
    PrintCommandAck("Bullet command updated");
    SendStatusLine();
    return;
  }

  if (EqualsIgnoreCase(command, "BURST"))
  {
    uint32_t parsed = 0;

    if (!ParseU32(argument, &parsed) || (parsed == 0U) || (parsed > 8U))
    {
      PrintCommandAck("Burst must be 1..8");
      return;
    }

    burst_count = (uint8_t)parsed;
    PrintCommandAck("Burst count updated");
    SendStatusLine();
    return;
  }

  if (EqualsIgnoreCase(command, "GAP"))
  {
    uint32_t parsed = 0;

    if (!ParseU32(argument, &parsed) || (parsed < 120U))
    {
      PrintCommandAck("Gap must be at least 120 ms");
      return;
    }

    burst_gap_ms = parsed;
    PrintCommandAck("Burst gap updated");
    SendStatusLine();
    return;
  }

  if (EqualsIgnoreCase(command, "INTERVAL"))
  {
    uint32_t parsed = 0;

    if (!ParseU32(argument, &parsed) || (parsed < 120U))
    {
      PrintCommandAck("Interval must be at least 120 ms");
      return;
    }

    auto_fire_interval_ms = parsed;
    PrintCommandAck("Auto-fire interval updated");
    SendStatusLine();
    return;
  }

  if (EqualsIgnoreCase(command, "AUTO"))
  {
    if (EqualsIgnoreCase(argument, "ON"))
    {
      auto_fire_enabled = 1;
      if (weapon_mode == WEAPON_MODE_SINGLE)
      {
        weapon_mode = WEAPON_MODE_BURST;
      }
      next_cycle_tick = HAL_GetTick() + auto_fire_interval_ms;
      PrintCommandAck("Auto-fire enabled");
      SendStatusLine();
      return;
    }

    if (EqualsIgnoreCase(argument, "OFF"))
    {
      auto_fire_enabled = 0;
      PrintCommandAck("Auto-fire disabled");
      SendStatusLine();
      return;
    }

    if (*argument != '\0')
    {
      uint32_t parsed = 0;

      if (!ParseU32(argument, &parsed) || (parsed < 120U))
      {
        PrintCommandAck("Auto-fire interval must be at least 120 ms");
        return;
      }

      auto_fire_enabled = 1;
      auto_fire_interval_ms = parsed;
      if (weapon_mode == WEAPON_MODE_SINGLE)
      {
        weapon_mode = WEAPON_MODE_BURST;
      }
      next_cycle_tick = HAL_GetTick() + auto_fire_interval_ms;
      PrintCommandAck("Auto-fire enabled");
      SendStatusLine();
      return;
    }

    PrintCommandAck("AUTO needs ON, OFF or a delay in ms");
    return;
  }

  if (EqualsIgnoreCase(command, "MODE"))
  {
    if (EqualsIgnoreCase(argument, "SINGLE"))
    {
      weapon_mode = WEAPON_MODE_SINGLE;
      auto_fire_enabled = 0;
      PrintCommandAck("Mode set to SINGLE");
      SendStatusLine();
      return;
    }

    if (EqualsIgnoreCase(argument, "SHOTGUN"))
    {
      weapon_mode = WEAPON_MODE_SHOTGUN;
      auto_fire_enabled = 0;
      PrintCommandAck("Mode set to SHOTGUN");
      SendStatusLine();
      return;
    }

    if (EqualsIgnoreCase(argument, "BURST"))
    {
      weapon_mode = WEAPON_MODE_BURST;
      auto_fire_enabled = 1;
      next_cycle_tick = HAL_GetTick() + auto_fire_interval_ms;
      PrintCommandAck("Mode set to BURST");
      SendStatusLine();
      return;
    }

    PrintCommandAck("MODE must be SINGLE, SHOTGUN or BURST");
    return;
  }

  if (EqualsIgnoreCase(command, "FIRE"))
  {
    if (!firing_sequence_active)
    {
      StartManualFire();
      PrintCommandAck("Fire request queued");
    }
    else
    {
      PrintCommandAck("Fire sequence already running");
    }
    return;
  }

  PrintCommandAck("Unknown command, send HELP");
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance != USART1)
  {
    return;
  }

  if ((uart_rx_byte == '\r') || (uart_rx_byte == '\n'))
  {
    if (uart_line_len > 0U)
    {
      uart_line[uart_line_len] = '\0';
      uart_line_ready = 1;
    }
    uart_line_len = 0;
  }
  else if (uart_line_len < (sizeof(uart_line) - 1U))
  {
    uart_line[uart_line_len++] = (char)uart_rx_byte;
  }
  else
  {
    uart_line_len = 0;
  }

  HAL_UART_Receive_IT(&huart1, (uint8_t *)&uart_rx_byte, 1);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_3)  /* PA3 — BTN_TX */
  {
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) != GPIO_PIN_SET) return;

    uint32_t now = HAL_GetTick();
    if ((now - last_button_tick) < DEBOUNCE_MS) return;
    last_button_tick = now;

    if (button_pressed) return;
    button_pressed = 1;
  }
}
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
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM15_Init();
  MX_TIM16_Init();
  /* USER CODE BEGIN 2 */
  IR_Transceiver_Init();
  HAL_UART_Receive_IT(&huart1, (uint8_t *)&uart_rx_byte, 1);

  /* Quick boot banner — proves UART works */
  {
    const char *banner = "\r\n[BOOT] IR TX/RX ready (115200-8N1) | use HELP for commands\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t *)banner, strlen(banner), 50);
  }
  SendStatusLine();
  SendHelpLine();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    /* --- State machine processing --- */
    IR_Transceiver_Process();
    ProcessAutoFireSchedule();

    if (uart_line_ready)
    {
      uart_line_ready = 0;
      ProcessCommandLine(uart_line);
    }

    /* --- TX trigger (button on PB3 temporarily, or serial command) --- */
    if (button_pressed && IR_GetState() == IR_STATE_IDLE)
    {
      StartManualFire();
      button_pressed = 0;
    }

    /* --- RX frame received --- */
    if (RC5FrameReceived && IR_GetState() == IR_STATE_IDLE)
    {
      RC5_Decode(&RC5_FRAME);

      char buf[64];
      snprintf(buf, sizeof(buf), "[RX] PlayerID:%u Cmd:%u Tog:%u\r\n",
           RC5_FRAME.Address, RC5_FRAME.Command, RC5_FRAME.ToggleBit);
      HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 50);

      /* Quick LED blink */
      HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
      HAL_Delay(100);
      HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_SlaveConfigTypeDef sSlaveConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 31;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 3700;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sSlaveConfig.SlaveMode = TIM_SLAVEMODE_RESET;
  sSlaveConfig.InputTrigger = TIM_TS_TI1FP1;
  sSlaveConfig.TriggerPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
  sSlaveConfig.TriggerPrescaler = TIM_ICPSC_DIV1;
  sSlaveConfig.TriggerFilter = 0;
  if (HAL_TIM_SlaveConfigSynchro(&htim2, &sSlaveConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_INDIRECTTI;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* Re-apply slave reset mode AFTER IC channel config, because
     HAL_TIM_IC_ConfigChannel on CH1 can clobber SMCR. */
  {
    TIM_SlaveConfigTypeDef sSlaveRe = {0};
    sSlaveRe.SlaveMode        = TIM_SLAVEMODE_RESET;
    sSlaveRe.InputTrigger     = TIM_TS_TI1FP1;
    sSlaveRe.TriggerPolarity  = TIM_TRIGGERPOLARITY_FALLING;
    sSlaveRe.TriggerPrescaler = TIM_ICPSC_DIV1;
    sSlaveRe.TriggerFilter    = 0;
    HAL_TIM_SlaveConfigSynchro(&htim2, &sSlaveRe);
  }

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM15 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM15_Init(void)
{

  /* USER CODE BEGIN TIM15_Init 0 */

  /* USER CODE END TIM15_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM15_Init 1 */

  /* USER CODE END TIM15_Init 1 */
  htim15.Instance = TIM15;
  htim15.Init.Prescaler = 31;
  htim15.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim15.Init.Period = 888;
  htim15.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim15.Init.RepetitionCounter = 0;
  htim15.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim15) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim15, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim15, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM15_Init 2 */

  /* USER CODE END TIM15_Init 2 */

}

/**
  * @brief TIM16 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM16_Init(void)
{

  /* USER CODE BEGIN TIM16_Init 0 */

  /* USER CODE END TIM16_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM16_Init 1 */

  /* USER CODE END TIM16_Init 1 */
  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 0;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 842;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 210;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim16, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim16, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM16_Init 2 */

  /* USER CODE END TIM16_Init 2 */
  HAL_TIM_MspPostInit(&htim16);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LD3_Pin */
  GPIO_InitStruct.Pin = LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD3_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI3_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
