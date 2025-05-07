/* AASHEIK SARAN IISC,BANGALORE */

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
#define NEW_LINE '\n'
#define TAB '\t'

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
volatile bool display_ms = false;
volatile uint32_t blink_time = 0;
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
int validate_peek();
int validate_poke();

int main()
{
    initSystem();
    sendUARTMessage("System Initialized Successfully!\n\r");

    while (1)
    {
        if (blinking && blink_time > 0) {
            GPIO_PORTF_DATA_R = (sysTickCount % (SYSTICK_FREQ / (1000 / blink_delay)) == 0) ? currentColor : LED_OFF;
            if (sysTickCount % SYSTICK_FREQ == 0) blink_time--;
            if (blink_time == 0) blinking = false;
        } else if (!blinking) {
            GPIO_PORTF_DATA_R = currentColor;
        }

        if (stopwatch_running && !stopwatch_paused) {
            updateSevenSegment();
        } else {
            GPIO_PORTA_DATA_R = 0x00;
        }
    }
}

void initSystem()
{
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    while (!(SYSCTL_PRGPIO_R & SYSCTL_RCGCGPIO_R5));
    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R |= 0x1F;
    GPIO_PORTF_DEN_R |= 0x1F;
    GPIO_PORTF_DIR_R |= 0x0E;
    GPIO_PORTF_DIR_R &= ~0x11;
    GPIO_PORTF_PUR_R |= 0x11;
    GPIOIntTypeSet(GPIO_PORTF_BASE, 0x11, GPIO_FALLING_EDGE);
    GPIOIntEnable(GPIO_PORTF_BASE, 0x11);
    IntEnable(INT_GPIOF);

    SYSCTL_RCGCGPIO_R |= 0x03;
    while (!(SYSCTL_PRGPIO_R & 0x03));
    GPIO_PORTB_DIR_R = 0xFF;
    GPIO_PORTB_DEN_R = 0xFF;
    GPIO_PORTA_DIR_R = 0xF0;
    GPIO_PORTA_DEN_R = 0xF0;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!(SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R0));
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), UART_BAUD_RATE,
                       (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    UARTFIFOEnable(UART0_BASE);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    IntPrioritySet(INT_UART0, 0x00);
    IntEnable(INT_UART0);
    UARTEnable(UART0_BASE);

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
    if (stopwatch_running && !stopwatch_paused)
    {
        milliseconds += 10;
        if (milliseconds >= 1000)
        {
            milliseconds = 0;
            seconds++;
            if (seconds >= 60)
            {
                seconds = 0;
                minutes++;
                if (minutes >= 60) minutes = 0;
            }
        }
    }
}

void GPIOF_Handler(void)
{
    uint32_t status = GPIOIntStatus(GPIO_PORTF_BASE, true);
    GPIOIntClear(GPIO_PORTF_BASE, status);
    SysCtlDelay(SysCtlClockGet() / 30000);

    if (status & 0x10) {
        if (!stopwatch_running) stopwatch_start();
        else stopwatch_stop();
    }
    if (status & 0x01) {
        if (stopwatch_running) {
            if (stopwatch_paused) stopwatch_resume();
            else stopwatch_pause();
        }
    }
}

void updateSevenSegment()
{
    uint8_t digits[4];
    uint8_t dots[4] = {0, 0, 0, 0};

    digits[0] = sevenSegmentMap[(minutes / 10) % 10];
    digits[1] = sevenSegmentMap[minutes % 10];
    digits[2] = sevenSegmentMap[seconds / 10];
    digits[3] = sevenSegmentMap[seconds % 10];
     dots[1] = 1;

    for (int i = 0; i < 4; i++) {
        GPIO_PORTA_DATA_R = 0x00;
        uint8_t pattern = digits[i];
        if (dots[i]) pattern |= 0x80;
        GPIO_PORTB_DATA_R = pattern;
        GPIO_PORTA_DATA_R = (1 << (7 - i));
        delayMilliseconds(2);
    }
    GPIO_PORTA_DATA_R = 0x00;
}

void UART0_Handler(void)
{
    uint32_t status = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, status);

    while (UARTCharsAvail(UART0_BASE)) {
        char receivedChar = UARTCharGet(UART0_BASE);

        if (receivedChar == ENTER) {
            command[commandIndex] = NULL_CHAR;
            verification_Command();
            sendUARTMessage("\n\rReceived Command: ");
            sendUARTMessage(command);
            sendUARTMessage("\n\r");
            run_command();
            clearBuffer();
            commandIndex = 0;
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
                UARTCharPut(UART0_BASE, receivedChar);
            } else {
                UARTCharPut(UART0_BASE, '\a');
            }
        }
    }
}

void sendUARTMessage(const char *message)
{
    while (*message) {
        UARTCharPut(UART0_BASE, *message++);
    }
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
    strncpy(command, temp, BUFFER_SIZE - 1);
    command[BUFFER_SIZE - 1] = NULL_CHAR;

    if (strncmp(command, "poke", 4) == 0)
    {
        char correct_cmd[BUFFER_SIZE] = {0};
        int correct_cmd_index = 4;
        int space_count = 0;

        strncpy(correct_cmd, "poke", 4);

        for (i = 4; i < BUFFER_SIZE && command[i] != NULL_CHAR; i++)
        {
            if (command[i] != SPACE)
            {
                correct_cmd[correct_cmd_index++] = command[i];
            }
            else if (space_count < 2)
            {
                correct_cmd[correct_cmd_index++] = SPACE;
                space_count++;
                while (command[i] == SPACE && i < BUFFER_SIZE - 1) i++;
                i--;
            }
        }
        correct_cmd[correct_cmd_index] = NULL_CHAR;
        strncpy(command, correct_cmd, BUFFER_SIZE - 1);
        command[BUFFER_SIZE - 1] = NULL_CHAR;
    }
}

int validate_color()
{
    char entered_color[8];
    if (sscanf(command, "led %7s", entered_color) == 1) {
        if (strcasecmp(entered_color, "off") == 0) {
            select_color = -1;
            update_color();
            sendUARTMessage("LED Turned Off\n\r");
            return 1;
        }
        for (int i = 0; i < 7; i++) {
            if (strcasecmp(entered_color, colors_list[i]) == 0) {
                select_color = i;
                update_color();
                sendUARTMessage("LED Color Updated to ");
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
                blink_time = time * 100;
                blinking = true;
                sendUARTMessage("LED Blinking Started with ");
                sendUARTMessage(colors_list[i]);
                sendUARTMessage(" for ");
                char time_str[5];
                snprintf(time_str, sizeof(time_str), "%d", time);
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
    if (sscanf(command, "swt %7s", entered_command) == 1)
    {
        if (strcasecmp(entered_command, "start") == 0) return stopwatch_start(), 1;
        if (strcasecmp(entered_command, "stop") == 0) return stopwatch_stop(), 1;
        if (strcasecmp(entered_command, "pause") == 0) return stopwatch_pause(), 1;
        if (strcasecmp(entered_command, "resume") == 0) return stopwatch_resume(), 1;
    }
    return 0;
}

void stopwatch_start()
{
    if (!stopwatch_running)
    {
        stopwatch_running = true;
        stopwatch_paused = false;
        sendUARTMessage("Stopwatch Started\n\r");
    }
}

void stopwatch_stop()
{
    if (stopwatch_running)
    {
        stopwatch_running = false;
        stopwatch_paused = false;
        milliseconds = seconds = minutes = 0;
        sendUARTMessage("Stopwatch Stopped and Reset to 0\n\r");
        updateSevenSegment();
    }
}

void stopwatch_pause()
{
    if (stopwatch_running && !stopwatch_paused) {
        stopwatch_paused = true;
        sendUARTMessage("Stopwatch Paused\n\r");
    }
}

void stopwatch_resume()
{
    if (stopwatch_running && stopwatch_paused) {
        stopwatch_paused = false;
        sendUARTMessage("Stopwatch Resumed\n\r");
    }
}

int validate_peek()
{
    char temp_cmd[BUFFER_SIZE];
    strncpy(temp_cmd, command, BUFFER_SIZE - 1);
    temp_cmd[BUFFER_SIZE - 1] = NULL_CHAR;

    if (strncmp(temp_cmd, "peek", 4) == 0)
    {
        int address, bytes;
        if (sscanf(temp_cmd, "peek %x %d", &address, &bytes) == 2)
        {
            if (address >= 0x20000000 && address <= 0x20008000 && bytes > 0 && bytes <= 100)
            {
                sendUARTMessage("Data at given address is: ");
                for (int i = 0; i < bytes; i++) {
                    char data = *((char*)address + i);
                    if (data == NULL_CHAR) sendUARTMessage("_NULL_");
                    else if (data == NEW_LINE) sendUARTMessage("_NEW LINE_");
                    else if (data == ENTER) sendUARTMessage("_ENTER_");
                    else if (data == SPACE) sendUARTMessage("_SPACE_");
                    else if (data == TAB) sendUARTMessage("_TAB_");
                    else if (data == BACKSPACE) sendUARTMessage("_BACKSPACE_");
                    else {
                        char str[2] = {data, '\0'};
                        sendUARTMessage(str);
                    }
                }
                sendUARTMessage("\n\r");
                return 1;
            }
        }
    }
    return 0;
}

int validate_poke()
{
    if (strncmp(command, "poke", 4) == 0)
    {
        int address, bytes;
        char data[100];
        if (sscanf(command, "poke %x %d %99[^\n]", &address, &bytes, data) == 3)
        {
            if (address >= 0x20000000 && address <= 0x20008000 &&
                bytes > 0 && bytes <= 100 && bytes <= (int)strlen(data))
            {
                for (int i = 0; i < bytes; i++) {
                    *((char*)address + i) = data[i];
                }
                sendUARTMessage("Data written to given address.\n\r");
                return 1;
            }
        }
    }
    return 0;
}

void run_command()
{
    if (!validate_color() && !validate_blink_command() && !validate_stopwatch_command() &&
        !validate_peek() && !validate_poke())
    {
        sendUARTMessage("GIVEN COMMAND IS INVALID. VALID COMMANDS:\n\r"
                       "[led <name>]\n\r"
                       "[led off]\n\r"
                       "[led blink <color> <time>]\n\r"
                       "[swt <start/stop/pause/resume>]\n\r"
                       "[peek <address> <bytes>]\n\r"
                       "[poke <address> <bytes> <data>]\n\r");
    }
    clearBuffer();
}
