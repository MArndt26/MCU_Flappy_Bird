#include "stm32f0xx.h"
#include <stdint.h>
#include <stdlib.h>

#include "seven_seg.h"

const char nums[] = { 0x3f,  // 0
        0x06,  // 1
        0x5b,  // 2
        0x4f,  // 3
        0x66,  // 4
        0x6d,  // 5
        0x7d,  // 6
        0x07,  // 7
        0x7f,  // 8
        0x67,  // 9
        };

uint8_t offset;
uint8_t display[] = { letter_s, letter_c, letter_o, letter_r, letter_e, dash,
        0x3f, 0x3f };

//sets up timer isr for 1kHz and gpio for running the 7 segment displays
void setup_score_counter() {
    // Enable Port C
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    // Set PC0-PC10 for output.
    //MODER: 00=input, 01=output, 10=alt, 11=analog
    GPIOC->MODER &= ~(0x3fffff);
    GPIOC->MODER |= 0x155555;

    // setup timer 6
    // Set this to invoke the ISR 100 times per second.
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;

    TIM7->CR1 &= ~TIM_CR1_DIR; //clear to count up [counting direction 0=up, 1=down]

    //set update event to occur at 0.5Hz
    TIM7->PSC = 4800 - 1;  //prescale 48Mz --> 10000

    TIM7->ARR = 10 - 1;  //10000 --> 1000Hz

    TIM7->DIER |= TIM_DIER_UIE;  //generate an interrupt for each update event

    TIM7->CR1 |= TIM_CR1_CEN;  //enable timer

    NVIC->ISER[0] = 1 << TIM7_IRQn;  //enable interrupt for timer
}

void updateScore(int i) {
    i = i % 100; //cut off higher digits
    display[7] = nums[i % 10];
    i /= 10;
    display[6] = nums[i % 10];
}

void show_digit() {
    int off = offset & 7;
    GPIOC->ODR = (off << 8) | display[off];
}

void TIM7_IRQHandler(void) {
    TIM7->SR &= ~TIM_SR_UIF; //acknowledge interrupt
    show_digit();
    offset = (offset + 1) & 0x7; // count 0 ... 7 and repeat
}
