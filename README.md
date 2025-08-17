

# Basys MX3 PIC32 Mini-Game

*A small arcade-style game for the Digilent **Basys MX3 (PIC32MX370F512L)** that combines an ADC “hint”, 4×4 keypad menu, LCD custom sprites, SW0 player movement, Timer5-driven RGB LED effects, buzzer feedback, and a multiplexed 4-digit seven-segment coin counter.*

> **Demo:** https://youtu.be/Rewyk_EM36Y?si=jtj9DZYtVE9dN5MM

---

## Features
- **ADC “hint” gate** — adjust analog input until `display_val = adc_val / 4` is **100–102** (~**1.32 V** @ 3.3 V Vref) → LCD shows **“Correct!”** and unlocks the menu.  
- **4×4 keypad menu** — row/column scan with pull-ups + software debounce → **Play / Store / Exit**.  
- **LCD gameplay (16×2)** — player moves **up/down** using **SW0 (RF3)**; **CGRAM** sprites for player, coin, and bomb.  
- **Feedback** — coin: `coins++` + **buzzer** beep (RB14) + **green LED** pulse (RD12). Bomb: **red LED** triple blink (RD2) + **“BOOM! Game Over”**.  
- **Seven-segment coin display (×4)** — **Timer5 ISR** multiplexes digits smoothly.  
- **Difficulty** — Easy/Hard changes frame delay.

---

## Hardware / BOM
- Digilent **Basys MX3** (PIC32MX370F512L, 3.3 V logic)  
- **16×2 HD44780** LCD (parallel: RS/RW/EN + 8-bit data)  
- **4×4 matrix keypad**  
- **Buzzer** (RB14)  
- **RGB LEDs** (using RD2 = Red, RD12 = Green)  
- **4-digit 7-segment**, **common-anode**

---

## Pinout (key signals)

| Subsystem | Pins |
|---|---|
| **LCD** | RS = **RB15**, RW = **RD5**, EN = **RD4**, Data bus = **PORTE (RE0–RE7)**, Busy reads **RE7** |
| **Keypad rows (out)** | **RC2, RC1, RC4, RG6** |
| **Keypad cols (in + pull-ups)** | **RC3, RG7, RG8, RG9** |
| **Switch** | **SW0 = RF3** (input) |
| **Buzzer** | **RB14** (output) |
| **RGB LEDs** | **RD2 = Red**, **RD12 = Green** (outputs) |
| **7-segment anodes** | **AN0 = RB12**, **AN1 = RB13**, **AN2 = RA9**, **AN3 = RA10** (active-LOW) |
| **7-segment segments (CA..CG)** | **CA = RG12**, **CB = RA14**, **CC = RD6**, **CD = RG13**, **CE = RG15**, **CF = RD7**, **CG = RD13** (active-LOW, common-anode) |

> Tip: disable analog on used digital pins (e.g., set `ANSELx = 0` where needed).

---

## Build & Flash
- **Toolchain:** MPLAB X IDE + XC32  
- **Device:** Basys MX3 default PIC32 (config bits are set in source)  
- Open the project, build, and program the board. Ensure a stable **3.3 V** supply and correct wiring.

---

## Calibration (ADC Hint)
- ADC is **10-bit** (0..1023), Vref = **AVdd = 3.3 V**.  
- The gate checks `display_val = adc_val / 4 ∈ [100..102]` → roughly **1.32 V** at the analog input.  
- Quick math: `V ≈ (ADC / 1023) * 3.3`. For `display_val = 102` → `ADC ∈ [408..411]` → **≈ 1.32 V**.  
- Hold the value steady for ~**2 s** (20 loops × 100 ms) to pass.

---

## Controls
- **Keypad:** menu navigation (Play / Store / Exit) and store selections.  
- **SW0 (RF3):** toggle player row (top/bottom) during play.  
- **Coin:** increments counter, beeps, flashes **green**.  
- **Bomb:** flashes **red** (triple) and ends the run.

---

## Architecture
- **State machine:** *Riddle → Menu → Difficulty → Game → Store/Exit*.  
- **LCD driver:** command/data writes, **busy-flag** polling on **RE7**, **CGRAM** sprites for player, coin, bomb.  
- **Keypad scan:** drive rows LOW one at a time; read columns with pull-ups; software **debounce** + release wait.  
- **Timer5 ISR:**  
  - **Seven-segment multiplexing** each tick (write segment lines, enable one anode).  
  - **LED effects** via `volatile` flags and tick counters (brief green pulse; red triple blink).  
- **Buzzer:** simple **bit-bang** beep (RB14) using microsecond delays.  
- **Data sharing:** globals marked **`volatile`** where read/written in ISR (e.g., `coin_display_value`, `*_blink_active`).

> **Timer math note:** With PBCLK ≈ **80 MHz**, prescaler **1:16**, and `PR5 = 1250`, ISR period ≈ **0.25 ms**.  
> For ~**2 ms** ticks, set `PR5 ≈ 10000`.

---

## Troubleshooting
- **LCD stuck** — verify `busy()` sets **RE7** as input and restores **TRISE**; check **RS/RW/EN** polarity.  
- **Keypad ghosts/missed keys** — enable column pull-ups, debounce, ensure only one row is driven LOW at a time.  
- **Dim/flickering 7-seg** — increase ISR frequency or adjust duty; segments are **active-LOW** on common-anode.  
- **LED effects too fast/slow** — revisit Timer5 tick period and counters.  
- **ADC threshold finicky** — add a small RC filter or slightly widen the acceptance window.

---

## What I Learned
- Configuring the **ADC**, scaling to engineering units, and gating gameplay via analog thresholds.  
- Writing **LCD drivers** with busy-flag handling and crafting **CGRAM** sprites.  
- Building a robust **keypad scanner** with debounce.  
- Designing **Timer ISRs** that safely multiplex displays and schedule LED effects via `volatile` flags.  
- Balancing **polling vs. interrupts** in an embedded **state-machine** game.
