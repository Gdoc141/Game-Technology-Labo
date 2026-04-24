#ifndef APP_BLE_IR_H
#define APP_BLE_IR_H

#include <stdint.h>

void App_BleIr_Init(void);
void App_BleIr_Process(void);
void App_BleIr_OnExti(uint16_t gpioPin);

#endif
