
/* AASHEIK SARAN , IISC BANGALORE */

#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

#define SW1_PRESSED (!(GPIO_PORTF_DATA_R & (1 << 4)))
#define SW2_PRESSED (!(GPIO_PORTF_DATA_R & (1 << 0)))
#define PC4_PRESSED (!(GPIO_PORTC_DATA_R & (1 << 4)))
#define PC5_PRESSED (!(GPIO_PORTC_DATA_R & (1 << 5)))

#define LED_ON(color) (GPIO_PORTF_DATA_R = (color))
#define LED_OFF() (GPIO_PORTF_DATA_R = 0x00)

#define GREEN    0x08
#define BLUE     0x04
#define CYAN     0x0C
#define RED      0x02
#define YELLOW   0x0A
#define MAGENTA  0x06
#define WHITE    0x0E

void Init_PortF(void);
void Init_PortC_E(void);
void Init_SevenSegment(void);
void DelayMicroseconds(unsigned long time);

volatile uint8_t colorIndex = 0;
volatile uint8_t blinkSpeedIndex = 0;

const uint8_t LED_COLORS[] = {GREEN, BLUE, CYAN, RED, YELLOW, MAGENTA, WHITE};
const uint16_t BLINK_SPEEDS[] = {
    1000,
    500,
    333,
    250,
    200,
    166,
    142,
    125
};

const uint8_t SEG_DIGITS[] = {
    0x3F,
    0x06,
    0x5B,
    0x4F,
    0x66,
    0x6D,
    0x7D,
    0x07,
    0x7F,
    0x6F
};

#define COLOR_COUNT (sizeof(LED_COLORS) / sizeof(LED_COLORS[0]))
#define SPEED_COUNT (sizeof(BLINK_SPEEDS) / sizeof(BLINK_SPEEDS[0]))

void Init_PortF(void) {
    SYSCTL_RCGC2_R |= 0x00000020;
    volatile unsigned long delay = SYSCTL_RCGC2_R;
    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R = 0x1F;
    GPIO_PORTF_AMSEL_R = 0x00;
    GPIO_PORTF_PCTL_R = 0x00000000;
    GPIO_PORTF_DIR_R = 0x0E;
    GPIO_PORTF_AFSEL_R = 0x00;
    GPIO_PORTF_PUR_R = 0x11;
    GPIO_PORTF_DEN_R = 0x1F;
}

void Init_PortC_E(void) {
    SYSCTL_RCGC2_R |= (1 << 2) | (1 << 4);
    GPIO_PORTC_LOCK_R = 0x4C4F434B;
    GPIO_PORTC_CR_R |= (1 << 4) | (1 << 5);
    GPIO_PORTC_DIR_R &= ~((1 << 4) | (1 << 5));
    GPIO_PORTC_DEN_R |= (1 << 4) | (1 << 5);
    GPIO_PORTC_PUR_R |= (1 << 4) | (1 << 5);
    GPIO_PORTE_DIR_R |= (1 << 0);
    GPIO_PORTE_DEN_R |= (1 << 0);
}

void DelayMicroseconds(unsigned long time) {
    unsigned long i;
    for (i = 0; i < 16 * time; i++);
}

void Init_SevenSegment(void) {
    SYSCTL_RCGC2_R |= 0x03;
    GPIO_PORTB_DIR_R = 0xFF;
    GPIO_PORTB_DEN_R = 0xFF;
    GPIO_PORTA_DIR_R |= (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);
    GPIO_PORTA_DEN_R |= (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);
}

void ChangeLEDColor(void) {
    colorIndex = (colorIndex + 1) % COLOR_COUNT;
}

void ChangeBlinkSpeed(void) {
    blinkSpeedIndex = (blinkSpeedIndex + 1) % SPEED_COUNT;
}

void DisplayInfo(void) {
    uint8_t displayData[4] = {0x00, 0x00, 0x00, 0x00};
    displayData[1] = SEG_DIGITS[colorIndex + 1];
    displayData[3] = SEG_DIGITS[blinkSpeedIndex + 1];
    for (int i = 0; i < 4; i++) {
        GPIO_PORTB_DATA_R = displayData[i];
        GPIO_PORTA_DATA_R = (1 << (7 - i));
        DelayMicroseconds(200);
    }
}

int main(void) {
    Init_PortF();
    Init_PortC_E();
    Init_SevenSegment();

    unsigned long counter = 0;
    uint8_t ledState = 0;
    uint16_t currentDelay = BLINK_SPEEDS[blinkSpeedIndex];

    while (1) {
        DisplayInfo();

        if (SW1_PRESSED || PC5_PRESSED) {
            ChangeBlinkSpeed();
            currentDelay = BLINK_SPEEDS[blinkSpeedIndex];
            while (SW1_PRESSED || PC5_PRESSED);
        }

        if (SW2_PRESSED || PC4_PRESSED) {
            ChangeLEDColor();
            while (SW2_PRESSED || PC4_PRESSED);
        }

        if (counter++ >= currentDelay) {
            if (ledState) {
                LED_ON(LED_COLORS[colorIndex]);
            } else {
                LED_OFF();
            }
            ledState = !ledState;
            counter = 0;
        }
    }
}
