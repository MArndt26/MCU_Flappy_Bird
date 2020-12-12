/*
 * music.h
 *
 *  Created on: Dec 5, 2020
 *      Author: mitch
 */

#ifndef MUSIC_H_
#define MUSIC_H_

//============================================================================
// Parameters for the wavetable size and expected synthesis rate.
//============================================================================
#define N 1000
#define RATE 20000

void setup_tim1();

void init_wavetable(void);

void set_freq_a(float f);

void TIM6_DAC_IRQHandler(void);

void setup_tim6();

void setup_tim14();

void TIM14_IRQHandler(void);

#endif /* MUSIC_H_ */
