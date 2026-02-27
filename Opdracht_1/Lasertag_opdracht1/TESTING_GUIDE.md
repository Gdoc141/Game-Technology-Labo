# LaserTag Fase 1 - Testing & Troubleshooting Guide

## Quick Start Testing

### 1. Visuele Test (Zonder Oscilloscoop)
De eenvoudigste manier om te testen of de IR LED werkt:

**Materialen:**
- Smartphone camera (de meeste smartphones kunnen IR zien)
- Of: digitale camera

**Procedure:**
1. Flash de code naar de Nucleo board
2. Open de camera app op je smartphone
3. Richt de camera op de IR LED
4. Je zou een paarse/witte knippering moeten zien elke 2 seconden

⚠️ **Let op:** Sommige moderne smartphones hebben een IR-filter. Probeer beide camera's (voor en achter).

### 2. Test met TSOP4838 IR Ontvanger

**Aansluiting TSOP4838:**
```
TSOP4838 Pinout (van voorkant):
  ___
 |   |
 |   |  1. OUT (Signal) - Connect to PA1 or oscilloscoop probe
 |___|  2. GND         - Connect to GND
  |||   3. VS  (+3.3V) - Connect to +3.3V
  123
```

**Test Setup:**
```
Nucleo L432KC              TSOP4838
================          ===========
+3.3V ------------------- VS (Pin 3)
GND   ------------------- GND (Pin 2)
                          OUT (Pin 1) --- [Probe/LED]

PB6 (IR TX) ----[IR LED]----> [~50cm] ----[TSOP4838]
```

**Verwacht Resultaat:**
- TSOP4838 OUT is normaal HIGH (~3.3V)
- Bij IR ontvangst gaat OUT LOW
- Signal moet zichtbaar zijn op oscilloscoop of met LED indicator

### 3. Oscilloscoop Metingen

#### Meting 1: 38kHz Draaggolf (PB6)
- **Probe**: PB6 (TIM16_CH1)
- **Verwacht**: 38kHz blokgolf (26.3µs periode)
- **Amplitude**: 0V tot 3.3V
- **Duty cycle**: ~50%

#### Meting 2: Manchester Codering (PB6 gezoomed)
- **Probe**: PB6
- **Tijdbasis**: 2ms/div
- **Verwacht**: 38kHz bursts met Manchester patroon
- **Bit periode**: 1.778ms (889µs per half-bit)

#### Meting 3: Gedemoduleerd Signaal (TSOP4838 OUT)
- **Probe**: TSOP4838 OUT pin
- **Tijdbasis**: 2ms/div
- **Verwacht**: Manchester signaal (logische niveaus)
- **Let op**: TSOP4838 output is INVERTED (actief laag)

## Veelvoorkomende Problemen

### Probleem 1: Geen IR Signaal Detecteerbaar

**Symptomen:**
- Smartphone camera detecteert geen IR
- TSOP4838 reageert niet
- LED knippert niet op Nucleo

**Mogelijke Oorzaken & Oplossingen:**

1. **IR LED verkeerd aangesloten**
   - ✓ Controleer polariteit (lange poot = anode = +)
   - ✓ Meet spanning over LED (moet ~1.2V zijn bij aan)
   - ✓ Controleer weerstand waarde (47Ω tot 100Ω)

2. **Timer niet gestart**
   - ✓ Voeg debug output toe:
   ```c
   printf("Starting RC5 encoder...\r\n");
   RC5_Encode_Init();
   printf("Encoder initialized\r\n");
   ```

3. **GPIO niet correct geconfigureerd**
   - ✓ Verifieer in CubeMX: PB6 als TIM16_CH1 (Alternate Function AF14)
   - ✓ Check in code: GPIO mode = AF_PP

4. **Timer frequentie verkeerd**
   - ✓ Meet met oscilloscoop
   - ✓ Bereken: f = 80MHz / (ARR + 1) = 80MHz / 2105 = 38kHz

### Probleem 2: TSOP4838 Detecteert Constant Signaal

**Symptomen:**
- TSOP4838 OUT blijft laag
- Geen pulspatroon zichtbaar

**Mogelijke Oorzaken:**
1. **Te dichtbij / Te veel power**
   - → Vergroot afstand tot 50-100cm
   - → Verlaag IR LED stroom (grotere weerstand)

2. **Continue 38kHz zonder modulatie**
   - → Controleer of TIM_ForcedOC1Config werkt
   - → Test Manchester encoding functie

### Probleem 3: Verkeerd Manchester Patroon

**Symptomen:**
- Timing klopt niet
- Onregelmatig patroon

**Debug Stappen:**

1. **Test Timer 15 Interrupt:**
```c
// Add in HAL_TIM_PeriodElapsedCallback
static uint32_t counter = 0;
counter++;
if (counter % 100 == 0) {
  printf("TIM15 IRQ count: %lu\r\n", counter);
}
```

2. **Verifieer Frame Generatie:**
```c
// Add in RC5_Encode_SendFrame
printf("Frame: 0x%04X -> Manchester: 0x%08lX\r\n", 
       RC5BinaryFrameFormat, RC5ManchesterFrameFormat);
```

3. **Check Bit Counter:**
```c
// Add in RC5_Encode_SignalGenerate
printf("Bit %d: %d\r\n", BitsSentCounter, bit_msg);
```

### Probleem 4: Compilatie Errors

**Error: "undefined reference to `TimHandleLF`"**
- → Controleer of de variabelen gedeclareerd zijn in rc5_encode.c
- → Verificatie: grep "TIM_HandleTypeDef TimHandleLF" Core/Src/rc5_encode.c

**Error: "USE_NUCLEO_L432KC is not defined"**
- → Check ir_common.h regel ~24
- → Moet #define USE_NUCLEO_L432KC bevatten

**Error: "IR_TIM_LF_CLK is not defined"**
- → Controleer #ifdef USE_NUCLEO_L432KC sectie in ir_common.h
- → Moet alle IR_TIM defines bevatten

## Advanced Testing

### 1. Logic Analyzer Setup

Als je een logic analyzer hebt (bijv. Saleae Logic):

**Channels:**
- CH0: PB6 (IR TX raw)
- CH1: TSOP4838 OUT (demodulated)
- CH2: PB3 / LD3 (timing reference)

**Analyzer Settings:**
- Sample rate: 10 MS/s (minimaal voor 38kHz)
- Trigger: Rising edge op CH2

**Decode:**
- Gebruik Manchester decoder (if available)
- Bit period: 1.778ms
- Start condition: 2 start bits = "11"

### 2. Custom RC5 Decoder Test

Maak een simpele ontvanger test:

```c
/* In another Nucleo board or Arduino */
volatile uint32_t edge_times[50];
volatile uint8_t edge_count = 0;

void EXTI_Callback(void) {
  if (edge_count < 50) {
    edge_times[edge_count++] = HAL_GetTick();
  }
}

// Later analyze timing:
for (int i = 1; i < edge_count; i++) {
  uint32_t delta = edge_times[i] - edge_times[i-1];
  printf("Edge %d: %lu us\r\n", i, delta);
}
```

### 3. Power Consumption Test

Meet stroomverbruik tijdens transmissie:

**Setup:**
- Multimeter in serie met 3.3V rail
- Meet DC current tijdens verschillende modes

**Verwacht:**
- Idle: ~5-10mA (MCU only)
- TX Active (IR LED on): +50-100mA (afhankelijk van LED type)
- Piek current: Check rating van IR LED!

## Performance Verify Checklist

- [ ] 38kHz carrier frequency (± 1%)
- [ ] 1.778ms bit period (RC5 spec)
- [ ] 14-bit frame length
- [ ] Manchester encoding correct
- [ ] Toggle bit werkt
- [ ] Min 89ms tussen frames (RC5 spec)
- [ ] Frame compleet binnen 25ms

## Measurement Results Template

Vul dit sjabloon in voor je verslag:

```
Datum: __________
Test Setup: ________________________________

Timer Configuratie:
- TIM16 ARR: _______ → f = _______ kHz
- TIM15 ARR: _______ → T = _______ µs
- System Clock: _______ MHz

Metingen:
1. Carrier Frequency: _______ kHz (verwacht: 38kHz)
2. Bit Period: _______ ms (verwacht: 1.778ms)
3. Frame Duration: _______ ms (verwacht: ~25ms)
4. IR LED Forward Voltage: _______ V
5. IR LED Current: _______ mA

TSOP4838 Test:
- Maximale detectie afstand: _______ cm
- Minimale afstand (geen saturatie): _______ cm
- Signal kwaliteit: [ ] Goed [ ] Matig [ ] Slecht

Opmerkingen:
________________________________
________________________________
```

## Nuttige Terminal Commands

### Via STM32CubeIDE Console:
```
// Monitor RC5 transmissions
Expression: BitsSentCounter
Expression: RC5ManchesterFrameFormat

// Breakpoint in RC5_Encode_SendFrame
// Step through frame generation
```

### Via Serial Terminal (PuTTY/Tera Term):
```
115200 baud, 8N1, No flow control

// Send test commands (if UART control implemented)
S0C    // Send command 0x0C (12)
S10    // Send command 0x10 (16)
```

## Next Steps

Na succesvolle tests:
1. ✓ Documenteer je metingen
2. ✓ Maak foto's van setup
3. ✓ Bewaar oscilloscoop screenshots
4. → Ga verder met Fase 2 (IR Receiver)

---

**Veel succes met testen!**

Voor vragen of problemen, controleer eerst:
1. Deze troubleshooting guide
2. AN4834 application note
3. RC5 protocol documentatie
