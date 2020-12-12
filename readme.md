# Microcontroller Game Console for Flappy Bird

This project used multiple periferal systsems of a STM32 microcontroller to recreate the infamous flappy bird game. The system shows the main content through an LCD display controlled through SPI communication with the microcontroller. There is also an additional 7 segment display to update the user’s score. Finally, game music (the Tetris Theme Song) is output to the user by passing a variable duty cycle PWM signal through a hardware low pass filter.

## Notes:

- This project was programmed using [System Workbench for STM32](https://www.st.com/en/development-tools/sw4stm32.html)
- This project was programmed using the STM32F091 microcontroler on a custom development board
