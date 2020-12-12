/*
 * seven_seg.h
 *
 *  Created on: Dec 5, 2020
 *      Author: mitch
 */

#ifndef SEVEN_SEG_H_
#define SEVEN_SEG_H_

#define letter_s    0x6d
#define letter_c    0x39
#define letter_o    0x3f
#define letter_r    0x50
#define letter_e    0x79
#define dash        0x40

void setup_score_counter();

void updateScore(int i);

void show_digit();

void TIM7_IRQHandler(void);

#endif /* SEVEN_SEG_H_ */
