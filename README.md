# Arduino Tetris on LED Matrices

Tetris running on an Arduino Mega, drawn on eight MAX7219 8x8 LED matrices
arranged as a 16x32 display. The score is shown on a TM1637 4-digit display
and a buzzer plays the move and game-over sounds.

Author: Jhon Lee Quint

**Run it online:** [Wokwi simulation](https://wokwi.com/projects/452359878509846529)

## Hardware

| Part | Connection |
|---|---|
| Arduino Mega 2560 | — |
| 8x MAX7219 8x8 matrix (daisy-chained) | DIN 11, CLK 13, CS 10 |
| TM1637 4-digit display | CLK 21, DIO 20 |
| Buzzer | 12 |
| Push buttons (10 kΩ pull-down) | 4 right, 5 down, 6 left, 7 rotate |

The full wiring is in [`diagram.json`](diagram.json).

## Controls

In the Wokwi simulator the buttons are mapped to the keyboard:

| Key | Action |
|---|---|
| `A` | Move left |
| `D` | Move right |
| `S` | Move down |
| `W` | Rotate |

## How it works

- The playfield is 10x20 cells. Everything around it stays lit and acts as
  walls and floor, so collision checks are a single framebuffer bit test.
- The framebuffer holds one bit per LED (`fb[]`); `show()` writes it to the
  chained MAX7219s row by row, flipping the segments that are mounted upside
  down.
- Pieces fall every 500 ms; buttons are read every 100 ms.
- Full lines are removed and add one point. The next piece is previewed at the
  top of the display.

## Project layout

| File | Responsibility |
|---|---|
| `sketch.ino` | Game logic, display driver and main loop |
| `singing.ino` | Note table and buzzer sounds |
| `diagram.json` | Wokwi circuit |
| `libraries.txt` | Wokwi library list (Grove 4-Digit Display for the TM1637) |

## Building locally

Open the folder in the Arduino IDE, install **Grove 4-Digit Display** from the
Library Manager and select *Arduino Mega or Mega 2560*.

## Credits

Based on an open-source Arduino Tetris example, adapted and documented for this
circuit.
