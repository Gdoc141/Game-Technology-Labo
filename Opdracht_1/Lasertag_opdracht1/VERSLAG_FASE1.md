# LaserTag Opdracht 1 – Fase 1: IR Transmitter
**Hogeschool VIVES Brugge** | Game Technology Labo | Maart 2026

---

## Inhoudsopgave

1. [Projectoverzicht](#1-projectoverzicht)
2. [Theoretische Achtergrond](#2-theoretische-achtergrond)
3. [Hardware en Berekeningen](#3-hardware-en-berekeningen)
4. [STM32CubeMX Configuratie](#4-stm32cubemx-configuratie)
5. [Software Werking](#5-software-werking)
6. [Timerberekeningen](#6-timerberekeningen)
7. [Testen](#7-testen)
8. [Referenties](#8-referenties)

---

## 1. Projectoverzicht

Een **IR zender** voor een LaserTag-systeem op basis van de **STM32L432KC Nucleo-32**. De zender stuurt IR-signalen via het **RC5-protocol**, gemoduleerd op een **38 kHz draaggolf**.

| Parameter | Waarde |
|-----------|--------|
| Board | STM32 Nucleo-L432KC |
| MCU | STM32L432KCU6 |
| Systeemklok | 32 MHz (MSI + PLL) |
| IDE | Keil MDK-ARM µVision |

---

## 2. Theoretische Achtergrond

### 2.1 RC5 Protocol

Het **RC5-protocol** is een IR-standaard van Philips met **14 bits per frame**:

| Veld | Bits | Beschrijving |
|------|------|--------------|
| S1 | 1 | Startbit, altijd '1' |
| S2 (Field) | 1 | '1' voor cmd 0–63, '0' voor cmd 64–127 |
| Toggle | 1 | Wisselt bij elke nieuwe toetsdruk |
| Address | 5 | Apparaatadres (0–31) |
| Command | 6 | Commando (0–63) |

Elke bit duurt **1,778 ms** → totale frameduur ≈ **24,9 ms**.

### 2.2 Manchester Codering

Manchester codering is een zelf-synchroniserende codering waarbij **elke bit een flankovergang bevat**. De RC5-standaard gebruikt:

| Bit | Patroon | Beschrijving |
|-----|---------|--------------|
| '1' | HIGH → LOW | Neergaande flank in het midden van de bitperiode |
| '0' | LOW → HIGH | Opgaande flank in het midden van de bitperiode |

Elke bit bestaat uit twee halve periodes van **889 µs**. Een 14-bit RC5-frame wordt zo omgezet naar een **28-bit Manchester-frame**.

![RC5 protocol en Manchester codering](images/rc5_manchester.png)

*Figuur 1: RC5 frame structuur en Manchester-codering*

### 2.3 38 kHz Draaggolf en Envelope

IR-ontvangers zoals de **TSOP4838** detecteren enkel IR-licht gemoduleerd op **38 kHz**. De draaggolf wordt aan- en uitgeschakeld door de Manchester-envelope:

- **Carrier aan** (burst) → IR-LED pulseert op 38 kHz
- **Carrier uit** → geen IR-signaal

Duty cycle typisch **25%** om de gemiddelde stroom in de IR-LED laag te houden.

---

## 3. Hardware en Berekeningen

### 3.1 Gebruikte Componenten

| Component | Waarde/Type | Beschrijving |
|-----------|-------------|--------------|
| MCU | STM32L432KC | Nucleo-32 board |
| IR LED | TSAL6200 | 940 nm IR LED |
| Transistor | BC547 (NPN) | hFE ≈ 100–800 |
| R_B (basis) | 1 kΩ | Stuurt basisstroom |
| R_C (collector) | 47 Ω | Beperkt LED-stroom |
| IR ontvanger | TSOP4838 | 38 kHz demodulator |
| Voeding | 3,3 V | Nucleo board |

### 3.2 Transistorschakeling

De STM32 GPIO kan veilig slechts een beperkte stroom leveren. Omdat de TSAL6200 meer stroom nodig heeft voor voldoende bereik, wordt een **NPN-transistor (BC547) als schakelaar** gebruikt.

![Transistorschema IR LED](images/transistor_schema.png)

*Figuur 2: Transistorschakeling voor de IR-LED aansturing*

#### Berekeningen

Gewenste LED-stroom: **I_C = 60 mA** (piek bij 25% duty cycle).

**Stap 1 – Collector weerstand R_C:**

$$R_C = \frac{V_{CC} - V_{LED} - V_{CE(sat)}}{I_C} = \frac{3{,}3 - 1{,}35 - 0{,}2}{60\ \text{mA}} = \frac{1{,}75}{0{,}060} \approx 29{,}2\ \Omega$$

Standaardwaarde gekozen: **47 Ω** (veiliger, transistor en LED worden minder belast)

Met R_C = 47 Ω:

$$I_C = \frac{3{,}3 - 1{,}35 - 0{,}2}{47} = \frac{1{,}75}{47} \approx 37\ \text{mA}$$

**Stap 2 – Basis weerstand R_B:**

Minimale basisstroom voor verzadiging:

$$I_{B(\min)} = \frac{I_C}{h_{FE(\min)}} = \frac{37\ \text{mA}}{100} = 0{,}37\ \text{mA}$$

Met overdrive factor 10:

$$I_{B(\text{gewenst})} = 10 \times 0{,}37 = 3{,}7\ \text{mA}$$

$$R_B = \frac{V_{GPIO} - V_{BE}}{I_B} = \frac{3{,}3 - 0{,}7}{3{,}7\ \text{mA}} = \frac{2{,}6}{0{,}0037} \approx 703\ \Omega$$

Standaardwaarde gekozen: **1 kΩ**

**Verificatie met R_B = 1 kΩ:**

$$I_B = \frac{3{,}3 - 0{,}7}{1000} = 2{,}6\ \text{mA}$$

$$h_{FE(\text{eff})} = \frac{I_C}{I_B} = \frac{37}{2{,}6} \approx 14{,}2 \ll h_{FE(\min)} = 100\ \Rightarrow \text{transistor in verzadiging ✓}$$

GPIO-belasting: 2,6 mA → veilig ✓

#### Samenvatting berekeningen

| Parameter | Berekend | Gekozen |
|-----------|----------|---------|
| R_C | 29,2 Ω | **47 Ω** |
| I_C | 37 mA | 37 mA |
| R_B | 703 Ω | **1 kΩ** |
| I_B | 2,6 mA | 2,6 mA |

---

## 4. STM32CubeMX Configuratie

![CubeMX configuratie overzicht](images/cubemx_config.png)

*Figuur 3: STM32CubeMX configuratie – TIM15, TIM16 en klokboom*

### Systeemklok – 32 MHz via PLL

| Parameter | Waarde |
|-----------|--------|
| Oscillator | MSI Range 6 (4 MHz) |
| PLLM | /1 |
| PLLN | ×16 |
| PLLR | /2 |
| **SYSCLK** | **32 MHz** |

### TIM16 – 38 kHz Carrier (PWM op PA6)

| Parameter | Waarde | Reden |
|-----------|--------|-------|
| Mode | PWM Generation CH1 | PWM uitgang op PA6 |
| Prescaler (PSC) | 0 | Timerklok = 32 MHz |
| Counter Period (ARR) | 842 | → 37,96 kHz ≈ 38 kHz |
| Pulse (CCR1) | 210 | 25% duty cycle |

### TIM15 – 889 µs Bit-timing (Manchester)

| Parameter | Waarde | Reden |
|-----------|--------|-------|
| Mode | Output Compare No Output | Enkel timing, geen pin |
| Prescaler (PSC) | 31 | Timerklok = 1 MHz |
| Counter Period (ARR) | 888 | 889 µs per interrupt |

### NVIC

- **TIM1_BRK_TIM15** interrupt inschakelen, Priority **0** (hoogste)

### Pin-overzicht

| Pin | Functie | Alternate Function |
|-----|---------|-------------------|
| PA6 | TIM16_CH1 – 38 kHz carrier | AF14 |
| PB3 | LD3 status LED | GPIO_Output |

---

## 5. Software Werking

### Dataflow

```
RC5_Encode_SendFrame(address, command, toggle)
        │
        ▼
RC5_BinFrameGeneration()   → 14-bit binair RC5-frame
        │
        ▼
RC5_ManchesterConvert()    → 28-bit Manchester-frame
        │
        ▼
TIM15 gestart → interrupt elke 889 µs
        │
RC5_Encode_SignalGenerate()  (in ISR)
        │
   bit = 1 → TIM_FORCED_ACTIVE   → 38 kHz carrier AAN
   bit = 0 → TIM_FORCED_INACTIVE → carrier UIT
```

### Carrier schakelen

TIM16 loopt **continu**. De carrier wordt aan/uit gezet door de `CCMR1`-registerbits van TIM16 rechtstreeks te schrijven – dit gaat sneller dan een HAL-functie aanroepen:

| Constante | Waarde | Effect |
|-----------|--------|--------|
| `TIM_FORCED_ACTIVE` | `0x0060` | PWM Mode 1 → carrier actief |
| `TIM_FORCED_INACTIVE` | `0x0040` | Forced Inactive → uitgang laag |

---

## 6. Timerberekeningen

### TIM16 – 38 kHz Carrier

$$f_{\text{carrier}} = \frac{f_{\text{PCLK2}}}{(PSC+1)(ARR+1)} = \frac{32{.}000{.}000}{(0+1)(842+1)} = \frac{32{.}000{.}000}{843} \approx 37{,}96\ \text{kHz}\ \checkmark$$

$$\text{Duty cycle} = \frac{CCR1}{ARR+1} = \frac{210}{843} \approx 24{,}9\% \approx 25\%$$

### TIM15 – 889 µs Bit-timing

$$f_{\text{TIM15}} = \frac{32\ \text{MHz}}{PSC+1} = \frac{32\ \text{MHz}}{32} = 1\ \text{MHz}\ \Rightarrow\ T_{\text{tick}} = 1\ \mu\text{s}$$

$$T_{\text{interrupt}} = (ARR+1) \times 1\ \mu\text{s} = 889\ \mu\text{s}\ \checkmark$$

Dit is exact de halve bitperiode van RC5:

$$\frac{T_{\text{RC5 bit}}}{2} = \frac{1{,}778\ \text{ms}}{2} = 889\ \mu\text{s}\ \checkmark$$

---

## 7. Testen

### Visuele test (smartphone camera)

Richt de achterste camera op de IR LED. Bij correct werkende code zie je elke seconde een **paarse flits**.

### Oscilloscoop

| Meetpunt | Tijdbasis | Verwacht |
|----------|-----------|----------|
| PA6 (carrier) | 20 µs/div | 38 kHz blokgolf, 25% duty cycle |
| PA6 (frame) | 2 ms/div | Manchester bursts van 889 µs |
| TSOP4838 OUT | 2 ms/div | Geïnverteerd Manchester-signaal |

---

## 8. Referenties

| Bron | Beschrijving |
|------|--------------|
| STMicroelectronics AN4834 | IR Remote Control Transmitter STM32 |
| Philips RC5 Protocol Specification | Originele RC5 definitie |
| TSAL6200 Datasheet | IR LED (940 nm) specificaties |
| TSOP4838 Datasheet | 38 kHz IR ontvanger |
| BC547 Datasheet | NPN transistor parameters |
| STM32L432KC Reference Manual (RM0394) | Timer registers (CCMR1, CCER) |

---

*Game Technology Labo – Hogeschool VIVES Brugge – Fase 1: IR Transmitter*