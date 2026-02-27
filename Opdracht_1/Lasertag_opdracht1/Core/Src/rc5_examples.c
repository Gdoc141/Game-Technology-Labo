/**
  ******************************************************************************
  * @file    rc5_examples.c
  * @brief   RC5 Encoder usage examples for LaserTag project
  * @note    Add these code snippets to main.c in the appropriate USER CODE sections
  ******************************************************************************
  */

/* Example 1: Basic RC5 Transmission */
/* Add to main.c after RC5_Encode_Init() in USER CODE BEGIN 2 */

/*
// Send a single RC5 command (TV Power)
HAL_Delay(500);
RC5_Encode_SendFrame(0, 12, RC5_CTRL_RESET);  // Address=0 (TV), Command=12 (Power)
*/


/* Example 2: Toggle Bit Implementation */
/* Add to main.c in USER CODE BEGIN WHILE */

/*
static uint32_t last_send = 0;
static RC5_Ctrl_t toggle = RC5_CTRL_RESET;

// Send command every 2 seconds with proper toggle bit
if (HAL_GetTick() - last_send >= 2000) {
  // Send Volume Up command
  RC5_Encode_SendFrame(0, 16, toggle);
  
  // Toggle the toggle bit for next transmission
  toggle = (toggle == RC5_CTRL_RESET) ? RC5_CTRL_SET : RC5_CTRL_RESET;
  
  last_send = HAL_GetTick();
  
  // Blink LED to indicate transmission
  HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
}
*/


/* Example 3: Button-Triggered Transmission */
/* Add button configuration in CubeMX: PA0 as GPIO_Input with pull-up */
/* Add to main.c in USER CODE BEGIN WHILE */

/*
static uint8_t button_pressed = 0;
static RC5_Ctrl_t toggle = RC5_CTRL_RESET;

// Read button state (active low with pull-up)
if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
  if (!button_pressed) {
    button_pressed = 1;
    
    // Send RC5 command on button press
    RC5_Encode_SendFrame(0, 12, toggle);  // TV Volume Up
    
    // Toggle for next press
    toggle = (toggle == RC5_CTRL_RESET) ? RC5_CTRL_SET : RC5_CTRL_RESET;
    
    // Visual feedback
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
  }
} else {
  button_pressed = 0;
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
}

HAL_Delay(50);  // Simple debounce
*/


/* Example 4: LaserTag Weapon Configuration */
/* Weapon ID system: Address=weapon_id (0-31), Command=shot_type (0-63) */

/*
// LaserTag weapon configuration
#define WEAPON_ID_PLAYER1      1
#define WEAPON_ID_PLAYER2      2
#define SHOT_TYPE_NORMAL       10
#define SHOT_TYPE_POWER        11
#define SHOT_TYPE_SPECIAL      12

static RC5_Ctrl_t shot_toggle = RC5_CTRL_RESET;

// Fire weapon function
void FireWeapon(uint8_t weapon_id, uint8_t shot_type) {
  RC5_Encode_SendFrame(weapon_id, shot_type, shot_toggle);
  shot_toggle = (shot_toggle == RC5_CTRL_RESET) ? RC5_CTRL_SET : RC5_CTRL_RESET;
  
  // Visual feedback
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
}

// In main loop:
// Fire normal shot from player 1's weapon
FireWeapon(WEAPON_ID_PLAYER1, SHOT_TYPE_NORMAL);
HAL_Delay(500);  // 500ms between shots
*/


/* Example 5: Command Sequence */
/* Send multiple RC5 commands in sequence */

/*
void SendCommandSequence(void) {
  static RC5_Ctrl_t toggle = RC5_CTRL_RESET;
  
  // Command 1: Volume Up
  RC5_Encode_SendFrame(0, 16, toggle);
  toggle = (toggle == RC5_CTRL_RESET) ? RC5_CTRL_SET : RC5_CTRL_RESET;
  HAL_Delay(200);  // Wait for transmission to complete + gap
  
  // Command 2: Volume Up again
  RC5_Encode_SendFrame(0, 16, toggle);
  toggle = (toggle == RC5_CTRL_RESET) ? RC5_CTRL_SET : RC5_CTRL_RESET;
  HAL_Delay(200);
  
  // Command 3: Channel Up
  RC5_Encode_SendFrame(0, 32, toggle);
  toggle = (toggle == RC5_CTRL_RESET) ? RC5_CTRL_SET : RC5_CTRL_RESET;
}

// In main loop:
SendCommandSequence();
HAL_Delay(5000);  // Wait 5 seconds before repeating
*/


/* Example 6: UART-Controlled Transmission */
/* Configure USART2 in CubeMX (already done: 115200 baud) */
/* Commands: "Sxx" where xx is command number in hex */

/*
#include <string.h>

char uart_buffer[10];
static RC5_Ctrl_t toggle = RC5_CTRL_RESET;

// In main loop:
if (HAL_UART_Receive(&huart2, (uint8_t*)uart_buffer, 3, 100) == HAL_OK) {
  if (uart_buffer[0] == 'S') {
    // Parse command: "S12" -> command 0x12 (18)
    uint8_t command = 0;
    sscanf(&uart_buffer[1], "%2hhx", &command);
    
    // Send RC5 frame
    RC5_Encode_SendFrame(0, command, toggle);
    toggle = (toggle == RC5_CTRL_RESET) ? RC5_CTRL_SET : RC5_CTRL_RESET;
    
    // Send confirmation
    char response[20];
    sprintf(response, "Sent: %d\r\n", command);
    HAL_UART_Transmit(&huart2, (uint8_t*)response, strlen(response), 100);
  }
}
*/


/* Example 7: Rapid Fire Mode (LaserTag) */
/* Fast burst of shots with proper timing */

/*
#define RAPID_FIRE_RATE_MS  150  // 150ms between shots (6.67 shots/sec)
#define BURST_SIZE          3    // 3-shot burst

void RapidFire(uint8_t weapon_id, uint8_t shot_count) {
  static RC5_Ctrl_t toggle = RC5_CTRL_RESET;
  
  for (uint8_t i = 0; i < shot_count; i++) {
    RC5_Encode_SendFrame(weapon_id, SHOT_TYPE_NORMAL, toggle);
    toggle = (toggle == RC5_CTRL_RESET) ? RC5_CTRL_SET : RC5_CTRL_RESET;
    
    // Visual feedback
    HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
    
    // Wait between shots
    if (i < shot_count - 1) {
      HAL_Delay(RAPID_FIRE_RATE_MS);
    }
  }
}

// In main loop (trigger with button):
if (button_pressed) {
  RapidFire(WEAPON_ID_PLAYER1, BURST_SIZE);
  HAL_Delay(1000);  // Cooldown period
}
*/


/* Example 8: Debug Output via UART */
/* Monitor RC5 transmissions via serial terminal */

/*
#include <stdio.h>

// Add this function for printf redirection
int _write(int file, char *ptr, int len) {
  HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
  return len;
}

// In your transmission code:
void SendRC5WithDebug(uint8_t address, uint8_t command, RC5_Ctrl_t toggle) {
  printf("Sending RC5: Addr=%d, Cmd=%d, Toggle=%d\r\n", 
         address, command, (toggle == RC5_CTRL_SET) ? 1 : 0);
  
  RC5_Encode_SendFrame(address, command, toggle);
  
  printf("Frame sent!\r\n");
}
*/


/**
  * RC5 Standard Addresses (for reference)
  * =======================================
  * 0:  TV1
  * 1:  TV2
  * 5:  VCR
  * 6:  VCR2
  * 17: Audio Amplifier
  * 18: Receiver (Satellite/Cable)
  * 20: CD/DVD Player
  * 
  * For LaserTag, you can use:
  * 0-15:  Player weapon IDs
  * 16-20: Base station IDs
  * 21-31: Special devices (health pack, etc.)
  */

/**
  * RC5 Common Commands (TV - for reference)
  * =========================================
  * 0-9:   Digit keys (0-9)
  * 12:    Power/Standby
  * 13:    Mute
  * 16:    Volume Up
  * 17:    Volume Down
  * 32:    Channel Up
  * 33:    Channel Down
  * 48:    Fast Forward
  * 50:    Rewind
  * 53:    Play
  * 54:    Stop
  * 
  * For LaserTag, define your own:
  * 0-9:   Reserved
  * 10-19: Weapon shot types
  * 20-29: Game control commands
  * 30-39: Special actions
  */

/************************ END OF FILE ************************/
