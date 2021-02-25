# MiM Arduino Nano Every

MiM firmware for Arduino Nano Every board.

## Building

1. Install support for 'Arduino megaAVR boards' to the Arduino IDE using Boards Manager, see https://www.arduino.cc/en/Guide/NANOEvery#use-your-arduino-nano-every-on-the-arduino-desktop-ide
2. Open sketch in the IDE
3. Select Tools -> Board -> Arduino megaAVR boards -> Arduino Nano Every
4. Disable Registers emulation: Tools -> Registers Emulation -> None
5. Compile sketch (no additional libraries needed)

## Pinout

| Device  | Arduino Nano Every PIN (Expansion Board)| Arduino Nano Every PIN (Keyestudio Shield)
|---|---|---|
| BLDC_PWM (blue wire) | D9 | D5 |
| BLDC_FG (green wire) | D2 | D2 |
| BLDC_DIR (yellow wire) | D8 | D8 |
| XY_XPWM_PWM (1khz) | D3 | A6 |
| RX/TX | RX/TX | RX/TX |
| Stepper Driver EN | D10 | D8 |
| Stepper Driver STEP | D11 | D7 |
| Stepper Driver DIR | D12 | D4 |
| Limit Switch 1 | A0 | D10 |
| Limit Switch 2 | A1 | D11 |
| Red LED | D4 | A1 |
| Green LED | D5 | A2 |
| Blue LED | D6 | A3 |
