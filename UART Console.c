/* AASHEIK SARAN FROM IISC, BANGALORE */


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

#define LED_GREEN  0x08
#define LED_BLUE   0x04
#define LED_CYAN   0x0C
#define LED_RED    0x02
#define LED_YELLOW 0x0A
#define LED_MAGENTA 0x06
#define LED_WHITE  0x0E
#define LED_OFF    0x00

#define MAX_BLINK_DELAY 1000
#define MIN_BLINK_DELAY 100
#define DEBOUNCE_TIME 50
#define BUFFER_SIZE 30
#define UART_BAUD_RATE 115200

#define SPACE ' '
#define NULL_CHAR '\0'
#define ENTER '\r'

#define SW1 (GPIO_PORTF_DATA_R & 0x10)
#define SW2 (GPIO_PORTF_DATA_R & 0x01)

uint8_t delayCounter;
int blink_delay = MAX_BLINK_DELAY;
int select_color = 0;
uint8_t currentColor = LED_GREEN;
char colors_list[7][8] = {"green", "blue", "cyan", "red", "yellow", "magenta", "white"};
char command[BUFFER_SIZE] = {NULL_CHAR};
int commandIndex = 0;

void delayMilliseconds(int n);
void update_color();
void processInput();
void processCommand();
void sendUARTMessage(char message[]);
void clearBuffer();
int validate_color();
int validate_freq();
void run_command();
void sanitizeCommand();

int main()
{
    // Enable GPIO Port F for LEDs & Switches
    SYSCTL_RCGC2_R |= 0x00000020;
    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R = 0x1F;
    GPIO_PORTF_DEN_R |= 0x1E;  // Enable LEDs and switches
    GPIO_PORTF_DIR_R |= 0x0E;  // Set LEDs as output
    GPIO_PORTF_DIR_R &= ~0x11; // Set switches as input
    GPIO_PORTF_PUR_R |= 0x11;  // Enable pull-ups for switches

    // Enable and configure UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), UART_BAUD_RATE,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    while (1)
    {
        GPIO_PORTF_DATA_R = currentColor;
        processInput();
        delayMilliseconds(blink_delay);
        processInput();
        GPIO_PORTF_DATA_R = LED_OFF;
        processInput();
        delayMilliseconds(blink_delay);
        processInput();
    }
}

void update_color()
{
    switch (select_color)
    {
        case 0: currentColor = LED_GREEN; break;
        case 1: currentColor = LED_BLUE; break;
        case 2: currentColor = LED_CYAN; break;
        case 3: currentColor = LED_RED; break;
        case 4: currentColor = LED_YELLOW; break;
        case 5: currentColor = LED_MAGENTA; break;
        case 6: currentColor = LED_WHITE; break;
    }
}

void delayMilliseconds(int n)
{
    int i, j;
    for (i = 0; i < n; i++) {
        for (j = 0; j < 3180; j++);
    }
}

void processInput()
{
    if (!SW1)
    {
        delayMilliseconds(DEBOUNCE_TIME);
        if (!SW1) // Confirm switch press
        {
            while (!SW1); // Wait for release
            select_color = (select_color + 1) % 7;
            update_color();
        }
    }

    if (!SW2)
    {
        delayMilliseconds(DEBOUNCE_TIME);
        if (!SW2) // Confirm switch press
        {
            while (!SW2); // Wait for release
            blink_delay = (blink_delay > MIN_BLINK_DELAY) ? blink_delay / 2 : MAX_BLINK_DELAY;
        }
    }

    processCommand();
}

void processCommand()
{
    if (commandIndex < BUFFER_SIZE - 1) {
        if (UARTCharsAvail(UART0_BASE)) {
            char receivedChar = UARTCharGet(UART0_BASE);

            if (receivedChar == ENTER) {
                command[commandIndex] = NULL_CHAR;
                sanitizeCommand();
                sendUARTMessage("\n\rReceived Command: ");
                sendUARTMessage(command);
                sendUARTMessage("\n\r");
                run_command();
                clearBuffer();
            }
            else if (isalpha(receivedChar) || isdigit(receivedChar) || receivedChar == SPACE)
            {
                UARTCharPut(UART0_BASE, receivedChar);
                command[commandIndex++] = tolower(receivedChar);
            }
        }
    } else {
        sendUARTMessage("\n\rInput buffer full. Command reset.\n\r");
        clearBuffer();
    }
}

void sanitizeCommand()
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

void sendUARTMessage(char* message)
{
    while (*message) {
        UARTCharPut(UART0_BASE, *(message++));
    }
}

void clearBuffer()
{
    memset(command, NULL_CHAR, BUFFER_SIZE);
    commandIndex = 0;
}

int validate_color()
{
    char entered_color[8];
    if (sscanf(command, "color %7s", entered_color) == 1)
    {
        for (int i = 0; i < 7; i++)
        {
            if (strcasecmp(entered_color, colors_list[i]) == 0)
            {
                select_color = i;
                update_color();
                return true;
            }
        }
    }
    return false;
}

int validate_freq()
{
    int blink_rate;
    if (sscanf(command, "blink %d", &blink_rate) == 1)
    {
        if (blink_rate > 0 && blink_rate <= 10) { // Ensuring a valid range
            int temp_blink_delay = MAX_BLINK_DELAY / blink_rate;
            if (temp_blink_delay >= MIN_BLINK_DELAY)
            {
                blink_delay = temp_blink_delay;
                return true;
            }
        }
    }
    return false;
}

void run_command()
{
    int validate = 0;
    switch (command[0])
    {
        case 'c':
            validate = validate_color();
            break;
        case 'b':
            validate = validate_freq();
            break;
        default:
            break;
    }

    if (!validate) {
        sendUARTMessage("\n\rInvalid command.\n\rCommand Help:\n\r");
        sendUARTMessage("[1] color <color name> (green, blue, cyan, red, yellow, magenta, white)\n\r");
        sendUARTMessage("[2] blink <blink rate> (1-10 Hz)\n\r");
    }
    clearBuffer();
}
