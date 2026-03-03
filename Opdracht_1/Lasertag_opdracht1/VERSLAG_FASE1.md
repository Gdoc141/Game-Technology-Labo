# LaserTag Opdracht 1 – Fase 1: IR Transmitter
## Uitgebreid Verslag – Game Technology Labo
**Hogeschool VIVES Brugge**  
**Datum:** Maart 2026  

---

## Inhoudsopgave

1. [Projectoverzicht](#1-projectoverzicht)
2. [Theoretische Achtergrond](#2-theoretische-achtergrond)
   - 2.1 [RC5 Protocol](#21-rc5-protocol)
   - 2.2 [Manchester Codering](#22-manchester-codering)
   - 2.3 [38 kHz Draaggolf (Carrier)](#23-38-khz-draaggolf-carrier)
   - 2.4 [Envelope (Modulatie-omhullende)](#24-envelope-modulatie-omhullende)
3. [Hardware](#3-hardware)
   - 3.1 [Gebruikte Componenten](#31-gebruikte-componenten)
   - 3.2 [Transistorschakelaar – Berekeningen](#32-transistorschakelaar--berekeningen)
   - 3.3 [Schemaoverzicht](#33-schemaoverzicht)
4. [STM32CubeMX Configuratie](#4-stm32cubemx-configuratie)
   - 4.1 [Nieuw Project Aanmaken](#41-nieuw-project-aanmaken)
   - 4.2 [Systeemklok Instellen](#42-systeemklok-instellen)
   - 4.3 [TIM16 – 38 kHz Draaggolf (PWM)](#43-tim16--38-khz-draaggolf-pwm)
   - 4.4 [TIM15 – Manchester Bit-Timing](#44-tim15--manchester-bit-timing)
   - 4.5 [GPIO Configuratie](#45-gpio-configuratie)
   - 4.6 [NVIC / Interrupt Instellingen](#46-nvic--interrupt-instellingen)
   - 4.7 [Code Genereren](#47-code-genereren)
5. [Softwarearchitectuur](#5-softwarearchitectuur)
   - 5.1 [Bestandsstructuur](#51-bestandsstructuur)
   - 5.2 [Dataflow](#52-dataflow)
6. [Gedetailleerde Code-uitleg](#6-gedetailleerde-code-uitleg)
   - 6.1 [ir_common.h – Hardware Definities](#61-ir_commonh--hardware-definities)
   - 6.2 [RC5_BinFrameGeneration – Frameopbouw](#62-rc5_binframegeneration--frameopbouw)
   - 6.3 [RC5_ManchesterConvert – Manchester Codering](#63-rc5_manchesterconvert--manchester-codering)
   - 6.4 [RC5_Encode_Init – Initialisatie](#64-rc5_encode_init--initialisatie)
   - 6.5 [RC5_Encode_SendFrame – Frame Verzenden](#65-rc5_encode_sendframe--frame-verzenden)
   - 6.6 [RC5_Encode_SignalGenerate – ISR Logica](#66-rc5_encode_signalgenerate--isr-logica)
   - 6.7 [TIM_ForcedOC1Config – Carrier Aan/Uit](#67-tim_forcedoc1config--carrier-aanuit)
   - 6.8 [main.c – Hoofdprogramma](#68-mainc--hoofdprogramma)
7. [Timerberekeningen](#7-timerberekeningen)
   - 7.1 [TIM16 – 38 kHz Carrier Frequentie](#71-tim16--38-khz-carrier-frequentie)
   - 7.2 [TIM15 – 889 µs Bit-Periode](#72-tim15--889-µs-bit-periode)
8. [Oscilloscoop Signalen](#8-oscilloscoop-signalen)
9. [Testen en Debuggen](#9-testen-en-debuggen)
10. [Veelvoorkomende Problemen](#10-veelvoorkomende-problemen)
11. [Referenties](#11-referenties)

---

## 1. Projectoverzicht

In dit labo-project wordt een **infrarood zender (IR transmitter)** gebouwd voor een LaserTag-systeem op basis van een **STM32L432KC Nucleo-32** microcontroller. De zender verstuurt IR-signalen via het **RC5-protocol**, gemoduleerd op een **38 kHz draaggolf**, waarna een IR-ontvanger (TSOP4838) het signaal kan oppikken en demoduleren.

### Doel

- Genereer een stabiele **38 kHz draaggolf** op een GPIO-pin via een timer in PWM-modus.
- Codeer een RC5 frame via **Manchester-codering** op een tweede timer.
- Schakel de draaggolf aan/uit (de "envelope") op basis van de Manchester-bittiming.
- Stuur het signaal naar een IR-LED die verbonden is via een **transistorschakelaar**.

### Gebruikte microcontroller

| Parameter | Waarde |
|-----------|--------|
| Board | STM32 Nucleo-L432KC |
| MCU | STM32L432KCU6 |
| Systeemklok | 32 MHz (MSI + PLL) |
| IDE/Toolchain | Keil MDK-ARM µVision |
| HAL versie | STM32L4xx HAL |

---

## 2. Theoretische Achtergrond

### 2.1 RC5 Protocol

Het **RC5-protocol** is een oudere maar wijdverspreide infrarood standaard, oorspronkelijk ontwikkeld door Philips. Het wordt gebruikt om commando's draadloos te versturen via IR (bijv. voor televisie-afstandsbedieningen). In ons LaserTag-systeem gebruiken we RC5 als communicatieprotocol.

#### Frame Structuur

Een RC5 frame bestaat uit **14 bits**:

```
Bit 13  Bit 12  Bit 11  Bits 10..6   Bits 5..0
------  ------  ------  ----------   ---------
  S1      S2    Toggle   Address      Command
  (1)     (1)    (0/1)   (5 bits)     (6 bits)
```

| Veld | Bits | Beschrijving |
|------|------|--------------|
| S1 | 1 | Startbit 1, altijd logisch '1' |
| S2 | 1 | Startbit 2 (field bit): '1' voor commando's 0–63, '0' voor commando's 64–127 |
| Toggle | 1 | Wisselt bij elke nieuwe toetsdruk, blijft gelijk bij ingehouden toets |
| Address | 5 | Apparaatadres (0–31), e.g. 0 = TV, 5 = VCR, 17 = Versterker |
| Command | 6 | Commando (0–63 of 0–127) |

**Totale frameduur:**  
`14 bits × 1,778 ms/bit = ~24,9 ms`

#### Voorbeeldwaarden frame

```
RC5_Encode_SendFrame(Address=0, Command=12, Toggle=0)
                                       ↓
Binary frame: S1=1, S2=1, T=0, Addr=00000, Cmd=001100
Bitpatroon (14 bit): 11 0 00000 001100  →  0x300C
```

---

### 2.2 Manchester Codering

Manchester codering (ook wel Biphase codering of Bi-phase Level) is een zelf-klokkenend lijncoderingsschema waarbij **elk bit een flank bevat**. Dit maakt synchronisatie door de ontvanger eenvoudig.

#### Coderingsregel (RC5 Manchester):

| Logische waarde | Manchester patroon | Beschrijving |
|-----------------|-------------------|--------------|
| Logisch **1** | `HIGH → LOW` (neergaande flank in het midden) | Eerste helft hoog, tweede helft laag |
| Logisch **0** | `LOW → HIGH` (opgaande flank in het midden) | Eerste helft laag, tweede helft hoog |

Elk bit duurt **1,778 ms**, opgesplitst in twee halve periodes van **889 µs**:

```
Bit '1':            ___
          t=0      |   |     t=T
                   |   |_____|
               <889µs><889µs>

Bit '0':   _____
          |     |
          |     |___
               <889µs><889µs>
```

#### Manchester Frame voorbeeld (vereenvoudigd):

```
RC5 Binary:   1     1     0     0     1     ...
              │     │     │     │     │
Manchester: ¯¯|_  ¯¯|_  _|¯¯  _|¯¯  ¯¯|_  ...
```

#### Hoe het in code werkt

In `rc5_encode.c` converteert de functie `RC5_ManchesterConvert()` elk binair bit naar 2 Manchester-bits. Als het bit '1' is, wordt het uitgebreid naar `HIGH–LOW` (= 0b10 = `RC5HIGHSTATE = 0x02`). Als het bit '0' is, dan `LOW–HIGH` (= 0b01 = `RC5LOWSTATE = 0x01`):

```c
#define RC5HIGHSTATE  ((uint8_t)0x02)   // Manchester '1' = HIGH dan LOW → 0b10
#define RC5LOWSTATE   ((uint8_t)0x01)   // Manchester '0' = LOW dan HIGH → 0b01
```

Het 14-bit RC5-frame wordt zo een **28-bit Manchester frame** dat bit-voor-bit door de timer-interrupt wordt uitgeklopt.

---

### 2.3 38 kHz Draaggolf (Carrier)

IR-ontvangers zoals de **TSOP4838** detecteren enkel IR-licht dat **gemoduleerd** is op een specifieke draaggolf. De TSOP4838 is geoptimaliseerd voor **38 kHz**. Zonder modulatie filtert de ontvanger het signaal weg (AGC bescherming).

De draaggolf is een rechthoekgolf van 38 kHz die continue door de IR-LED wordt gestuurd **wanneer de envelope actief is (logisch '1')**.

```
Carrier (38 kHz):
  ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐
  │ │ │ │ │ │ │ │ ...
──┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─
  ← 26,3 µs →
```

**Duty cycle**: Typisch 25–33% om de gemiddelde stroom in de IR-LED laag te houden en de maximale piekstroom niet te overschrijden (TSAL6200: max. 100 mA continu, maar tot 1 A piek bij lage duty cycle).

---

### 2.4 Envelope (Modulatie-omhullende)

De **envelope** is het langzame aan/uit-patroon van de IR-LED, bepaald door de Manchester-codering op 1,778 ms per bit. De draaggolf van 38 kHz wordt **ingeschakeld** als de envelope hoog is (burst) en **uitgeschakeld** als de envelope laag is (geen IR).

```
Envelope (Manchester voor bit '1'):
  ┌──────────────┐
  │  Burst aan   │  Burst uit
──┘              └──────────────
  ←   889 µs   →←   889 µs   →

Resultaat op IR-LED:
  ┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐  (geen signaal)
  ││││││││││││││
──┘└┘└┘└┘└┘└┘└┘└────────────────
  ← 38 kHz bursts →
```

De combinatie van carrier + envelope vormt het **uiteindelijke modulatiepatroon** dat door de TSOP4838 ontvangen en gedemoduleerd wordt.

---

## 3. Hardware

### 3.1 Gebruikte Componenten

| Component | Waarde/Type | Beschrijving |
|-----------|-------------|--------------|
| MCU | STM32L432KC | Nucleo-32 board |
| IR LED | TSAL6200 | 940 nm, max. 100 mA continu |
| Transistor | BC547 (NPN) | Small-signal NPN transistor, hFE ≈ 100–800 |
| R_basis | 1 kΩ | Stroombeperking stuurstroom basis |
| R_collector | 47 Ω | Stroombeperking IR LED |
| IR ontvanger | TSOP4838 | 38 kHz getuned demodulator |
| Voeding | 3,3 V | Vanuit Nucleo board |

---

### 3.2 Transistorschakelaar – Berekeningen

De STM32 GPIO-pin kan maximaal **25 mA** leveren. Voor een sterker IR-signaal (groter bereik) willen we de LED aansturen met **60–100 mA** piekstroom. Hiervoor gebruiken we een **NPN-transistor als schakelaar** (verzadigingsschakelaar).

#### Schema

```
         +3,3V
            |
           [R_C = 47 Ω]
            |
            +---------- IR LED (TSAL6200)
            |
           [C] Collector
           [B]----- [R_B = 1kΩ] ---- PA6 (GPIO → 3,3V)
           [E] Emitter
            |
           GND
```

#### Stap 1 – Gewenste collectorstroom bepalen

De IR LED (TSAL6200) heeft een aanbevolen stroom van **I_C = 60 mA** (piek, bij 25% duty cycle kan dit hoger).

Stuurspanning: `V_CC = 3,3 V`  
Doorlaatspanning LED bij 60 mA: `V_LED ≈ 1,4 V` (zie datasheet TSAL6200)  
Verzadigingsspanning transistor: `V_CE(sat) ≈ 0,2 V`

#### Stap 2 – Weerstand R_C berekenen

$$R_C = \frac{V_{CC} - V_{LED} - V_{CE(sat)}}{I_C}$$

$$R_C = \frac{3{,}3\text{ V} - 1{,}4\text{ V} - 0{,}2\text{ V}}{60\text{ mA}} = \frac{1{,}7\text{ V}}{0{,}060\text{ A}} \approx 28{,}3\text{ Ω}$$

→ Kies de **dichtstbijzijnde standaardwaarde**: **33 Ω** of **47 Ω** (47 Ω is veiliger = iets minder stroom maar veilig voor transistor en LED).

Met R_C = 47 Ω:

$$I_C = \frac{3{,}3 - 1{,}4 - 0{,}2}{47} = \frac{1{,}7}{47} \approx 36\text{ mA}$$

#### Stap 3 – Basisweerstand R_B berekenen

Voor volledige verzadiging van de transistor moet de basisstroom voldoende zijn. We werken met een **overdrive factor** van 10 (veiligheidsmarge):

$$I_{B(\text{min})} = \frac{I_C}{h_{FE(\text{min})}} = \frac{36\text{ mA}}{100} = 0{,}36\text{ mA}$$

Met overdrive factor 10:

$$I_{B(\text{gewenst})} = 10 \times I_{B(\text{min})} = 3{,}6\text{ mA}$$

Stuurspanning GPIO: `V_GPIO = 3,3 V`  
Basis-emitter drempelspanning: `V_BE ≈ 0,7 V`

$$R_B = \frac{V_{GPIO} - V_{BE}}{I_B} = \frac{3{,}3 - 0{,}7}{3{,}6\text{ mA}} = \frac{2{,}6}{0{,}0036} \approx 722\text{ Ω}$$

→ Kies **1 kΩ** (standaardwaarde, iets conservatiever maar de transistor verzadigt nog steeds).

Verificatie met R_B = 1 kΩ:

$$I_B = \frac{3{,}3 - 0{,}7}{1000} = \frac{2{,}6}{1000} = 2{,}6\text{ mA}$$

$$h_{FE(\text{effectief})} = \frac{I_C}{I_B} = \frac{36\text{ mA}}{2{,}6\text{ mA}} \approx 13{,}8$$

Omdat $h_{FE(\text{min})} = 100 \gg 13{,}8$, zit de transistor **zeker in verzadiging**. ✓

#### Stap 4 – GPIO stroom verificatie

De GPIO-pin van STM32L4 levert **I_B = 2,6 mA**, wat ruim onder de max. 25 mA valt. ✓

#### Samenvatting berekeningen

| Parameter | Berekend | Gekozen |
|-----------|----------|---------|
| R_C (collectorkring) | 28,3 Ω | **47 Ω** |
| Collectorstroom I_C | 36 mA | 36 mA |
| R_B (basiskring) | 722 Ω | **1 kΩ** |
| Basisstroom I_B | 2,6 mA | 2,6 mA |
| GPIO belasting | 2,6 mA | < 25 mA ✓ |

> **Opmerking:** Bij gebruik van de STM32 GPIO-pin **direct** (zonder transistor) voor een simpele test kan een 100 Ω weerstand in serie met de LED volstaan voor korte afstanden. Voor een correcte schakeling op langere afstand is de transistorschakeling noodzakelijk.

---

### 3.3 Schemaoverzicht

```
                      +3,3V
                         │
                        [47 Ω]  ← R_C (collectorkring)
                         │
                        ┌┴┐
                        │ │  TSAL6200 (IR LED)
                        └┬┘
                 Anode   │
                  ┌──────┤ Collector (C)
                  │     BC547
PA6 ──[1kΩ]──────┤ Basis (B)
(TIM16_CH1)      │
                  └──── Emitter (E) ──── GND


+3,3 V  ──────────────┬── TSOP4838 Pin 3 (VS)
                       │
GND     ──────────────┼── TSOP4838 Pin 2 (GND)
                       │
                      OUT (Pin 1) ─── Oscilloscoop / volgende fase
```

---

## 4. STM32CubeMX Configuratie

### 4.1 Nieuw Project Aanmaken

1. Open **STM32CubeMX**.
2. Klik op **"Access to Board Selector"**.
3. Zoek naar **"NUCLEO-L432KC"** en selecteer het board.
4. Klik op **"Start Project"**.
5. Bij de popup "Initialize all peripherals with their default mode?" klik op **"No"** (we configureren zelf).

---

### 4.2 Systeemklok Instellen

**Navigeer naar: `Clock Configuration` (tabblad bovenaan)**

De STM32L432KC gebruikt standaard de **MSI-oscillator** (Multi-Speed Internal). We configureren de PLL om 32 MHz te bereiken.

| Parameter | Instelling |
|-----------|-----------|
| Oscillator | MSI |
| MSI Range | Range 6 → 4 MHz |
| PLL Source | MSI |
| PLLM | /1 |
| PLLN | ×16 |
| PLLR | /2 |
| **Systeemklok (SYSCLK)** | **32 MHz** |
| AHB Prescaler | /1 → HCLK = 32 MHz |
| APB1 Prescaler | /1 → PCLK1 = 32 MHz |
| APB2 Prescaler | /1 → PCLK2 = 32 MHz |

> **Waarom 32 MHz?** De timers TIM15 en TIM16 hangen aan de APB2-bus (PCLK2). Met 32 MHz kunnen we nauwkeurige timerfrequenties instellen.

**Stappen in CubeMX Clock Configuration:**
1. Zet **MSI** aan, selecteer **Range 6 (4 MHz)**.
2. Activeer **PLL**: selecteer **MSI** als bron.
3. Stel in: PLLM=1, PLLN=16, PLLR=2.
4. Zet **SYSCLK Source** op **PLL**.
5. De klokbalk moet 32 MHz tonen.

---

### 4.3 TIM16 – 38 kHz Draaggolf (PWM)

**TIM16** genereert de continue **38 kHz PWM-golf** naar de IR-LED.

**Navigeer naar: `Pinout & Configuration → Timers → TIM16`**

| Parameter | Waarde | Uitleg |
|-----------|--------|--------|
| **Activated** | ✓ | TIM16 inschakelen |
| Channel 1 | **PWM Generation CH1** | PWM op kanaal 1 |
| Prescaler (PSC) | **0** | Geen deling → Timerklok = 32 MHz |
| Counter Period (ARR) | **842** | Perioderegister (zie berekening §7.1) |
| Pulse (CCR1) | **210** | ~25% duty cycle |
| Counter Mode | Up | Optellen |
| Auto-reload preload | Disable | |
| PWM Mode | **PWM Mode 1** | CH1 high zolang CNT < CCR1 |

**Pinout configuratie:**
- TIM16_CH1 wordt automatisch toegewezen aan **PA6** (Alternate Function 14).
- Controleer in de Pinout View dat PA6 groen gekleurd is als `TIM16_CH1`.

---

### 4.4 TIM15 – Manchester Bit-Timing

**TIM15** genereert interrupts elke **889 µs** voor de bit-timing van de Manchester-codering.

**Navigeer naar: `Pinout & Configuration → Timers → TIM15`**

| Parameter | Waarde | Uitleg |
|-----------|--------|--------|
| **Activated** | ✓ | TIM15 inschakelen |
| Channel 1 | **Output Compare No Output** | Timing mode (geen pin output) |
| Prescaler (PSC) | **31** | Timerklok = 32 MHz / 32 = 1 MHz |
| Counter Period (ARR) | **888** | 889 µs bij 1 MHz timerklok |
| Counter Mode | Up | Optellen |
| Auto-reload preload | Disable | |

> **Opmerking:** In de code gebruikt TIM15 ook het OC-kanaal in TIMING mode. Dit vereist HAL_TIM_PWM_Init in CubeMX maar wordt in de code omgezet naar timing mode via `TIM_OCMODE_TIMING`.

---

### 4.5 GPIO Configuratie

**Navigeer naar: `Pinout & Configuration → System Core → GPIO`**

De timerpins worden automatisch ingesteld door CubeMX bij het activeren van TIM15/TIM16. Controleer of de volgende pins correct staan:

| Pin | Functie | Mode | Speed |
|-----|---------|------|-------|
| **PA6** | TIM16_CH1 (38 kHz carrier) | AF_PP (AF14) | High |
| **PB3** | LD3 (Status LED) | GPIO_Output | Low |

De **status LED (PB3/LD3)** is al aanwezig op het Nucleo-32 board en wordt gebruikt als visuele indicator bij het verzenden.

---

### 4.6 NVIC / Interrupt Instellingen

**Navigeer naar: `Pinout & Configuration → System Core → NVIC`**

| Interrupt | Priority | Beschrijving |
|-----------|----------|--------------|
| **TIM1_BRK_TIM15_IRQn** | **0** (hoogste) | TIM15 periode-interrupt voor Manchester |
| SysTick | 15 | Lage prioriteit (HAL delay) |

> **Belangrijk:** TIM15 deelt zijn interrupt-vector met TIM1_BRK op de STM32L4. De ISR-naam is dus `TIM1_BRK_TIM15_IRQHandler()`.

Stappen:
1. Ga naar NVIC-tabblad.
2. Vink **"TIM1 break interrupt and TIM15 global interrupt"** aan.
3. Stel preemption priority in op **0**.

---

### 4.7 Code Genereren

1. Ga naar **`Project → Settings`**:
   - Project Name: `Lasertag_opdracht1`
   - Project Location: gewenste map
   - Toolchain/IDE: **MDK-ARM** (Keil µVision) of **STM32CubeIDE**
2. Klik op **`Generate Code`** (Ctrl+Shift+G).
3. Open het gegenereerde project in Keil MDK of STM32CubeIDE.
4. Voeg de bestanden `rc5_encode.c`, `rc5_encode.h` en `ir_common.h` toe aan het project.

---

## 5. Softwarearchitectuur

### 5.1 Bestandsstructuur

```
Core/
├── Inc/
│   ├── ir_common.h       ← Hardware definities & pinout voor Nucleo-L432KC
│   ├── rc5_encode.h      ← Publieke API van de RC5 encoder
│   ├── main.h            ← Gegenereerd door CubeMX
│   └── stm32l4xx_it.h    ← Interrupt handler declaraties
└── Src/
    ├── main.c            ← Hoofdprogramma (initialisatie + verzendlus)
    ├── rc5_encode.c      ← RC5 frame generatie + Manchester encoder
    ├── stm32l4xx_it.c    ← Timer interrupt handler
    └── rc5_examples.c    ← Gebruiksvoorbeelden (referentie)
```

### 5.2 Dataflow

```
Gebruiker roept RC5_Encode_SendFrame(address, command, toggle) aan
                        │
                        ▼
            RC5_BinFrameGeneration()
            → stelt 14-bit binary frame samen
                        │
                        ▼
            RC5_ManchesterConvert()
            → zet 14-bit om naar 28-bit Manchester
                        │
                        ▼
            TIM15 gestart met interrupt elke 889 µs
                        │
          Elke 889 µs: TIM1_BRK_TIM15_IRQHandler()
                        │
                        ▼
            RC5_Encode_SignalGenerate()
            → leest volgend bit uit 28-bit Manchester frame
                        │
               ┌────────┴────────┐
               │ bit = 1         │ bit = 0
               ▼                 ▼
     TIM_FORCED_ACTIVE   TIM_FORCED_INACTIVE
     (38 kHz carrier     (carrier UIT
      naar IR LED)        naar IR LED)
```

---

## 6. Gedetailleerde Code-uitleg

### 6.1 ir_common.h – Hardware Definities

Dit headerbestand centraliseert alle hardware-specifieke constanten voor de Nucleo-L432KC:

```c
#ifdef USE_NUCLEO_L432KC

/* Timer definities */
#define IR_TIM_LF          TIM15          // Lage freq. timer (Manchester timing)
#define IR_TIM_HF          TIM16          // Hoge freq. timer (38 kHz carrier)
#define TIM_PRESCALER      ((uint32_t)31) // 32 MHz / 32 = 1 MHz timerklok (LF)
#define IR_TIM_LF_IRQn     TIM1_BRK_TIM15_IRQn  // Interrupt vector naam

/* Timer perioden */
#define IR_ENC_HPERIOD_RC5  ((uint32_t)842)  // TIM16 ARR: 32MHz/(842+1) ≈ 38 kHz
#define IR_ENC_LPERIOD_RC5  ((uint32_t)888)  // TIM15 ARR: 1MHz/889 ≈ 889 µs

/* GPIO pinnen */
#define IR_GPIO_PORT_HF     GPIOA          // PA6: TIM16_CH1 (carrier)
#define IR_GPIO_PIN_HF      GPIO_PIN_6     // PA6
#define IR_GPIO_AF_HF       GPIO_AF14_TIM16  // Alternate Function 14

/* Manchester schakelconstanten */
#define TIM_FORCED_ACTIVE    ((uint16_t)0x0060)  // CCMR1-bits: PWM Mode 1 → 38kHz aan
#define TIM_FORCED_INACTIVE  ((uint16_t)0x0040)  // CCMR1-bits: Forced inactive → uit

#endif
```

**Waarom deze constanten?**
- `IR_ENC_HPERIOD_RC5 = 842`: Berekend zodat TIM16 bij 32 MHz exact 38 kHz genereert.
- `IR_ENC_LPERIOD_RC5 = 888`: TIM15 met PSC=31 geeft 1 MHz timerklok, 888+1=889 telstappen = 889 µs.
- `TIM_FORCED_ACTIVE/INACTIVE`: De CCMR1-bits van de STM32 timer bepalen de uitgangsmode. Door deze bits direct te schrijven kunnen we razendsnel wisselen tussen "PWM actief" (carrier aan) en "gedwongen laag" (carrier uit), zonder de hele timer te herstarten.

---

### 6.2 RC5_BinFrameGeneration – Frameopbouw

```c
static uint16_t RC5_BinFrameGeneration(uint8_t RC5_Address, 
                                        uint8_t RC5_Instruction, 
                                        RC5_Ctrl_t RC5_Ctrl)
{
    uint16_t star1 = 0x2000;  // Bit 13: Startbit S1 (altijd 1)
    uint16_t star2 = 0x1000;  // Bit 12: Startbit S2 / Field bit
    uint16_t addr  = 0;

    // Wacht tot vorige verzending klaar is
    while (RC5SendOpCompleteFlag == 0x00) {}

    // Controleer of commando > 63 → S2 = 0 (extended commands)
    if (RC5_Instruction >= 64) {
        star2 = 0;                      // Field bit = 0 voor uitgebreide commando's
        RC5_Instruction &= 0x003F;      // Alleen laagste 6 bits bewaren
    } else {
        star2 = 0x1000;                 // Field bit = 1 voor standaard commando's
    }

    // Adres schuiven naar positie bits 10..6
    addr = ((uint16_t)(RC5_Address)) << 6;

    // Frame samenvoegen: S1 | S2 | Toggle | Address | Command
    RC5BinaryFrameFormat = (star1) | (star2) | (RC5_Ctrl) | (addr) | (RC5_Instruction);

    return RC5BinaryFrameFormat;
}
```

**Voorbeeld:**
```
RC5_Encode_SendFrame(Address=0, Command=12, Toggle=RC5_CTRL_RESET)

star1  = 0x2000 = 0010 0000 0000 0000  ← bit 13 = 1
star2  = 0x1000 = 0001 0000 0000 0000  ← bit 12 = 1 (commando < 64)
toggle = 0x0000 = 0000 0000 0000 0000  ← bit 11 = 0 (RESET)
addr   = 0x0000 = 0000 0000 0000 0000  ← bits 10..6 = 00000 (adres 0)
cmd    = 0x000C = 0000 0000 0000 1100  ← bits 5..0 = 001100 (command 12)

Frame  = 0x300C = 0011 0000 0000 1100
```

---

### 6.3 RC5_ManchesterConvert – Manchester Codering

Dit is een van de **meest kritieke functies**. Ze zet het 14-bit binair RC5-frame om naar een 28-bit Manchester-gecodeerd frame:

```c
#define RC5HIGHSTATE  ((uint8_t)0x02)  // 0b10 → bit is '1': eerste helft HIGH, dan LOW
#define RC5LOWSTATE   ((uint8_t)0x01)  // 0b01 → bit is '0': eerste helft LOW, dan HIGH

static uint32_t RC5_ManchesterConvert(uint16_t RC5_BinaryFrameFormat)
{
    uint8_t  i            = 0;
    uint16_t Mask         = 1;
    uint16_t bit_format   = 0;
    uint32_t ConvertedMsg = 0;

    for (i = 0; i < RC5RealFrameLength; i++)    // RC5RealFrameLength = 14
    {
        // Isoleer het i-de bit van het RC5-frame
        bit_format = ((((uint16_t)(RC5_BinaryFrameFormat)) >> i) & Mask) << i;

        // Maak ruimte voor 2 nieuwe bits (schuif resultaat 2 posities op)
        ConvertedMsg = ConvertedMsg << 2;

        if (bit_format != 0)  // Logisch '1'
        {
            ConvertedMsg |= RC5HIGHSTATE;  // 0b10: eerste helft HIGH → carrier aan
        }
        else  // Logisch '0'
        {
            ConvertedMsg |= RC5LOWSTATE;   // 0b01: eerste helft LOW → carrier uit
        }
    }
    return ConvertedMsg;
}
```

**Visuele uitleg van de omzetting:**

```
RC5 binary (14 bits):   1    1    0    0    1    0  ...
                        │    │    │    │    │    │
Manchester (28 bits):  10   10   01   01   10   01  ...
                       │    │    │    │    │    │
                      HI  HI   LO   LO   HI   LO  (eerste halve bit periode)
                       LO  LO   HI   HI   LO   HI  (tweede halve bit periode)
```

**Resultaat:** Het 28-bit getal `ConvertedMsg` bevat het volledige Manchester-signaalpatroon. Elke twee bits in dit getal stellen één RC5-bit voor.

---

### 6.4 RC5_Encode_Init – Initialisatie

```c
void RC5_Encode_Init(void)
{
    TIM_OC_InitTypeDef   ch_config;
    GPIO_InitTypeDef     gpio_init_struct;

    /* ─── GPIO Configuratie ─── */
    
    // PA2: TIM15_CH1 (envelope - enkel voor hardware output als nodig)
    gpio_init_struct.Pin       = IR_GPIO_PIN_LF;        // PA2
    gpio_init_struct.Mode      = GPIO_MODE_AF_PP;        // Alternate Function, push-pull
    gpio_init_struct.Pull      = GPIO_NOPULL;
    gpio_init_struct.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_init_struct.Alternate = IR_GPIO_AF_LF;          // AF14 voor TIM15
    HAL_GPIO_Init(IR_GPIO_PORT_LF, &gpio_init_struct);

    // PA6: TIM16_CH1 (38 kHz carrier naar IR LED)
    gpio_init_struct.Pin       = IR_GPIO_PIN_HF;         // PA6
    gpio_init_struct.Alternate = IR_GPIO_AF_HF;          // AF14 voor TIM16
    HAL_GPIO_Init(IR_GPIO_PORT_HF, &gpio_init_struct);

    /* ─── TIM16: 38 kHz Carrier PWM ─── */
    ch_config.OCMode       = TIM_OCMODE_PWM1;            // PWM Mode 1
    ch_config.Pulse        = IR_ENC_HPERIOD_RC5 / 4;    // 25% duty cycle
    ch_config.OCPolarity   = TIM_OCPOLARITY_HIGH;
    ch_config.OCFastMode   = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&TimHandleHF, &ch_config, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&TimHandleHF, TIM_CHANNEL_1);      // Start 38 kHz

    /* ─── TIM15: Manchester Timing (Interrupt) ─── */
    ch_config.OCMode = TIM_OCMODE_TIMING;                // Alleen timing, geen pin output
    ch_config.Pulse  = IR_ENC_LPERIOD_RC5;               // 889 µs
    HAL_TIM_OC_ConfigChannel(&TimHandleLF, &ch_config, TIM_CHANNEL_1);

    /* ─── Interrupt activeren ─── */
    HAL_NVIC_SetPriority(IR_TIM_LF_IRQn, 0, 0);         // Hoogste prioriteit
    HAL_NVIC_EnableIRQ(IR_TIM_LF_IRQn);
    // TIM15 nog NIET starten - dat doet RC5_Encode_SendFrame()
}
```

**Belangrijk:** TIM16 start direct bij `RC5_Encode_Init()` en draait **continu**. Het scha­ke­len van de carrier (aan/uit) gebeurt niet via start/stop van TIM16, maar via de CCMR1-registerbits in `TIM_ForcedOC1Config()`.

---

### 6.5 RC5_Encode_SendFrame – Frame Verzenden

```c
void RC5_Encode_SendFrame(uint8_t RC5_Address,
                           uint8_t RC5_Instruction,
                           RC5_Ctrl_t RC5_Ctrl)
{
    // Stap 1: Genereer het 14-bit binair RC5-frame
    RC5BinaryFrameFormat = RC5_BinFrameGeneration(RC5_Address,
                                                   RC5_Instruction,
                                                   RC5_Ctrl);

    // Stap 2: Converteer naar 28-bit Manchester-formaat
    RC5ManchesterFrameFormat = RC5_ManchesterConvert(RC5BinaryFrameFormat);

    // Stap 3: Stel in dat het frame klaar is om verzonden te worden
    RC5SendOpReadyFlag = 1;

    // Stap 4: Reset de timer om zuivere timing te garanderen
    __HAL_TIM_SET_COUNTER(&TimHandleLF, 0);

    // Stap 5: Start TIM15 met interrupt → 889 µs ticks beginnen
    HAL_TIM_Base_Start_IT(&TimHandleLF);
}
```

**Gebruik in main.c:**
```c
// Eenmalige verzending (TV Volume Up, toggle = 0)
RC5_Encode_SendFrame(0, 12, RC5_CTRL_RESET);

// Met toggle bit wisselen (voor herhaald dezelfde toets):
static RC5_Ctrl_t toggle = RC5_CTRL_RESET;
RC5_Encode_SendFrame(0, 12, toggle);
toggle = (toggle == RC5_CTRL_RESET) ? RC5_CTRL_SET : RC5_CTRL_RESET;
```

---

### 6.6 RC5_Encode_SignalGenerate – ISR Logica

Deze functie wordt elke **889 µs** aangeroepen vanuit de timer-interrupt. Hij stuurt bit-voor-bit de Manchester-code uit door de carrier aan of uit te zetten:

```c
void RC5_Encode_SignalGenerate(void)
{
    uint32_t bit_msg = 0;

    // Zolang er bits te sturen zijn (28 bits × 2 flanken = GlobalFrameLength × 2)
    if ((RC5SendOpReadyFlag == 1) &&
        (BitsSentCounter <= (RC5GlobalFrameLength * 2)))
    {
        RC5SendOpCompleteFlag = 0x00;  // Bezig met verzenden

        // Haal het volgende bit op uit het Manchester-frame
        bit_msg = (uint8_t)((RC5ManchesterFrameFormat >> BitsSentCounter) & 1);

        if (bit_msg == 1)
        {
            // Carrier AAN → 38 kHz pulsen naar IR LED
            TIM_ForcedOC1Config(TIM_FORCED_ACTIVE);
        }
        else
        {
            // Carrier UIT → geen IR signaal
            TIM_ForcedOC1Config(TIM_FORCED_INACTIVE);
        }

        BitsSentCounter++;  // Volgende bit volgende interrupt
    }
    else
    {
        // Alle bits verzonden → opruimen
        RC5SendOpCompleteFlag = 0x01;
        HAL_TIM_Base_Stop_IT(&TimHandleLF);     // TIM15 stoppen
        RC5SendOpReadyFlag  = 0;
        BitsSentCounter     = 0;                // Teller resetten
        TIM_ForcedOC1Config(TIM_FORCED_INACTIVE); // Carrier definitief uit
        __HAL_TIM_DISABLE(&TimHandleLF);
    }
}
```

**Interrupt keten:**
```
TIM15 Period Elapsed (elke 889 µs)
    ↓
TIM1_BRK_TIM15_IRQHandler()   [stm32l4xx_it.c]
    ↓
HAL_TIM_PeriodElapsedCallback()
    ↓
if (htim == &htim15) →  RC5_Encode_SignalGenerate()
```

---

### 6.7 TIM_ForcedOC1Config – Carrier Aan/Uit

Dit is de **laagste niveau functie** die de draaggolf in- of uitschakelt. Ze schrijft rechtstreeks naar de CCMR1-tijdregisterbits van TIM16:

```c
void TIM_ForcedOC1Config(uint32_t action)
{
    TIM_TypeDef *TIMx = TimHandleHF.Instance;  // TIM16
    
    // 1. Tijdelijk Channel 1 uitschakelen
    TIMx->CCER &= ~TIM_CCER_CC1E;
    
    // 2. Output Compare Mode bits aanpassen (OC1M in CCMR1)
    MODIFY_REG(TIMx->CCMR1, TIM_CCMR1_OC1M, action);
    
    // 3. Channel 1 opnieuw inschakelen
    TIMx->CCER |= TIM_CCER_CC1E;
}
```

| `action` waarde | Hexwaarde | Effect | CCMR1 OC1M bits |
|-----------------|-----------|--------|-----------------|
| `TIM_FORCED_ACTIVE` | `0x0060` | PWM Mode 1 → 38 kHz carrier actief | `110` |
| `TIM_FORCED_INACTIVE` | `0x0040` | Forced Inactive → uitgang altijd laag | `100` |

**Waarom direct registers schrijven?** De HAL-functies zijn te traag en te complex voor deze kritische operatie die elke 889 µs moet plaatsvinden. Door rechtstreeks de CCMR1-bits te manipuleren is de schakelactie in slechts enkele klokticken voltooid.

---

### 6.8 main.c – Hoofdprogramma

```c
int main(void)
{
    HAL_Init();              // HAL + SysTick initialiseren
    SystemClock_Config();    // PLL → 32 MHz
    MX_GPIO_Init();          // LD3 als output
    MX_TIM15_Init();         // TIM15: 889 µs bittimer
    MX_TIM16_Init();         // TIM16: 38 kHz carrier
    
    RC5_Encode_Init();       // GPIO + Timer + NVIC voor IR
    HAL_Delay(500);          // Wachten voor stabiliteit

    while (1)
    {
        HAL_Delay(1000);     // 1 seconde wachten
        
        // Status LED knipperen (visuele feedback)
        HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
        
        // Stuur RC5 commando: Address=0 (TV), Command=12 (Volume Up)
        RC5_Encode_SendFrame(0, 12, RC5_CTRL_RESET);
    }
}
```

**SystemClock_Config() – PLL Instelling:**
```c
RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;  // MSI = 4 MHz
RCC_OscInitStruct.PLL.PLLState  = RCC_PLL_ON;
RCC_OscInitStruct.PLL.PLLM      = 1;
RCC_OscInitStruct.PLL.PLLN      = 16;   // VCO = 4 MHz × 16 = 64 MHz
RCC_OscInitStruct.PLL.PLLR      = RCC_PLLR_DIV2;  // SYSCLK = 64/2 = 32 MHz
```

---

## 7. Timerberekeningen

### 7.1 TIM16 – 38 kHz Carrier Frequentie

TIM16 is verbonden aan PCLK2 (APB2). Met APB2-prescaler = 1 geldt:

$$f_{\text{timerklok}} = \text{PCLK2} = 32\text{ MHz}$$

De timerfrequentie is:

$$f_{\text{timer}} = \frac{f_{\text{timerklok}}}{(\text{PSC} + 1) \times (\text{ARR} + 1)}$$

Met PSC = 0 en ARR = 842:

$$f_{\text{carrier}} = \frac{32{,}000{,}000}{(0+1) \times (842+1)} = \frac{32{,}000{,}000}{843} \approx 37{,}96\text{ kHz} \approx 38\text{ kHz} \checkmark$$

**Duty cycle berekening (CCR1 = 210):**

$$\text{Duty Cycle} = \frac{\text{CCR1}}{\text{ARR} + 1} = \frac{210}{843} \approx 24{,}9\% \approx 25\%$$

---

### 7.2 TIM15 – 889 µs Bit-Periode

TIM15 is ook op PCLK2 aangesloten. Met PSC = 31:

$$f_{\text{timerklok LF}} = \frac{32\text{ MHz}}{31 + 1} = \frac{32\text{ MHz}}{32} = 1\text{ MHz}$$

$$T_{\text{tick}} = \frac{1}{1\text{ MHz}} = 1\text{ µs per tick}$$

Met ARR = 888:

$$T_{\text{interrupt}} = (888 + 1) \times 1\text{ µs} = 889\text{ µs} \checkmark$$

Dit stemt overeen met de **halve bitperiode** van RC5:

$$T_{\text{RC5 half-bit}} = \frac{1{,}778\text{ ms}}{2} = 889\text{ µs} \checkmark$$

---

## 8. Oscilloscoop Signalen

### Verwachte signalen

#### Meting 1 – PA6 (38 kHz draaggolf, gezoomed in)

```
Tijdbasis: 20 µs/div

  3,3V ┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐
       │ │ │ │ │ │ │ │ │ │ │
  0V   └─┘ └─┘ └─┘ └─┘ └─┘ └─
       ←  26,3 µs  →
       (38 kHz, 25% duty cycle)
```

#### Meting 2 – PA6 (Manchester-gemoduleerde carrier, uitgezoomd)

```
Tijdbasis: 2 ms/div

  3,3V ┌──────────────┐           ┌──────────────────┐
       │ 38kHz bursts │  geen IR  │  38kHz bursts    │
  0V   ┘              └───────────┘                  └──
       ←   889 µs   →←   889 µs →← 1778 µs (2×889)  →
         (Logisch '1')              (Logisch '1')
```

#### Meting 3 – TSOP4838 OUT (gedemoduleerd Manchester signaal)

```
Tijdbasis: 2 ms/div
Let op: TSOP4838 is actief LAAG (geïnverteerd)

  3,3V ┘              ┌───────────┘                  ┌──
       │  signaal laag │  geen IR   │   signaal laag  │
  0V   └──────────────┘            └──────────────────┘
       (ontvanger actief bij burst)
```

### Totale frameduur meting

```
Tijdbasis: 5 ms/div

Volledige RC5 frame (~25 ms):
┌─────────────────────────────────────────────────────────┐ ~3,3V
│ S1  │ S2  │ Tog │ A4  │ A3  │ A2  │ A1  │ A0  │ ...   │
└─────────────────────────────────────────────────────────┘ 0V
← 1,778ms →← 1,778ms →← 1,778ms → ...
```

---

## 9. Testen en Debuggen

### Test 1 – Visuele IR Test (Smartphone Camera)

1. Flash de code naar de Nucleo.
2. Open camera-app (achterste camera).
3. Richt camera op IR LED.
4. Je zou een **paarse/witte knipperiging** moeten zien, elke 1 seconde.

> Sommige moderne smartphones hebben een IR-filter. Probeer dan de voorcamera.

### Test 2 – Oscilloscoop (zonder ontvanger)

| Meetpunt | Probe | Tijdbasis | Verwacht |
|----------|-------|-----------|----------|
| PA6 (carrier) | × 1 of × 10 | 20 µs/div | 38 kHz blokgolf, 25% duty |
| PA6 (frame) | × 1 | 2 ms/div | Manchester bursts |
| PB3 (LD3) | × 1 | 1 s/div | 1 Hz knipperiging |

### Test 3 – TSOP4838 Ontvanger

```
Aansluiting:
  TSOP4838 Pin 1 (OUT)  → oscilloscoop probe
  TSOP4838 Pin 2 (GND)  → GND Nucleo
  TSOP4838 Pin 3 (VS)   → 3,3V Nucleo
```

- Afstand IR LED → TSOP4838: **30–100 cm**
- Verwachte output: actief laag Manchester-signaal

### Debug code toevoegen

```c
/* In HAL_TIM_PeriodElapsedCallback - controleer of ISR werkt */
static uint32_t irq_cnt = 0;
if (htim->Instance == TIM15) {
    irq_cnt++;
    if (irq_cnt % 50 == 0) {
        HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);  // Snel knipperen = ISR actief
    }
}
```

```c
/* In RC5_Encode_SendFrame - print frame waarden via UART */
char buf[64];
sprintf(buf, "Binary: 0x%04X  Manchester: 0x%08lX\r\n",
        RC5BinaryFrameFormat, RC5ManchesterFrameFormat);
HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), 100);
```

---

## 10. Veelvoorkomende Problemen

| Probleem | Mogelijke oorzaak | Oplossing |
|----------|-------------------|-----------|
| Geen IR zichtbaar op camera | LED verkeerd aangesloten | Controleer polariteit (lange poot = anode = +) |
| TSOP4838 reageert niet | Verkeerde frequentie of te dichtbij | Meet carrier freq. met scope; vergroot afstand |
| LED brandt constant (geen modulatie) | `TIM_ForcedOC1Config` werkt niet | Controleer CCMR1-waarden in ir_common.h |
| Compilatiefout: undefined `TimHandleLF` | extern declaratie ontbreekt | Controleer `extern TIM_HandleTypeDef htim15;` in rc5_encode.c |
| Verkeerd Manchester-patroon | Timerfrequentie klopt niet | Herbereken PSC/ARR voor 32 MHz klok |
| ISR wordt niet aangeroepen | NVIC niet ingeschakeld | Controleer CubeMX NVIC-tab en `HAL_NVIC_EnableIRQ` |
| Compilatie stopt bij `USE_NUCLEO_L432KC` | Define ontbreekt | Voeg `#define USE_NUCLEO_L432KC` toe aan ir_common.h of project-defines |

---

## 11. Referenties

| Bron | Beschrijving |
|------|--------------|
| STMicroelectronics AN4834 | IR Remote Control Transmitter with STM32 |
| PHILIPS RC5 Protocol Specification | Originele RC5 definitie |
| TSAL6200 Datasheet | IR LED specificaties (940 nm) |
| TSOP4838 Datasheet | 38 kHz IR ontvanger module |
| BC547 Datasheet | NPN transistor – hFE, V_CE(sat), V_BE |
| STM32L432KC Reference Manual (RM0394) | Timer registers (CCMR1, CCER) |
| STM32L432KC Datasheet | GPIO, pin alternates (AF14 voor TIM15/TIM16) |
| STM32CubeMX User Manual (UM1718) | CubeMX gebruik en code generatie |

---

*Verslag opgesteld voor Game Technology Labo – Hogeschool VIVES Brugge*  
*Fase 1 – IR Transmitter met RC5 Protocol op STM32L432KC Nucleo-32*
