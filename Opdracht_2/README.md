# Opdracht 2 – LaserTag IR Receiver

**Vak:** Game Technology  
**Board:** STM32L432KC (Nucleo-32)  
**Protocol:** RC5 (Manchester-codering, 38 kHz draaggolf)  
**Sensor:** TSOP4838 IR-ontvanger

---

## Mapstructuur

```
Opdracht_2/
├── README.md                          ← dit bestand
├── Game tech - opdracht 2.pdf         ← opdrachtbeschrijving
└── Opdracht_2/
    └── Opdracht_2_LaserTag_Game/      ← STM32CubeIDE project
        ├── Inc/                       ← header bestanden (.h)
        ├── Src/                       ← bronbestanden (.c)
        ├── Startup/                   ← startup assembly
        ├── .project / .cproject       ← Eclipse/CubeIDE project files
        └── STM32L432KCUX_FLASH.ld     ← linker script
```

---

## Hardware aansluiting – TSOP4838 op STM32L432KC

### Pinout TSOP4838 (TO-92 behuizing, platte kant naar je toe)

```
   ┌──────────┐
   │ TSOP4838 │
   │          │
   └──┬──┬──┬─┘
      1  2  3
     OUT GND VS
```

| TSOP4838 Pin | Naam | Verbinding |
|:---:|------|-----------|
| 1 | OUT | → PA0 op Nucleo-32 (TIM2_CH1) |
| 2 | GND | → GND |
| 3 | VS   | → 100 Ω weerstand → 3.3V |

### Aanbevolen filtercircuit op VS (pin 3)

```
3.3V ──── 100Ω ──── VS (pin 3)
                  │
                100nF
                  │
                 GND
```

> **Waarom?** De 100Ω + 100nF vormen een RC-filter die ruis op de voeding onderdrukt.  
> Plaats de condensator zo dicht mogelijk bij de module.

### Belangrijke eigenschappen TSOP4838

| Parameter | Waarde |
|-----------|--------|
| Draaggolffrequentie | 38 kHz |
| Voedingsspanning | 2.5V – 5.5V (gebruik **3.3V** voor STM32) |
| Uitgang | Actief laag |
| Signaalpolariteit | **Omgekeerd**: idle = hoog, burst = laag |

---

## CubeIDE / CubeMX Configuratie

### Stap 1 – Systeemklok instellen

1. Open het `.ioc` bestand in CubeIDE (dubbelklik)
2. Ga naar **Clock Configuration** tab
3. Stel de HCLK in op **32 MHz** (via PLL, HSI als bron)
4. Zorg dat APB1 (waarop TIM2 hangt) ook op **32 MHz** loopt

> Dit is dezelfde kloksnelheid als opdracht 1. Nodig voor een nauwkeurige timermeting van 889 µs (RC5 halve bitperiode).

### Stap 2 – TIM2 configureren als PWM Input

1. Ga naar **Pinout & Configuration** tab
2. Links in de lijst: **Timers → TIM2**
3. Stel in:
   - **Clock Source:** `Internal Clock`
   - **Channel 1:** `Input Capture direct mode` → wordt automatisch PWM Input als je CH2 ook instelt
   - **Combined Channels:** `PWM Input on CH1`
4. Dit activeert automatisch **CH1** (neergaande flank) en **CH2** (stijgende flank)

### Stap 3 – TIM2 prescaler instellen

In de **Parameter Settings** van TIM2:

| Parameter | Waarde | Uitleg |
|-----------|--------|--------|
| Prescaler | `31` | 32 MHz / (31+1) = **1 MHz** → 1 tick = 1 µs |
| Counter Period | `65535` | Maximale telwaarde (16-bit) |
| Polarity (CH1) | `Falling Edge` | Meet bij neergaande flank |
| Polarity (CH2) | `Rising Edge` | Meet bij stijgende flank |

### Stap 4 – PA0 als TIM2_CH1

1. Ga naar **Pinout view** (de chip afbeelding)
2. Klik op **PA0**
3. Selecteer `TIM2_CH1`
4. De pin wordt groen gekleurd

### Stap 5 – Interrupts activeren

1. Ga naar **NVIC Settings** tab (binnen TIM2 configuratie)
2. Zet een vinkje bij:
   - `TIM2 global interrupt` → **Enabled**

### Stap 6 – Code genereren

1. Sla het `.ioc` bestand op (**Ctrl+S**)
2. CubeIDE vraagt om code te genereren → klik **Yes**
3. De gegenereerde code staat in `Src/main.c` en `Src/stm32l4xx_it.c`

---

## RC5 Protocol – Samenvatting

| Parameter | Waarde |
|-----------|--------|
| Frame lengte | 14 bits |
| Halve bitperiode (T) | 889 µs |
| Volledige bitperiode (2T) | 1778 µs |
| Draaggolf | 38 kHz, duty cycle 25–33% |
| Herhalingstijd | 113.778 ms |

### Framestructuur

```
 Bit:  13  12  11  10   9   8   7   6   5   4   3   2   1   0
      [ S ][ F ][ C ][    Adres (5 bits)   ][   Commando (6 bits)  ]
```

| Veld | Bits | Beschrijving |
|------|------|-------------|
| S | 1 | Start bit, altijd `1` |
| F | 1 | Field bit (uitbreiding commando) |
| C | 1 | Toggle bit (wisselt bij elke nieuwe toetsdruk) |
| Adres | 5 | Apparaatadres (0–31) |
| Commando | 6 | Commando (0–63) |

### Manchester-codering (na inversie TSOP4838)

| Bitwaarde | Overgang in midden bittijd | Lage puls |
|-----------|--------------------------|-----------|
| `1` | Laag → Hoog | 2e helft |
| `0` | Hoog → Laag | 1e helft |

> De TSOP4838 inverteert het signaal. De decoder houdt hier automatisch rekening mee.

---

## Timer Events in de ISR

| Event | Beschrijving |
|-------|-------------|
| CH1 (Falling Edge) | Meet totale periode tussen 2 neergaande flanken → T of 2T |
| CH2 (Rising Edge) | Meet duur lage puls → bepaalt bit 0 of 1 |
| Update (Overflow) | Na 3.7 ms geen flank → reset huidig frame |

---
