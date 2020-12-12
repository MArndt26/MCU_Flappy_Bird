#include "stm32f0xx.h"
#include <stdint.h>
#include <stdlib.h>
#include <math.h>   // for M_PI

#include "music.h"

short int wavetable[N];

int volume = 2048;
int stepa = 0;
int offseta = 0;

#define A3 220.00
#define B3 246.94
#define C4 261.63
#define D4 293.66
#define E4 329.63
#define F4 349.23
#define A4 440.00
#define G4 392.00

#define PAUSE 10.0

#define LAST_NOTE -1

int SONG_LEN = 0;

// Tetris Theme song
// used https://www.youtube.com/watch?v=pRchYIL9KCI
// starting at 1:17 to get the notes for this song
struct {
    float frequency;
    uint16_t duration;
} song[] = {
    { E4, 600 },
    { B3, 300 },
    { C4, 500 },
    { D4, 600 },
    { C4, 300 },
    { B3, 200 },
    { A3, 500 },
    { PAUSE, 300 },

    { A3, 200 },
    { C4, 300 },
    { E4, 500 },
    { D4, 300 },
    { C4, 300 },
    { B3, 500 },
    { PAUSE, 500 },

    { C4, 300 },
    { D4, 600 },
    { E4, 600 },
    { C4, 600 },
    { A3, 400 },
    { PAUSE, 300 },
    { A3, 600 },
    { PAUSE, 1000 },
    { D4, 500 },
    { F4, 300 },
    { A4, 600 },
    { G4, 300 },
    { F4, 300 },
    { E4, 400 },
    { PAUSE, 700 },
    { C4, 300 },
    { E4, 500 },
    { D4, 300 },
    { C4, 300 },
    { B3, 300 },
    { PAUSE, 400 },
    { B3, 300 },
    { C4, 300 },
    { D4, 300 },
    { PAUSE, 300 },
    { E4, 200 },
    { PAUSE, 400 },
    { C4, 300 },
    { PAUSE, 400 },
    { A3, 300 },
    { PAUSE, 400 },
    { A3, 400 },
    { PAUSE, 600 },

    { E4, 700 },
    { PAUSE, 200 },
    { C4, 700 },
    { PAUSE, 200 },
    { D4, 700 },
    { PAUSE, 300 },
    { B3, 700 },
    { PAUSE, 300 },
    { C4, 700 },
    { PAUSE, 300 },
    { A3, 500 },
    { PAUSE, 300 },
    { B3, 700 },

    { PAUSE, 800 },
    { E4, 700 },
    { PAUSE, 400 },
    { C4, 700 },
    { PAUSE, 400 },
    { D4, 700 },
    { PAUSE, 400 },
    { B3, 500 },
    { PAUSE, 400 },
    { C4, 400 },

    { E4, 400 },
    { A4, 600 },
    { D4, 1000 },


    { PAUSE, 3000 },
    {LAST_NOTE, 0},
};

int song_off = 0;
int cur_duration = 0;

//============================================================================
// setup_tim1()    (Autotest #1)
// Configure Timer 1 and the PWM output pins.
// Parameters: none
//============================================================================
void setup_tim1()
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    //MODER: 00=input, 01=output, 10=alt, 11=analog
    GPIOA->MODER &= ~(GPIO_MODER_MODER11); //setup PA11 as "alternate function"
    GPIOA->MODER |= GPIO_MODER_MODER11_1;

    // set the alternate functions for PA8 - PA11
    // PA0-7 are on AFR[0], 8-15 are on AFR[1]
    // column of table 14 of datasheet indicate what AFx to set
    // page 41 of datasheet
    GPIOA->AFR[1] &= ~0xffff;
    GPIOA->AFR[1] |= 0x2222;

    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    // counting direction 0=up, 1=down
    TIM1->CR1 &= ~TIM_CR1_DIR;  // clear it to count up

    TIM1->BDTR |= TIM_BDTR_MOE;  //BDTR break and dead-time register, MOE Main Output enable

    TIM1->PSC = 0; //prescale by 1

    TIM1->ARR = 2400 - 1;  //set timer freq to 20kHz

    TIM1->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1; //set timer to mode 1 (pwm) (OC1M = 110) for channel 1
    TIM1->CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1; //set timer to mode 1 (pwm) (OC1M = 110) for channel 2
    TIM1->CCMR2 |= TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1; //set timer to mode 1 (pwm) (OC1M = 110) for channel 3
    TIM1->CCMR2 |= TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4PE; //set timer to mode 1 (pwm) (OC1M = 110) for channel 4

    TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;  //enable output

    TIM1->CR1 |= TIM_CR1_CEN;   //enable timer
}

//============================================================================
// init_wavetable()    (Autotest #2)
// Write the pattern for one complete cycle of a sine wave into the
// wavetable[] array.
// Parameters: none
//============================================================================
void init_wavetable(void)
{
    for (int i = 0; i < N; i++)
    {
        wavetable[i] = 32767 * sin(2 * M_PI * i / N);
    }
}

void init_song_len(void) {
    int i = 0;
    SONG_LEN = 0;
    while (song[i].frequency != LAST_NOTE) {
        SONG_LEN++;
        i++;
    }
    ;
}

//============================================================================
// set_freq_n()    (Autotest #2)
// Set the four step and four offset variables based on the frequency.
// Parameters: f: The floating-point frequency desired.
//============================================================================
void set_freq_a(float f)
{
    if (f == 0.0)
    {
        stepa = 0;
        offseta = 0;
    }
    else
    {
        stepa = f * N / RATE * (1<<16);
    }
}

//============================================================================
// Timer 6 ISR    (Autotest #2)
// The ISR for Timer 6 which computes the DAC samples.
// Parameters: none
// (Write the entire subroutine below.)
//============================================================================
void TIM6_DAC_IRQHandler(void)
{
    TIM6->SR &= ~TIM_SR_UIF;  //write 0 to UIF to acknowledge interrupt
//  DAC->SWTRIGR |= DAC_SWTRIGR_SWTRIG1;    //initiate DAC software trigger
    offseta += stepa;
    if ((offseta>>16) >= N) //If we go past the end of the array
        offseta -= N<<16;  //wrap around with same offset as overshoot


    int sample = 0;
    sample += wavetable[offseta>>16];

//  sample = sample / 32 + 2048; //adjust for DAC range

    sample = ((sample * volume)>>17) + 1200;

    if (sample > 4095)      sample = 4095;      // clip high end
    else if (sample < 0)    sample = 0;         // clip low end

//  DAC->DHR12R1 = sample;                  //write sample to DAC data holding register
    TIM1->CCR4 = sample;        //write sample to TIMER capture control register for channel 4

    //adjust volume from ADC input
//  start_adc_channel(0);
//
//  volume = read_adc();
}

//============================================================================
// setup_tim6()    (Autotest #2)
// Set tim6 to create update event at 20kHz
// Parameters: none
//============================================================================
void setup_tim6()
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    TIM6->CR1 &= ~TIM_CR1_DIR; //clear to count up [counting direction 0=up, 1=down]

    //set update event to occur at 20kHz
    TIM6->PSC = 1200 - 1;  //prescale 48Mz / 1200 --> 40k

    TIM6->ARR = 2 - 1;  //auto reload 2

    TIM6->DIER |= TIM_DIER_UIE;  //generate an interrupt for each update event

    TIM6->CR1 |= TIM_CR1_CEN;  //enable timer

    NVIC->ISER[0] = 1<<TIM6_DAC_IRQn;  //enable interrupt for timer
}

// set tim14 to refresh at 1Hz
void setup_tim14() {
    RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;

    TIM14->CR1 &= ~TIM_CR1_DIR; //clear to count up [counting direction 0=up, 1=down]

    //set update event to occur at 20kHz
    TIM14->PSC = 48000 - 1;  //prescale

    TIM14->ARR = 100 - 1;  //auto reload

    TIM14->DIER |= TIM_DIER_UIE;  //generate an interrupt for each update event

    TIM14->CR1 |= TIM_CR1_CEN;  //enable timer

    NVIC->ISER[0] = 1<<TIM14_IRQn;  //enable interrupt for timer
}

// step through the table of notes to play the song
void TIM14_IRQHandler(void) {
    TIM14->SR &= ~TIM_SR_UIF;  //write 0 to UIF to acknowledge interrupt
    set_freq_a(song[song_off].frequency);
    cur_duration += 100;
    if (cur_duration >= song[song_off].duration) {
        song_off++;
        cur_duration = 0;
    }
    if (song_off >= SONG_LEN) {
        song_off = 0; //wrap around
    }
}
