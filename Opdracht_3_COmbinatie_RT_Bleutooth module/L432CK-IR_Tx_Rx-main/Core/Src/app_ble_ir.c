#include "app_ble_ir.h"

#include "ir_transceiver.h"
#include "main.h"
#include "rc5_decode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

#define BLE_RX_DMA_SIZE      512U
#define BLE_TX_TMP_SIZE      192U
#define CMD_LINE_SIZE        128U
#define BUTTON_DEBOUNCE_MS   300U
#define BLE_DEVICE_NAME      "LASERTAG_L432"
#define VCP_RX_QUEUE_SIZE    64U

static uint8_t ble_rx_dma[BLE_RX_DMA_SIZE];
static uint16_t ble_rx_last_pos = 0;

static char cmd_line[CMD_LINE_SIZE];
static uint16_t cmd_len = 0;

static char vcp_cmd_line[CMD_LINE_SIZE];
static uint16_t vcp_cmd_len = 0;
static uint32_t vcp_cmd_last_byte_tick = 0;

static volatile uint8_t tx_dma_busy = 0;
static uint8_t vcp_rx_byte = 0;
static volatile uint16_t vcp_rx_head = 0;
static volatile uint16_t vcp_rx_tail = 0;
static uint8_t vcp_rx_queue[VCP_RX_QUEUE_SIZE];
static uint8_t button_pressed = 0;
static uint32_t last_button_tick = 0;

static uint8_t toggle_bit = 0;
static uint8_t tx_address = 0;
static uint8_t tx_command = 0;

static uint32_t total_hits = 0;
static uint32_t hits_by_addr[32] = {0};

typedef enum
{
  CMD_SRC_VCP = 0,
  CMD_SRC_BLE = 1
} CmdSource;

static CmdSource active_cmd_source = CMD_SRC_VCP;

static void BleTx(const char *text);
static void DebugTx(const char *text);
static void ReplyTx(const char *text);
static void ProcessVcpInput(void);
static void HandleVcpByte(uint8_t byte);

static void BleSendAtRaw(const char *cmd)
{
  char out[96];
  int n = snprintf(out, sizeof(out), "%s\r\n", cmd);

  if ((n <= 0) || ((size_t)n >= sizeof(out)))
  {
    return;
  }

  BleTx(out);

  snprintf(out, sizeof(out), "[AT] %s\r\n", cmd);
  DebugTx(out);
}

static void BleRunStartupAtSequence(void)
{
  /* Initialize BLE on ESP32C3 */
  DebugTx("[BOOT] Initializing BLE...\r\n");
  
  /* Activate BLE */
  BleTx("AT+BLEINIT=1\r\n");
  HAL_Delay(500);
  
  /* Set device name */
  BleTx("AT+BLENAME=\"LASERTAG\"\r\n");
  HAL_Delay(200);
  
  DebugTx("[BOOT] BLE activated (AT+BLEINIT=1)\r\n");
  DebugTx("[BOOT] Device name set to 'LASERTAG'\r\n");
}

static void BleTx(const char *text)
{
  size_t len = strlen(text);
  uint32_t start = HAL_GetTick();

  if (len == 0U)
  {
    return;
  }

  while (tx_dma_busy)
  {
    if ((HAL_GetTick() - start) > 20U)
    {
      HAL_UART_Transmit(&huart1, (uint8_t *)text, (uint16_t)len, 80);
      return;
    }
  }

  tx_dma_busy = 1U;
  if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *)text, (uint16_t)len) != HAL_OK)
  {
    tx_dma_busy = 0U;
    HAL_UART_Transmit(&huart1, (uint8_t *)text, (uint16_t)len, 80);
  }
}

static void DebugTx(const char *text)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)text, (uint16_t)strlen(text), 80);
}

static void ReplyTx(const char *text)
{
  if (active_cmd_source == CMD_SRC_BLE)
  {
    BleTx(text);
  }
  else
  {
    DebugTx(text);
  }
}

static char *Trim(char *s)
{
  while ((*s == ' ') || (*s == '\t'))
  {
    s++;
  }

  if (*s == '\0')
  {
    return s;
  }

  char *end = s + strlen(s) - 1;
  while ((end > s) && ((*end == ' ') || (*end == '\t') || (*end == '\r') || (*end == '\n')))
  {
    *end = '\0';
    end--;
  }

  return s;
}

static int ParseValue(const char *input, uint32_t *out)
{
  char buf[24];
  const char *start = input;
  const char *end = NULL;
  char *parse_end = NULL;
  unsigned long val;

  while ((*start == ' ') || (*start == '\t'))
  {
    start++;
  }

  if (*start == '"')
  {
    start++;
    end = strchr(start, '"');
    if (end == NULL)
    {
      return 0;
    }
  }
  else
  {
    end = start;
    while ((*end != '\0') && (*end != ' ') && (*end != '\t'))
    {
      end++;
    }
  }

  size_t n = (size_t)(end - start);
  if ((n == 0U) || (n >= sizeof(buf)))
  {
    return 0;
  }

  memcpy(buf, start, n);
  buf[n] = '\0';

  val = strtoul(buf, &parse_end, 0);
  if ((parse_end == buf) || (*parse_end != '\0'))
  {
    return 0;
  }

  *out = (uint32_t)val;
  return 1;
}

static void SendCurrentSettings(void)
{
  char out[64];
  snprintf(out, sizeof(out), "current_settings: address=%u command=%u\r\n", tx_address, tx_command);
  ReplyTx(out);
}

static void SendCurrentHits(void)
{
  char out[BLE_TX_TMP_SIZE];
  size_t used = 0;

  int n = snprintf(out, sizeof(out), "current_hits: total=%lu", (unsigned long)total_hits);
  if (n < 0)
  {
    return;
  }
  used = (size_t)n;

  for (uint32_t i = 0; i < 32U; i++)
  {
    if (hits_by_addr[i] == 0U)
    {
      continue;
    }

    n = snprintf(&out[used], sizeof(out) - used, " A%lu=%lu", (unsigned long)i, (unsigned long)hits_by_addr[i]);
    if ((n < 0) || ((size_t)n >= (sizeof(out) - used)))
    {
      break;
    }
    used += (size_t)n;
  }

  if (used < (sizeof(out) - 3U))
  {
    out[used++] = '\r';
    out[used++] = '\n';
    out[used] = '\0';
  }
  else
  {
    out[sizeof(out) - 3U] = '\r';
    out[sizeof(out) - 2U] = '\n';
    out[sizeof(out) - 1U] = '\0';
  }

  ReplyTx(out);
}

static void ResetHits(void)
{
  memset(hits_by_addr, 0, sizeof(hits_by_addr));
  total_hits = 0;
  ReplyTx("reset_hits: ok\r\n");
}

static void SendAtCommand(const char *at_cmd)
{
  /* Send AT command to BLE module via USART1 and capture response from DMA buffer */
  char cmd_with_crlf[96];
  int n = snprintf(cmd_with_crlf, sizeof(cmd_with_crlf), "%s\r\n", at_cmd);
  
  if ((n <= 0) || ((size_t)n >= sizeof(cmd_with_crlf)))
  {
    ReplyTx("error: AT command too long\r\n");
    return;
  }

  /* Mark DMA position before sending */
  uint16_t dma_pos_before = (uint16_t)(BLE_RX_DMA_SIZE - __HAL_DMA_GET_COUNTER(huart1.hdmarx));
  
  /* Send command */
  BleTx(cmd_with_crlf);
  
  /* Wait for response (typically <100ms for AT commands) */
  HAL_Delay(200);
  
  /* Read DMA position after delay */
  uint16_t dma_pos_after = (uint16_t)(BLE_RX_DMA_SIZE - __HAL_DMA_GET_COUNTER(huart1.hdmarx));
  
  ReplyTx("[AT response] ");
  
  if (dma_pos_after > dma_pos_before)
  {
    /* Normal case: new data appended */
    for (uint16_t i = dma_pos_before; i < dma_pos_after; i++)
    {
      uint8_t c = ble_rx_dma[i];
      if ((c >= 32) && (c < 127))
      {
        char ch[2] = {(char)c, '\0'};
        ReplyTx(ch);
      }
      else if (c == '\n')
      {
        ReplyTx(" ");
      }
    }
  }
  else if (dma_pos_after < dma_pos_before)
  {
    /* Wrapped around buffer */
    for (uint16_t i = dma_pos_before; i < BLE_RX_DMA_SIZE; i++)
    {
      uint8_t c = ble_rx_dma[i];
      if ((c >= 32) && (c < 127))
      {
        char ch[2] = {(char)c, '\0'};
        ReplyTx(ch);
      }
      else if (c == '\n')
      {
        ReplyTx(" ");
      }
    }
    for (uint16_t i = 0; i < dma_pos_after; i++)
    {
      uint8_t c = ble_rx_dma[i];
      if ((c >= 32) && (c < 127))
      {
        char ch[2] = {(char)c, '\0'};
        ReplyTx(ch);
      }
      else if (c == '\n')
      {
        ReplyTx(" ");
      }
    }
  }
  else
  {
    ReplyTx("[no response]");
  }
  ReplyTx("\r\n");
}

static void HandleCommand(char *line)
{
  uint32_t value = 0;
  char *cmd = Trim(line);

  if (*cmd == '\0')
  {
    /* Empty line, ignore silently */
    return;
  }

  /* Ignore common BLE-module response lines so parser does not process them. */
  if ((strcmp(cmd, "OK") == 0) ||
      (strcmp(cmd, "ERROR") == 0) ||
      (strcmp(cmd, "ready") == 0) ||
      (strncmp(cmd, "+", 1) == 0) ||
      (strncmp(cmd, "AT", 2) == 0) ||
      (strncmp(cmd, "SDK version", 11) == 0) ||
      (strncmp(cmd, "compile time", 12) == 0) ||
      (strncmp(cmd, "Bin version", 11) == 0))
  {
    return;
  }

  if ((strcmp(cmd, "current_settings") == 0) ||
      (strcmp(cmd, "current settings") == 0))
  {
    SendCurrentSettings();
    return;
  }

  if (strncmp(cmd, "set_address:", 12) == 0)
  {
    if (!ParseValue(cmd + 12, &value) || (value > 31U))
    {
      ReplyTx("error: set_address expects 0..31\r\n");
      return;
    }

    tx_address = (uint8_t)value;
    ReplyTx("set_address: ok\r\n");
    return;
  }

  if (strncmp(cmd, "set_command:", 12) == 0)
  {
    if (!ParseValue(cmd + 12, &value) || (value > 127U))
    {
      ReplyTx("error: set_command expects 0..127\r\n");
      return;
    }

    tx_command = (uint8_t)value;
    ReplyTx("set_command: ok\r\n");
    return;
  }

  if (strcmp(cmd, "current_hits") == 0)
  {
    SendCurrentHits();
    return;
  }

  if (strcmp(cmd, "reset_hits") == 0)
  {
    ResetHits();
    return;
  }

  if (strncmp(cmd, "at:", 3) == 0)
  {
    const char *at_cmd = cmd + 3;
    if (*at_cmd == '\0')
    {
      ReplyTx("error: at: requires a command (e.g., at:AT or at:AT+NAME?)\r\n");
      return;
    }
    SendAtCommand(at_cmd);
    return;
  }

  ReplyTx("error: unknown command: [");
  ReplyTx(cmd);
  ReplyTx("]\r\n");
}

static void ProcessIncomingByte(uint8_t b)
{
  /* DEBUG: Echo to Serial Monitor */
  char debug_char[2] = {(char)b, '\0'};
  DebugTx(debug_char);
  
  /* Echo back to Android */
  BleTx(debug_char);
  
  if ((b == '\r') || (b == '\n'))
  {
    if (cmd_len > 0U)
    {
      cmd_line[cmd_len] = '\0';
      DebugTx("[BLE CMD] ");
      DebugTx(cmd_line);
      DebugTx("\r\n");
      active_cmd_source = CMD_SRC_BLE;
      HandleCommand(cmd_line);
      cmd_len = 0U;
    }
    return;
  }

  if (cmd_len < (CMD_LINE_SIZE - 1U))
  {
    cmd_line[cmd_len++] = (char)b;
  }
  else
  {
    cmd_len = 0U;
    BleTx("error: command too long\r\n");
  }
}

static void ProcessBleDmaRx(void)
{
  uint16_t pos;

  if (huart1.hdmarx == NULL)
  {
    return;
  }

  pos = (uint16_t)(BLE_RX_DMA_SIZE - __HAL_DMA_GET_COUNTER(huart1.hdmarx));
  if (pos == ble_rx_last_pos)
  {
    return;
  }

  if (pos > ble_rx_last_pos)
  {
    for (uint16_t i = ble_rx_last_pos; i < pos; i++)
    {
      ProcessIncomingByte(ble_rx_dma[i]);
    }
  }
  else
  {
    for (uint16_t i = ble_rx_last_pos; i < BLE_RX_DMA_SIZE; i++)
    {
      ProcessIncomingByte(ble_rx_dma[i]);
    }
    for (uint16_t i = 0; i < pos; i++)
    {
      ProcessIncomingByte(ble_rx_dma[i]);
    }
  }

  ble_rx_last_pos = pos;
}

void App_BleIr_Init(void)
{
  IR_Transceiver_Init();

  memset(ble_rx_dma, 0, sizeof(ble_rx_dma));
  ble_rx_last_pos = 0;
  cmd_len = 0;
  vcp_cmd_len = 0;
  vcp_rx_head = 0;
  vcp_rx_tail = 0;

  if (HAL_UART_Receive_DMA(&huart1, ble_rx_dma, BLE_RX_DMA_SIZE) == HAL_OK)
  {
    if (huart1.hdmarx != NULL)
    {
      __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
    }
  }

  /* Start interrupt-based VCP RX on USART2. */
  HAL_UART_Receive_IT(&huart2, &vcp_rx_byte, 1);

  DebugTx("\r\n[BOOT] IR TX/RX ready (VCP USART2)\r\n");
  DebugTx("[BOOT] BLE UART1 ready (DMA 512 bytes)\r\n");
  
  BleRunStartupAtSequence();
  
  HAL_Delay(1000);
  
  DebugTx("[BOOT] BLE initialization complete\r\n");
  DebugTx("[BOOT] Commands via VCP: current_settings | set_address:\"<0..31>\" | set_command:\"<0..127>\" | current_hits | reset_hits\r\n");
  DebugTx("[BOOT] Ready.\r\n");
}

void App_BleIr_OnExti(uint16_t gpioPin)
{
  if (gpioPin == GPIO_PIN_3)
  {
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) != GPIO_PIN_SET)
    {
      return;
    }

    uint32_t now = HAL_GetTick();
    if ((now - last_button_tick) < BUTTON_DEBOUNCE_MS)
    {
      return;
    }

    last_button_tick = now;
    button_pressed = 1U;
  }
}

void App_BleIr_Process(void)
{
  ProcessBleDmaRx();
  ProcessVcpInput();
  IR_Transceiver_Process();

  if (button_pressed && (IR_GetState() == IR_STATE_IDLE))
  {
    button_pressed = 0U;
    toggle_bit ^= 1U;

    if (IR_StartTransmit(toggle_bit, tx_address, tx_command) == 0)
    {
      char out[64];
      snprintf(out, sizeof(out), "[TX] Addr:%u Cmd:%u Tog:%u\r\n", tx_address, tx_command, toggle_bit);
      DebugTx(out);
    }
  }

  if (RC5FrameReceived && (IR_GetState() == IR_STATE_IDLE))
  {
    RC5_Decode(&RC5_FRAME);

    hits_by_addr[RC5_FRAME.Address & 0x1FU]++;
    total_hits++;

    char out[64];
    snprintf(out, sizeof(out), "[RX] Addr:%u Cmd:%u Tog:%u\r\n", RC5_FRAME.Address, RC5_FRAME.Command, RC5_FRAME.ToggleBit);
    DebugTx(out);

    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    tx_dma_busy = 0U;
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    tx_dma_busy = 0U;
  }
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart != &huart2)
  {
    return;
  }

  uint16_t next = (uint16_t)((vcp_rx_head + 1U) % VCP_RX_QUEUE_SIZE);
  if (next != vcp_rx_tail)
  {
    vcp_rx_queue[vcp_rx_head] = vcp_rx_byte;
    vcp_rx_head = next;
  }

  HAL_UART_Receive_IT(&huart2, &vcp_rx_byte, 1);
}

static void ProcessVcpInput(void)
{
  while (vcp_rx_tail != vcp_rx_head)
  {
    uint8_t byte = vcp_rx_queue[vcp_rx_tail];
    vcp_rx_tail = (uint16_t)((vcp_rx_tail + 1U) % VCP_RX_QUEUE_SIZE);
    HandleVcpByte(byte);
  }

  /* Timeout-based command trigger: if we have pending input and no bytes for 100ms, execute */
  if (vcp_cmd_len > 0U)
  {
    uint32_t now = HAL_GetTick();
    if ((now - vcp_cmd_last_byte_tick) > 100U)
    {
      vcp_cmd_line[vcp_cmd_len] = '\0';
      DebugTx("> ");
      DebugTx(vcp_cmd_line);
      DebugTx("\r\n");
      active_cmd_source = CMD_SRC_VCP;
      HandleCommand(vcp_cmd_line);
      vcp_cmd_len = 0U;
      vcp_cmd_last_byte_tick = now;
    }
  }
}

static void HandleVcpByte(uint8_t byte)
{
  /* Ignore high ASCII */
  if (byte > 127U)
  {
    return;
  }

  /* Track last byte time for timeout-based command trigger */
  vcp_cmd_last_byte_tick = HAL_GetTick();

  /* Accept CR/LF and ETX (0x03) as end-of-command. */
  if ((byte == '\r') || (byte == '\n') || (byte == 0x03U))
  {
    if (vcp_cmd_len > 0U)
    {
      vcp_cmd_line[vcp_cmd_len] = '\0';
      DebugTx("> ");
      DebugTx(vcp_cmd_line);
      DebugTx("\r\n");
      active_cmd_source = CMD_SRC_VCP;
      HandleCommand(vcp_cmd_line);
      vcp_cmd_len = 0U;
    }
    return;
  }

  /* Ignore control chars except space/tab */
  if ((byte < 32U) && (byte != '\t') && (byte != ' '))
  {
    return;
  }

  if (vcp_cmd_len < (CMD_LINE_SIZE - 1U))
  {
    vcp_cmd_line[vcp_cmd_len++] = (char)byte;
  }
  else
  {
    vcp_cmd_len = 0U;
    DebugTx("error: command too long\r\n");
  }
}
