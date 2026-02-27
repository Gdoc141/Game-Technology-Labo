# LaserTag Opdracht 1 - Fase 1: IR Transmitter

## Overzicht

Deze implementatie bouwt een IR-zender voor het LaserTag-systeem met behulp van het RC5-protocol aangepast voor 38kHz (i.p.v. de standaard 36kHz).

## Hardware Configuratie

### STM32L432KC Pinout
- **PB6 (TIM16_CH1)**: IR LED uitgang (38kHz PWM met Manchester codering)
- **PB3 (LD3)**: Status LED (knippert bij het verzenden)

### RC5 Protocol Specificaties
- **Draaggolf frequentie**: 38kHz (voor TSOP4838 IR ontvanger)
- **Bit periode**: 1.778ms (889µs per half-bit)
- **Codering**: Manchester (Biphase)
- **Frame structuur**: 14 bits (2 start bits + 1 toggle bit + 5 address bits + 6 command bits)

## Timer Configuratie

### TIM16 - High Frequency Timer (38kHz Draaggolf)
- **Clock**: 80MHz (systeem clock)
- **Prescaler**: 0
- **Period (ARR)**: 2104 → 80MHz / 2105 ≈ 38kHz
- **Pulse (CCR1)**: 1052 → 50% duty cycle
- **Mode**: PWM Mode 1
- **Functie**: Genereert de 38kHz IR draaggolf

### TIM15 - Low Frequency Timer (RC5 Bit Timing)
- **Clock**: 80MHz (systeem clock)
- **Prescaler**: 79 → Timer clock = 1MHz
- **Period (ARR)**: 889 → 889µs periode
- **Mode**: Output Compare Timing
- **Functie**: Genereert de RC5 bit timing (Manchester half-bit periodes)
- **Interrupt**: TIM1_BRK_TIM15_IRQn voor bit-wise signaal generatie

## Belangrijkste Bestanden

### Core/Src/rc5_encode.c
Bevat de RC5 encoder implementatie:
- `RC5_Encode_Init()`: Initialiseert de timers en GPIO
- `RC5_Encode_SendFrame(address, command, toggle)`: Verstuurt een RC5 frame
- `RC5_Encode_SignalGenerate()`: Timer interrupt handler voor Manchester codering
- `TIM_ForcedOC1Config()`: Schakelt de 38kHz draaggolf aan/uit

### Core/Inc/ir_common.h
Bevat hardware-specifieke configuratie voor STM32L432KC:
- Timer definities
- GPIO pinout
- Protocol timing parameters

### Core/Src/stm32l4xx_it.c
Bevat interrupt handlers:
- `TIM1_BRK_TIM15_IRQHandler()`: Timer 15 interrupt
- `HAL_TIM_PeriodElapsedCallback()`: Roept RC5_Encode_SignalGenerate() aan

## Hardware Setup

### IR LED Circuit (Zie AN4834 Figure 8)
```
                     +3.3V
                       |
                      [R1] 47Ω
                       |
    PB6 (TIM16_CH1) --+--|>|-- IR LED (e.g., TSAL6200)
                       |
                      [R2] 100Ω
                       |
                      GND
```

### Aanbevolen Componenten
- **IR LED**: TSAL6200 of gelijkwaardig (940nm, 100mA)
- **Weerstand R1**: 47Ω (stroombeperking voor LED)
- **Weerstand R2**: 100Ω (extra bescherming)
- **IR Ontvanger**: TSOP4838 (38kHz, voor testen)

## Gebruik

### Basis RC5 Frame Verzenden
```c
#include "rc5_encode.h"

// Initialiseer de encoder (gebeurt automatisch in main.c)
RC5_Encode_Init();

// Verstuur een RC5 commando
// RC5_Encode_SendFrame(address, command, toggle_bit)
RC5_Encode_SendFrame(0, 12, RC5_CTRL_RESET);  // TV Volume Up
```

### RC5 Address Codes (Voorbeelden)
- `0`: TV
- `5`: VCR
- `17`: Amplifier
- `20`: CD Player

### RC5 Command Codes (TV - Voorbeelden)
- `0`: Cijfer 0
- `1-9`: Cijfers 1-9
- `12`: Volume Up
- `13`: Volume Down
- `16`: Mute
- `32`: Channel Up
- `33`: Channel Down

### Toggle Bit
De toggle bit moet wisselen tussen opeenvolgende verzendingen van dezelfde toets:
```c
static RC5_Ctrl_t toggle = RC5_CTRL_RESET;

// Bij elke toetsdruk:
RC5_Encode_SendFrame(0, 12, toggle);
toggle = (toggle == RC5_CTRL_RESET) ? RC5_CTRL_SET : RC5_CTRL_RESET;
```

## Testen

### Test Setup
1. Sluit IR LED aan op PB6 volgens het schema
2. Plaats TSOP4838 IR ontvanger op ~50cm afstand
3. Sluit TSOP4838 uitgang aan op oscilloscoop of logic analyzer
4. Flash de code naar de Nucleo-L432KC

### Verwacht Signaal
- **Aan de LED**: 38kHz burst modulated met Manchester codering
- **Aan de TSOP4838**: Gedemoduleerd Manchester signaal (actief laag)
- **Frame duur**: ~25ms (14 bits × 1.778ms)
- **Frame herhaling**: Elke 2 seconden (in het voorbeeld)

### Debug Tips
1. **LED knippert niet**: Controleer TIM16 configuratie en GPIO setup
2. **Geen signaal op ontvanger**: 
   - Controleer IR LED polariteit
   - Vergroot de afstand als de ontvanger verzadigd raakt
   - Controleer of de LED in de richting van de ontvanger wijst
3. **Verkeerd signaal**:
   - Controleer timer frequenties met oscilloscoop
   - Verifieer Manchester codering in RC5_ManchesterConvert()

## Volgende Stappen (Fase 2)

In Fase 2 zul je:
1. Een RC5 decoder implementeren
2. TSOP4838 aansluiten op een timer input capture pin
3. Manchester decoding implementeren
4. Ontvangen frames verwerken

## Referenties

- **AN4834**: STMicroelectronics Application Note - "IR remote control transmitter based on STM32F0/F3 MCUs using STM32Cube"
- **RC5 Protocol**: Philips RC5 Infrared Remote Control Protocol
- **TSOP4838**: Vishay IR Receiver Modules for Remote Control Systems datasheet

## Bijlagen

### Berekeningen

#### 38kHz Draaggolf (TIM16)
```
f_carrier = 38kHz
T_carrier = 1 / 38kHz = 26.32µs

f_timer = 80MHz
PSC = 0 → f_timer_clock = 80MHz / (PSC + 1) = 80MHz
ARR = (f_timer_clock / f_carrier) - 1
    = (80MHz / 38kHz) - 1
    = 2104.74 ≈ 2104

Werkelijke frequentie = 80MHz / 2105 = 38.00475kHz ≈ 38kHz ✓
```

#### RC5 Bit Timing (TIM15)
```
T_half_bit = 889µs (RC5 specificatie)

PSC = 79 → f_timer_clock = 80MHz / 80 = 1MHz
ARR = T_half_bit × f_timer_clock
    = 889µs × 1MHz
    = 889

Werkelijke half-bit periode = 889 / 1MHz = 889µs ✓
RC5 bit periode = 2 × 889µs = 1.778ms ✓
```

---

**Auteur**: Implementatie voor GameTech Labo  
**Datum**: Februari 2026  
**Versie**: 1.0 - Fase 1 Complete
