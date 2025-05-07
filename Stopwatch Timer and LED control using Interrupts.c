/*Stopwatch Timer and LED control using Interrupts*/

#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "./inc/tm4c123gh6pm.h"
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/uart.h>
#include <driverlib/interrupt.h>

#define LED_GREEN  0x08
#define LED_BLUE   0x04
#define LED_CYAN   0x0C
#define LED_RED    0x02
#define LED_YELLOW 0x0A
#define LED_MAGENTA 0x06
#define LED_WHITE  0x0E
#define LED_OFF    0x00

#define SEG_0   0x3F  // 0
#define SEG_1   0x06  // 1
#define SEG_2   0x5B  // 2
#define SEG_3   0x4F  // 3
#define SEG_4   0x66  // 4
#define SEG_5   0x6D  // 5
#define SEG_6   0x7D  // 6
#define SEG_7   0x07  // 7
#define SEG_8   0x7F  // 8
#define SEG_9   0x6F  // 9

#define MAX_BLINK_DELAY 1000
#define BUFFER_SIZE 30
#define UART_BAUD_RATE 115200
#define SYSTICK_FREQ 100  // 10ms intervals

#define SPACE ' '
#define NULL_CHAR '\0'
#define ENTER '\r'
#define BACKSPACE '\b'

#define SW1 (!(GPIO_PORTF_DATA_R & 0x10)) // Switch 1 (PF4) - Start/Stop
#define SW2 (!(GPIO_PORTF_DATA_R & 0x01)) // Switch 2 (PF0) - Pause/Resume

volatile uint32_t sysTickCount = 0;
int blink_delay = MAX_BLINK_DELAY;
int select_color = 0;
uint8_t currentColor = LED_GREEN;
char colors_list[7][8] = {"green", "blue", "cyan", "red", "yellow", "magenta", "white"};
const uint8_t sevenSegmentMap[10] = {SEG_0, SEG_1, SEG_2, SEG_3, SEG_4, SEG_5, SEG_6, SEG_7, SEG_8, SEG_9};
char command[BUFFER_SIZE] = {NULL_CHAR};
int commandIndex = 0;
volatile bool stopwatch_running = false;
volatile bool stopwatch_paused = false;
volatile uint32_t milliseconds = 0;
volatile uint32_t seconds = 0;
volatile uint32_t minutes = 0;
volatile bool display_ms = false;  // Toggle between MM:SS and M:SS:ms
volatile uint32_t blink_time = 0;  // For blink command duration (in 10ms ticks)
volatile bool blinking = false;

void delayMilliseconds(int n);
void update_color();
void sendUARTMessage(const char *message);
void clearBuffer();
int validate_color();
int validate_blink_command();
int validate_stopwatch_command();
void run_command();
void initSystem();
void updateSevenSegment();
void UART0_Handler(void);
void GPIOF_Handler(void);
void SysTick_Handler(void);
void stopwatch_start();
void stopwatch_stop();
void stopwatch_pause();
void stopwatch_resume();
void verification_Command();

int main()
{
    initSystem();
    sendUARTMessage("\n\rSystem Initialized Successfully!\n\r");

    while (1)
    {
        // Handle LED blinking
        if (blinking && blink_time > 0) {
            GPIO_PORTF_DATA_R = (sysTickCount % (SYSTICK_FREQ / (1000 / blink_delay)) == 0) ? currentColor : LED_OFF;
            if (sysTickCount % SYSTICK_FREQ == 0) blink_time--;
            if (blink_time == 0) blinking = false;
        } else if (!blinking) {
            GPIO_PORTF_DATA_R = currentColor;
        }

        // Update seven-segment display if stopwatch is running
        if (stopwatch_running && !stopwatch_paused) {
            updateSevenSegment();
        } else {
            GPIO_PORTA_DATA_R = 0x00; // Clear display when paused or stopped
        }
    }
}

void initSystem()
{
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // GPIO Port F setup (LEDs and Switches)
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    while (!(SYSCTL_PRGPIO_R & SYSCTL_RCGCGPIO_R5));
    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R |= 0x1F;
    GPIO_PORTF_DEN_R |= 0x1F;
    GPIO_PORTF_DIR_R |= 0x0E;  // PF1-3 for LEDs
    GPIO_PORTF_DIR_R &= ~0x11; // PF0, PF4 as inputs (SW2, SW1)
    GPIO_PORTF_PUR_R |= 0x11;  // Enable pull-up for switches
    GPIOIntTypeSet(GPIO_PORTF_BASE, 0x11, GPIO_FALLING_EDGE);
    GPIOIntEnable(GPIO_PORTF_BASE, 0x11);
    IntEnable(INT_GPIOF);

    // GPIO Port B and A setup (7-segment display)
    SYSCTL_RCGCGPIO_R |= 0x03;
    while (!(SYSCTL_PRGPIO_R & 0x03));
    GPIO_PORTB_DIR_R = 0xFF;  // Port B for segments a-g
    GPIO_PORTB_DEN_R = 0xFF;
    GPIO_PORTA_DIR_R = 0xF0;  // PA4-PA7 for digit selection
    GPIO_PORTA_DEN_R = 0xF0;

    // UART0 setup
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!(SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R0)); // Wait for GPIOA to be ready
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), UART_BAUD_RATE,
                       (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    UARTFIFOEnable(UART0_BASE); // Enable UART FIFO to handle multiple characters
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT); // Enable RX and timeout interrupts
    IntPrioritySet(INT_UART0, 0x00); // Set UART0 interrupt priority (optional, adjust as needed)
    IntEnable(INT_UART0);
    UARTEnable(UART0_BASE);

    // SysTick setup for 10ms intervals
    SysTickPeriodSet(SysCtlClockGet() / SYSTICK_FREQ);
    SysTickIntEnable();
    SysTickEnable();

    IntMasterEnable();
}

void update_color()
{
    if (select_color == -1) {
        currentColor = LED_OFF;
    } else {
        uint8_t color_map[] = {LED_GREEN, LED_BLUE, LED_CYAN, LED_RED, LED_YELLOW, LED_MAGENTA, LED_WHITE};
        currentColor = color_map[select_color];
    }
}

void delayMilliseconds(int n)
{
    SysCtlDelay((SysCtlClockGet() / 3000) * n);
}

void SysTick_Handler(void)
{
    sysTickCount++;
    if (stopwatch_running && !stopwatch_paused) {
        milliseconds += 10;
        if (milliseconds >= 1000) {
            milliseconds = 0;
            seconds++;
            if (seconds >= 60) {
                seconds = 0;
                minutes++;
                if (minutes >= 60) minutes = 0; // Reset minutes after 60
            }
        }
    }
}

void GPIOF_Handler(void)
{
    uint32_t status = GPIOIntStatus(GPIO_PORTF_BASE, true);
    GPIOIntClear(GPIO_PORTF_BASE, status);
    SysCtlDelay(SysCtlClockGet() / 30000); // Debounce delay

    if (status & 0x10) {  // SW1 (PF4) - Start/Stop
        if (!stopwatch_running) stopwatch_start();
        else stopwatch_stop();
    }
    if (status & 0x01) {  // SW2 (PF0) - Pause/Resume
        if (stopwatch_running) {
            if (stopwatch_paused) stopwatch_resume();
            else stopwatch_pause();
        }
    }
}

void updateSevenSegment()
{
    uint8_t digits[4];
    if (display_ms) {
        digits[0] = sevenSegmentMap[minutes % 10];         // Minutes (ones)
        digits[1] = sevenSegmentMap[seconds / 10];         // Seconds (tens)
        digits[2] = sevenSegmentMap[seconds % 10];         // Seconds (ones)
        digits[3] = sevenSegmentMap[(milliseconds / 10) % 10]; // Milliseconds (tens)
    } else {
        digits[0] = sevenSegmentMap[minutes];              // Minutes
        digits[1] = sevenSegmentMap[seconds / 10];         // Seconds (tens)
        digits[2] = sevenSegmentMap[seconds % 10];         // Seconds (ones)
        digits[3] = SEG_0;                                 // Blank
    }

    // Multiplexing the 7-segment display
    for (int i = 0; i < 4; i++) {
        GPIO_PORTB_DATA_R = digits[i];
        GPIO_PORTA_DATA_R = (1 << (7 - i));  // Activate digit (PA7-PA4)
        delayMilliseconds(2);                // Short delay for multiplexing
    }
    GPIO_PORTA_DATA_R = 0x00;  // Turn off all digits
}

void UART0_Handler(void)
{
    uint32_t status = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, status);

    while (UARTCharsAvail(UART0_BASE)) {
        char receivedChar = UARTCharGet(UART0_BASE);

        if (receivedChar == ENTER) {
            command[commandIndex] = NULL_CHAR;
            verification_Command();  // Clean the command
            sendUARTMessage("\n\rReceived Command: ");
            sendUARTMessage(command);
            sendUARTMessage("\n\r");
            run_command();
            clearBuffer();
            commandIndex = 0; // Reset index after processing
        }
        else if (receivedChar == BACKSPACE || receivedChar == 0x7F) {
            if (commandIndex > 0) {
                commandIndex--;
                command[commandIndex] = NULL_CHAR;
                UARTCharPut(UART0_BASE, BACKSPACE);
                UARTCharPut(UART0_BASE, SPACE);
                UARTCharPut(UART0_BASE, BACKSPACE);
            }
        }
        else if (isalpha(receivedChar) || isdigit(receivedChar) || receivedChar == SPACE) {
            if (commandIndex < BUFFER_SIZE - 1) {
                command[commandIndex++] = tolower(receivedChar);
                UARTCharPut(UART0_BASE, receivedChar); // Echo character
            } else {
                UARTCharPut(UART0_BASE, '\a'); // Bell sound for buffer full
            }
        }
    }
}

void sendUARTMessage(const char *message)
{
    while (*message) {
        UARTCharPut(UART0_BASE, *message++);
    }
    UARTCharPut(UART0_BASE, '\n');
    UARTCharPut(UART0_BASE, '\r');
}

void clearBuffer()
{
    memset(command, NULL_CHAR, BUFFER_SIZE);
    commandIndex = 0;
}

void verification_Command()
{
    int i, j = 0;
    char temp[BUFFER_SIZE] = {0};
    for (i = 0; command[i] != NULL_CHAR && i < BUFFER_SIZE; i++)
    {
        if (isalpha(command[i]) || isdigit(command[i]) || command[i] == SPACE)
        {
            temp[j++] = command[i];
        }
    }
    temp[j] = NULL_CHAR;
    strcpy(command, temp);
}

int validate_color()
{
    char entered_color[8];
    if (sscanf(command, "led %7s", entered_color) == 1) {
        if (strcasecmp(entered_color, "off") == 0) {
            select_color = -1;
            update_color();
            sendUARTMessage("\n\rLED Turned Off\n\r");
            return 1;
        }
        for (int i = 0; i < 7; i++) {
            if (strcasecmp(entered_color, colors_list[i]) == 0) {
                select_color = i;
                update_color();
                sendUARTMessage("\n\rLED Color Updated to ");
                sendUARTMessage(colors_list[i]);
                sendUARTMessage("\n\r");
                return 1;
            }
        }
    }
    return 0;
}

int validate_blink_command()
{
    char entered_color[8];
    int time;
    if (sscanf(command, "led blink %7s %d", entered_color, &time) == 2) {
        for (int i = 0; i < 7; i++) {
            if (strcasecmp(entered_color, colors_list[i]) == 0) {
                select_color = i;
                update_color();
                blink_time = time * 100;  // Convert seconds to 10ms ticks
                blinking = true;
                sendUARTMessage("\n\rLED Blinking Started with ");
                sendUARTMessage(colors_list[i]);
                sendUARTMessage(" for ");
                char time_str[4];
                sprintf(time_str, "%d", time);
                sendUARTMessage(time_str);
                sendUARTMessage(" seconds\n\r");
                return 1;
            }
        }
    }
    return 0;
}

int validate_stopwatch_command()
{
    char entered_command[8];
    if (sscanf(command, "swt %7s", entered_command) == 1) {
        if (strcasecmp(entered_command, "start") == 0) {
            stopwatch_start();
            return 1;
        }
        else if (strcasecmp(entered_command, "stop") == 0) {
            stopwatch_stop();
            return 1;
        }
        else if (strcasecmp(entered_command, "pause") == 0) {
            stopwatch_pause();
            return 1;
        }
        else if (strcasecmp(entered_command, "resume") == 0) {
            stopwatch_resume();
            return 1;
        }
    }
    return 0;
}

void stopwatch_start()
{
    if (!stopwatch_running) {
        stopwatch_running = true;
        stopwatch_paused = false;
        sendUARTMessage("\n\rStopwatch Started\n\r");
    }
}

void stopwatch_stop()
{
    if (stopwatch_running) {
        stopwatch_running = false;
        stopwatch_paused = false;
        milliseconds = 0;  // Reset milliseconds
        seconds = 0;       // Reset seconds
        minutes = 0;       // Reset minutes
        sendUARTMessage("\n\rStopwatch Stopped and Reset to 0\n\r");
        updateSevenSegment();  // Update display to show 00:00 or 0:00:00
    }
}

void stopwatch_pause()
{
    if (stopwatch_running && !stopwatch_paused) {
        stopwatch_paused = true;
        sendUARTMessage("\n\rStopwatch Paused\n\r");
    }
}

void stopwatch_resume()
{
    if (stopwatch_running && stopwatch_paused) {
        stopwatch_paused = false;
        sendUARTMessage("\n\rStopwatch Resumed\n\r");
    }
}

void run_command()
{
    if (!validate_color() && !validate_blink_command() && !validate_stopwatch_command())
    {
        sendUARTMessage("\n\rGIVEN COMMAND IS INVALID. VALID COMMANDS:\n\r"
                       "[led <name>]\n\r"
                       "[led off]\n\r"
                       "[led blink <color> <time>]\n\r"
                       "[swt <start/stop/pause/resume>]\n\r");
    }
    clearBuffer();
}
