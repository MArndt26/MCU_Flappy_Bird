#include "stm32f0xx.h"
#include <stdint.h>
#include <stdlib.h>
#include "lcd.h"
#include <stdio.h> //for sprintf()

#include "music.h"
#include "seven_seg.h"

//#define HIT_DETECT_EASY

//used for background color
#define TAN 0xd692

void updateScore(int i);

void setup_tim17() {
    // Set this to invoke the ISR 100 times per second.
    RCC->APB2ENR |= RCC_APB2ENR_TIM17EN;

    TIM17->CR1 &= ~TIM_CR1_DIR; //clear to count up [counting direction 0=up, 1=down]

    //set update event to occur at 0.5Hz
    TIM17->PSC = 48000 - 1;  //prescale 48Mz --> 1000

    TIM17->ARR = 10 - 1;  //10000 --> 10Hz

    TIM17->DIER |= TIM_DIER_UIE;  //generate an interrupt for each update event

    TIM17->CR1 |= TIM_CR1_CEN;  //enable timer

    NVIC->ISER[0] = 1 << TIM17_IRQn;  //enable interrupt for timer

    NVIC_SetPriority(TIM17_IRQn, 1);
}

void setup_portb() {
    // Enable Port B.
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    // Set PB0-PB3 for output.
    //MODER: 00=input, 01=output, 10=alt, 11=analog
    GPIOB->MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1 | GPIO_MODER_MODER2
            | GPIO_MODER_MODER3);
    GPIOB->MODER |= GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0
            | GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0;

    // Set PB4-PB7 for input and enable pull-down resistors.
    GPIOB->MODER &= ~(GPIO_MODER_MODER4 | GPIO_MODER_MODER5 | GPIO_MODER_MODER6
            | GPIO_MODER_MODER7);

    //PUPDR: 00=nothing, 01=pull-up, 10=pull-down
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR4 | GPIO_PUPDR_PUPDR5 | GPIO_PUPDR_PUPDR6
            | GPIO_PUPDR_PUPDR7);
    GPIOB->PUPDR |= GPIO_PUPDR_PUPDR4_1 | GPIO_PUPDR_PUPDR5_1
            | GPIO_PUPDR_PUPDR6_1 | GPIO_PUPDR_PUPDR7_1;

    // Turn on the output for the lower row.
    GPIOB->BSRR |= GPIO_BSRR_BS_3;
}

char check_key() {
    int cols = GPIOB->IDR & 0xf0;

    if (cols == GPIO_IDR_7) {
        // If the 'D' key is pressed, return 'D'
        return 'D';
    }
#ifdef HIT_DETECT_EASY
    else if (cols == GPIO_IDR_6) {
        // If the '*' key is pressed, return '*'
        return '*';
    }
#endif
    else {
        // Otherwise, return 0
        return 0;
    }
}

void setup_spi1() {
    // Use setup_spi1() from lab 8.
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    //MODER: 00=input, 01=output, 10=alt, 11=analog
    GPIOA->MODER &=
            ~(GPIO_MODER_MODER4 | GPIO_MODER_MODER5 | GPIO_MODER_MODER7);
    GPIOA->MODER |= (GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1
            | GPIO_MODER_MODER7_1);
    GPIOA->AFR[0] |= 0x0 << (4 * 4) | 0x0 << (5 * 4) | 0x0 << (7 * 4); //not really needed but good to see how to change AFR

    GPIOA->MODER &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
    GPIOA->MODER |= GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0;

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    SPI1->CR1 |= SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE | SPI_CR1_MSTR; //Configure the SPI channel to be in "master mode", but set the BIDIMODE and BIDIOE as well.

    SPI1->CR1 &= ~SPI_CR1_BR; //Set the baud rate as high as possible (make the SCK divisor as small as possible.)

    SPI1->CR2 = SPI_CR2_SSOE | SPI_CR2_NSSP | SPI_CR2_DS_2 | SPI_CR2_DS_1
            | SPI_CR2_DS_0; //Set the SS Output enable bit and enable NSSP.

    SPI1->CR1 |= SPI_CR1_SPE; //Enable the SPI channel.
}

// Copy a subset of a large source picture into a smaller destination.
// sx,sy are the offset into the source picture.
void pic_subset(Picture *dst, const Picture *src, int sx, int sy) {
    int dw = dst->width;
    int dh = dst->height;
    if (dw + sx > src->width)
        for (;;)
            ;
    if (dh + sy > src->height)
        for (;;)
            ;
    for (int y = 0; y < dh; y++)
        for (int x = 0; x < dw; x++)
            dst->pix2[dw * y + x] = src->pix2[src->width * (y + sy) + x + sx];
}

// Overlay a picture onto a destination picture.
// xoffset,yoffset are the offset into the destination picture that the
// source picture is placed.
// Any pixel in the source that is the 'transparent' color will not be
// copied.  This defines a color in the source that can be used as a
// transparent color.
void pic_overlay(Picture *dst, int xoffset, int yoffset, const Picture *src,
        int transparent) {
    for (int y = 0; y < src->height; y++) {
        int dy = y + yoffset;
        if (dy < 0 || dy >= dst->height)
            continue;
        for (int x = 0; x < src->width; x++) {
            int dx = x + xoffset;
            if (dx < 0 || dx >= dst->width)
                continue;
            unsigned short int p = src->pix2[y * src->width + x];
            if (p != transparent)
                dst->pix2[dy * dst->width + dx] = p;
        }
    }
}

// stop the game
void game_over() {
    TIM17->CR1 &= ~TIM_CR1_CEN;  //disable timer
    char line[30];
    sprintf(line, "Game Over");
    LCD_DrawString(20, 300, BLACK, TAN, line, 16, 0);
}

extern const Picture background; // A 240x320 background image
extern const Picture bird; // A 19x19 purple bird with white boundaries
extern const Picture ring; // A 3x70 ring

int rxmin; // Farthest to the left the center of the ring can go
int rxmax; // Farthest to the right the center of the ring can go
int rymin; // Farthest to the top the center of the ring can go
int rymax; // Farthest to the bottom the center of the ring can go
int ymin; // Farthest to the top the center of the bird can go
int ymax; // Farthest to the bottom the center of the bird can go
volatile int y; // Center of bird
volatile int vy; // Velocity components of bird
volatile int ay; // Acceleration of bird

int difficulty = 1;
int ringSpeed = 3;

int rx; // Center of ring in x direction
int ry; // Center of ring in y direction

int score; // game score (number of rings)

// This C macro will create an array of Picture elements.
// Really, you'll just use it as a pointer to a single Picture
// element with an internal pix2[] array large enough to hold
// an image of the specified size.
// BE CAREFUL HOW LARGE OF A PICTURE YOU TRY TO CREATE:
// A 100x100 picture uses 20000 bytes.  You have 32768 bytes of SRAM.
#define TempPicturePtr(name,width,height) Picture name[(width)*(height)/6+2] = { {width,height,2} }

// Create a 29x29 object to hold the bird plus padding
TempPicturePtr(object, 29, 29);
TempPicturePtr(tmp, 29, 29); // Create a global temporary image to draw bird

void TIM17_IRQHandler(void) {
    TIM17->SR &= ~TIM_SR_UIF;

    // handle ring

    if (rx < rxmin * ringSpeed) {
        //clear ring from image
        TempPicturePtr(tmpRing, 12, 70);
        pic_subset(tmpRing, &background, rx - tmpRing->width / 2,
                ry - tmpRing->height / 2); // Copy the background
        LCD_DrawPicture(rx - tmpRing->width / 2, ry - tmpRing->height / 2,
                tmpRing);
        rx = rxmax;
        ry = (random() % (rymax - rymin)) + rymin; //update y position
    } else {
        rx--;
    }

    // handle bouncing the bird
    if (y >= ymax) {
        game_over();
        return;
    }

    char key = check_key();

#ifdef HIT_DETECT_EASY
    if (key == 'D' && y >= ymin) {
        y--;
    } else if (key == '*' && y <= ymax) {
        y++;
    }

#else
    if (key == 'D' && y >= ymin) {
        vy = -75;
    } else {
        ay = 2;
    }

    int addr = vy / 15;
    if (addr > 5)
        addr = 5;
    y += addr;

    vy += ay;

    if (y <= ymin) {
        y = ymin;
        vy += ay;
    } else if (y >= ymax) {
        y = ymax;
        vy += ay;
    } else if (y >= ymax && vy > 0) {
        vy = 0;
    }
#endif

    if (rx == (background.width/2 + bird.width/2) ) {
        if ((y >= (ry - ring.height / 2)) && (y <= (ry + ring.height / 2))) {
            // hit detected
            updateScore(++score);
        }
    }

    //draw ring
    TempPicturePtr(tmpRing, 12, 70);
    pic_subset(tmpRing, &background, rx - tmpRing->width / 2,
            ry - tmpRing->height / 2); // Copy the background
    pic_overlay(tmpRing, 1, 0, &ring, 0xffff);
    LCD_DrawPicture(rx - tmpRing->width / 2, ry - tmpRing->height / 2, tmpRing);

    //draw object with ring overlaid (if applicable)
    pic_subset(tmp, &background, background.width / 2 - tmp->width / 2,
            y - tmp->height / 2); // Copy the background
    pic_overlay(tmp, 0, 0, object, 0xffff); // Overlay the object
    pic_overlay(tmp,
            (rx - ring.width / 2) - (background.width / 2 - tmp->width / 2),
            (ry - ring.height / 2) - (y - tmp->height / 2), &ring, 0xffff); // Draw the ring into the image
    LCD_DrawPicture(background.width / 2 - tmp->width / 2, y - tmp->height / 2,
            tmp); // Re-draw it to the screen
}

void count_down(int count) {
    char line[30];
    sprintf(line, "    %d    ", count);
    LCD_DrawString(20, 300, BLACK, TAN, line, 16, 0);
}

int main(void) {
    setup_portb();
    setup_spi1();
    LCD_Init();
    setup_score_counter();

    // Draw the background.
    LCD_DrawPicture(0, 0, &background);

    // Set all pixels in the object to white.
    for (int i = 0; i < 29 * 29; i++)
        object->pix2[i] = 0xffff;

    // Center the 19x19 bird into center of the 29x29 object.
    // Now, the 19x19 bird has 5-pixel white borders in all directions.
    pic_overlay(object, 5, 5, &bird, 0xffff);

    int border = 2;
    // Initialize the game state.
    ymin = object->height / 2 + border;
    ymax = background.height - object->height / 2 - border - 50;

    rxmin = ring.width / 2 + border;
    rxmax = background.width - ring.width / 2 - border;
    rymin = ring.height / 2 + border;
    rymax = ymax - ring.height / 2;

    y = ymin; // start bird at top of screen
    vy = 1;
    ay = 0;

    rx = rxmax; // start ring on right of screen
    ry = (rxmax + rxmin) / 2; //start ring in center of playable screen

    //draw bird in starting position
    pic_subset(tmp, &background, background.width / 2 - tmp->width / 2,
            y - tmp->height / 2); // Copy the background
    pic_overlay(tmp, 0, 0, object, 0xffff); // Overlay the object
    LCD_DrawPicture(background.width / 2 - tmp->width / 2, y - tmp->height / 2,
            tmp); // Re-draw it to the screen


    //start music
    init_song_len();
    init_wavetable();
    setup_tim1();
    setup_tim6();
    setup_tim14();

    for (int i = 3; i >= 0; i--) {
        count_down(i);
        nano_wait(1e9); //wait 1 second
    }

    setup_tim17();
}
