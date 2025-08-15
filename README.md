# pic32-embedded-mini-game
A small arcade-style game for the Digilent Basys MX3 (PIC32MX370F512L) that combines an ADC “hint”, 4×4 keypad menu, LCD custom sprites, SW0 player movement, Timer5-driven RGB LED effects, buzzer feedback, and a multiplexed 4-digit seven-segment coin counter.
Basys MX3 PIC32 Mini-Game


Features

ADC “hint” gate: Adjust analog input until display_val = adc_val/4 falls in 100–102 (~1.32 V @ 3.3 V Vref) → LCD shows “Correct!” and unlocks menu.

Menu via 4×4 keypad: Row/column scan with pull-ups + debounce → Play / Store / Exit.

Gameplay on 16×2 LCD: Player moves up/down (two rows) using SW0 (RF3); CGRAM sprites for player poses, coin, and bomb.

Feedback:

Coin: coins++ + buzzer beep (RB14) + green LED (RD12) pulse.

Bomb: red LED (RD2) triple-blink + “BOOM! Game Over”.

Seven-segment coins display (x4): Timer5 ISR multiplexes digits smoothly.

Difficulty: Easy/Hard changes frame delay.

Hardware / BOM

Digilent Basys MX3 (PIC32MX370F512L, 3.3 V logic)

16×2 HD44780 LCD (parallel: RS/RW/EN + 8-bit data)

4×4 matrix keypad

Buzzer (RB14)

RGB LEDs (using RD2=Red, RD12=Green)

4-digit 7-segment, common-anode

Pinout (key signals)
Subsystem	Pins
LCD	RS=RB15, RW=RD5, EN=RD4, Data bus=PORTE (RE0–RE7), Busy reads RE7
Keypad rows (out)	RC2, RC1, RC4, RG6
Keypad cols (in + pull-ups)	RC3, RG7, RG8, RG9
Switch	SW0=RF3 (input)
Buzzer	RB14 (output)
RGB LEDs	RD2=Red, RD12=Green (outputs)
7-segment anodes	AN0=RB12, AN1=RB13, AN2=RA9, AN3=RA10 (active-LOW)
7-segment segments (CA..CG)	CA=RG12, CB=RA14, CC=RD6, CD=RG13, CE=RG15, CF=RD7, CG=RD13 (active-LOW for common-anode)

Don’t forget to disable analog on used digital pins (e.g., ANSELx=0 as in code).

Build & Flash

Toolchain: MPLAB X IDE + XC32

Device: Basys MX3 default PIC32 (project config bits set in source)

Open the project, build, and program the board. Ensure 3.3 V supply and correct wiring.

Calibration (ADC Hint)

ADC is 10-bit (0..1023), Vref = AVdd=3.3 V.

Game checks display_val = adc_val / 4 ∈ [100..102] → roughly 1.32 V at the analog input.

Quick math: V ≈ (ADC/1023)*3.3. For display_val=102 → ADC∈[408..411] → ≈1.32 V.

Hold the value steady ~2 s (20 loops × 100 ms) to pass.

Controls

Keypad: Menu navigation (Play / Store / Exit) and store selections.

SW0 (RF3): Toggle player row (top/bottom) during play.

Coin: Increments counter, beeps, flashes green.

Bomb: Flashes red (triple) and ends the run.

Architecture

State machine: Riddle → Menu → Difficulty → Game → Store/Exit.

LCD driver: Command/data writes, busy-flag polling on RE7, CGRAM sprites for player, coin, bomb.

Keypad scan: Drive rows LOW one at a time, read columns with pull-ups; software debounce + release wait.

Timer5 ISR:

Seven-segment multiplexing every tick (writes segment lines and enables one anode).

LED effects using volatile flags and tick counters (brief green pulse; red triple-blink).

Buzzer: Simple bit-bang beep (RB14) using microsecond delays.

Data sharing: Globals marked volatile where read/written in ISR (e.g., coin_display_value, *_blink_active).

Timer math note: With PBCLK≈80 MHz, prescaler 1:16, and PR5=1250, ISR period ≈ 0.25 ms.
If you want ~2 ms ticks, set PR5 ≈ 10000. Timing comments can be adjusted accordingly.

Troubleshooting

LCD stuck: Verify busy() sets RE7 as input and restores TRISE; check RS/RW/EN polarity.

Keypad ghosts/missed keys: Confirm pull-ups enabled on columns, add debounce, ensure only one row driven LOW at a time.

Dim/flickering 7-seg: Increase ISR frequency or duty balance; for common-anode remember segments are active-LOW.

LED effects too fast/slow: Revisit Timer5 tick math (see note) and counters inside ISR.

ADC threshold finicky: Noise on analog source—add small RC filter or widen acceptance window slightly.
