#include <stdint.h>
#include "inc/tm4c123gh6pm.h"


#define ROW1  (0b00000001)  // PE0
#define ROW2  (0b00000010)  // PE1
#define ROW3  (0b00000100)  // PE2
#define ROW4  (0b00001000)  // PE3
#define COL1  (0b00010000)  // PC4
#define COL2  (0b00100000)  // PC5
#define COL3  (0b01000000)  // PC6
#define COL4  (0b10000000)  // PC7


#define LED_RED    0b00000010
#define LED_BLUE   0b00000100
#define LED_GREEN  0b00001000
#define LED_WHITE  0b00001110
#define LED_DARK   0b00000000
#define LED_YELLOW 0b00001010
#define LED_SKYBLUE 0b00001100


volatile uint8_t disp[4];
uint8_t colors[7] = {LED_RED, LED_BLUE, LED_GREEN, LED_WHITE, LED_DARK, LED_YELLOW, LED_SKYBLUE};
uint32_t delays[7] = {1000000, 800000, 600000, 400000, 200000, 100000, 50000};
volatile unsigned long i = 0, j = 0;
int m = 0;
char key;

// Timing Variables
volatile unsigned long Count = 0;
volatile unsigned long current = 0;
volatile unsigned long blink = 0;
int Delay = 200;

// 7-Segment Display Encoding
#define R 0b00111001
#define E 0b01111001
#define D 0b01011110
#define B 0b01111100
#define L 0b00111000
#define U 0b00111110
#define G 0b00101111
#define N 0b00110111
#define W 0b01110111
#define H 0b01110100
#define T 0b01111000
#define A 0b01110111
#define K 0b01110011
#define Y 0b01101110
#define O 0b00111111
#define S 0b01011011

// Initialize GPIO and Ports
void Init(void)
{
    SYSCTL_RCGC2_R |= 0b00110111;  // Enable clock for Ports A, B, C, E, F
    while ((SYSCTL_PRGPIO_R & 0b00110111) == 0);

    GPIO_PORTE_DIR_R = 0b00001111;   // PE0-PE3 as output (rows)
    GPIO_PORTE_DEN_R = 0b00011111;   // Enable digital function (PE4 for SW1)

    GPIO_PORTC_DIR_R &= 0b00000000;  // PC4-PC7 as input (columns)
    GPIO_PORTC_DEN_R |= 0b11110000;  // Enable digital function
    GPIO_PORTC_PUR_R |= 0b11110000;  // Enable pull-up resistors (PC5 for SW2)

    GPIO_PORTB_DIR_R = 0xFF;  // Segment display output
    GPIO_PORTB_DEN_R = 0xFF;

    GPIO_PORTA_DIR_R = 0b11110000;  // Control lines for segment display
    GPIO_PORTA_DEN_R = 0b11110000;

    GPIO_PORTF_LOCK_R = 0x4C4F434B;  // Unlock GPIOF
    GPIO_PORTF_CR_R = 0x1F;
    GPIO_PORTF_DIR_R = 0b00001110;
    GPIO_PORTF_DEN_R = 0b00011111;
    GPIO_PORTF_PUR_R = 0b00010001;  // Enable pull-up resistors
}

// Simple delay function
void delay_time(uint32_t time) {
    volatile uint32_t k;
    for (k = 0; k < time; k++);
}

// Keypad scanning function
char Keypad_Scan(void) {
    uint8_t row, col;
    char key = 0xFF; // No key pressed by default

    for (row = 0; row < 4; row++)
    {
        GPIO_PORTE_DATA_R = (1 << row);  // Activate only one row
        delay_time(10);  // Small delay for stabilization

        col = GPIO_PORTC_DATA_R & 0b11110000; // Read column values

        if (col != 0b11110000)     // If any key is pressed
        {
            if (col == 0b11100000 && row == 0) key = '1';
            if (col == 0b11010000 && row == 0) key = '2';
            return key; // Return key immediately after detection
        }
    }
    return 0xFF; // No key detected
}

// Function to update the 7-segment display based on color selection
void segment()
{
    switch (i)
    {
        case 0: disp[3] = R; disp[2] = E; disp[1] = D; disp[0] = 0; break;
        case 1: disp[3] = B; disp[2] = L; disp[1] = U; disp[0] = E; break;
        case 2: disp[3] = G; disp[2] = R; disp[1] = N; disp[0] = 0; break;
        case 3: disp[3] = W; disp[2] = H; disp[1] = T; disp[0] = E; break;
        case 4: disp[3] = D; disp[2] = A; disp[1] = R; disp[0] = K; break;
        case 5: disp[3] = Y; disp[2] = L; disp[1] = O; disp[0] = W; break;
        case 6: disp[3] = S; disp[2] = K; disp[1] = Y; disp[0] = 0; break;
        default: disp[3] = 0; disp[2] = 0; disp[1] = 0; disp[0] = 0; break;
    }
}


void display_segment()
{
    m = 0;
    while (m < 4)
    {
        GPIO_PORTB_DATA_R = disp[m];
        GPIO_PORTA_DATA_R = (0b00010000 << m);
        delay_time(10000); // Delay to reduce flickering
        m++;
    }
}


void keypad()
{
    key = Keypad_Scan();  // Read key input

    if (key == '1')
    {
        delay_time(50000);  // Debounce delay
        i = (i + 1) % 7; // Cycle through LED colors
        while (Keypad_Scan() == '1');  // Wait for release
    }

    if (key == '2')
    {
        delay_time(50000);  // Debounce delay
        Delay+= 100; // Increase blink delay
        if (Delay> 700) Delay = 100; // Wrap around after a threshold
        while (Keypad_Scan() == '2');  // Wait for release
    }
}


void initTimer(void)
{
    SYSCTL_RCGCTIMER_R |= (1 << 0); //Enable clock for Timer0
    TIMER0_CTL_R = 0; //Disable timer before initialization
    TIMER0_CFG_R = 0x00000000; //16-bit option
    TIMER0_TAMR_R |= (0x02 << 0); //Set periodic mode
    TIMER0_TAMR_R &= (1 << 4); //Set down counter mode
    TIMER0_TAILR_R = 16000000 / 1000; //Give load value register a value of 16,000,000
    TIMER0_CTL_R |= 0x01; //Enable Timer0 after initialization
}

int time(void)
{
    if ((TIMER0_RIS_R & 0x01) == 1)
    {
        TIMER0_ICR_R |= (1 << 0);  // Clear interrupt flag
        Count++;
    }
    return Count;
}

// Main function
int main(void)
{
    Init();  // Initialize hardware
    initTimer(); // Initialize timer
    key = 0xFF; // Initialize key state

    while (1)
    {
        current = time();
        segment();          // Update segment display buffer
        display_segment();  // Refresh 7-segment display
        keypad();           // Read and process keypad input


            GPIO_PORTF_DATA_R = colors[i];  // Set LED color
            delay_time(10000); // LED ON time
            GPIO_PORTF_DATA_R = LED_DARK;  // Turn off LED
            blink = current;

    }
}
