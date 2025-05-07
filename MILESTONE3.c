
/* AASHEIK SARAN IISC, BANGALORE */

#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "inc/tm4c123gh6pm.h"
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/uart.h>
#include <driverlib/interrupt.h>

#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3
#define LED_OFF   0x00

#define COL1 GPIO_PIN_4
#define COL2 GPIO_PIN_5
#define COL3 GPIO_PIN_6
#define COL4 GPIO_PIN_7

#define BUFFER_SIZE 30
#define UART_BAUD_RATE 115200
#define NEW_LINE '\n'
#define SPACE ' '
#define NULL_CHAR '\0'
#define ENTER '\r'
#define BACKSPACE '\b'

typedef struct {
    int *data;
    int size;
    int capacity;
    bool isCreated;
} PriorityQueue;

PriorityQueue queue;

char command[BUFFER_SIZE] = {NULL_CHAR};
int commandIndex = 0;
volatile bool uartCommandReady = false;

void UART_Init(void);
void sendUARTMessage(const char *message);
void UART_SendChar(char c);
void LED_Init(void);
void Keypad_Init(void);
char Keypad_Scan(void);
void delay_time(uint32_t ms);
void LED_Update(void);
void Queue_Create(int size);
void Queue_Delete(void);
void Queue_Insert(int num);
int Queue_Get(void);
void Queue_Display(void);
void swap(int *a, int *b);
void bubbleSort(int arr[], int n);
void clearBuffer(void);
void verification_Command(void);
int validate_createq(void);
int validate_deleteq(void);
int validate_putnum(void);
int validate_getnum(void);
void run_command(void);
void UART0_Handler(void);

void UART_Init(void) {
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
}

void sendUARTMessage(const char *message) {
    while (*message) {
        UARTCharPut(UART0_BASE, *message++);
    }
}

void UART_SendChar(char c) {
    UARTCharPut(UART0_BASE, c);
}

void UART0_Handler(void) {
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
            uartCommandReady = true;
        } else if (receivedChar == BACKSPACE || receivedChar == 0x7F) {
            if (commandIndex > 0) {
                commandIndex--;
                command[commandIndex] = NULL_CHAR;
                UARTCharPut(UART0_BASE, BACKSPACE);
                UARTCharPut(UART0_BASE, SPACE);
                UARTCharPut(UART0_BASE, BACKSPACE);
            }
        } else if (isalpha(receivedChar) || isdigit(receivedChar) || receivedChar == SPACE) {
            if (commandIndex < BUFFER_SIZE - 1) {
                command[commandIndex++] = tolower(receivedChar);
                UARTCharPut(UART0_BASE, receivedChar);
            } else {
                UARTCharPut(UART0_BASE, '\a');
            }
        }
    }
}

void clearBuffer(void) {
    memset(command, NULL_CHAR, BUFFER_SIZE);
    commandIndex = 0;
}

void verification_Command(void) {
    int i, j = 0;
    char temp[BUFFER_SIZE] = {0};
    for (i = 0; command[i] != NULL_CHAR && i < BUFFER_SIZE; i++) {
        if (isalpha(command[i]) || isdigit(command[i]) || command[i] == SPACE) {
            temp[j++] = command[i];
        }
    }
    temp[j] = NULL_CHAR;
    strncpy(command, temp, BUFFER_SIZE - 1);
    command[BUFFER_SIZE - 1] = NULL_CHAR;
}

void Keypad_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, COL1 | COL2 | COL3 | COL4);
    GPIOPadConfigSet(GPIO_PORTC_BASE, COL1 | COL2 | COL3 | COL4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

char Keypad_Scan(void) {
    uint8_t row;
    char key = 0xFF;
    GPIO_PORTE_DATA_R = 0b00001111;
    delay_time(10);

    for (row = 0; row < 4; row++) {
        GPIO_PORTE_DATA_R = ~(1 << row) & 0b00001111;
        delay_time(10);

        uint8_t col = GPIO_PORTC_DATA_R & 0b11110000;

        if (col != 0b11110000) {
            if (row == 0) {
                if (col == (0b11110000 & ~COL1)) return '1';
                if (col == (0b11110000 & ~COL2)) return '2';
                if (col == (0b11110000 & ~COL3)) return '3';
                if (col == (0b11110000 & ~COL4)) return 'A';
            } else if (row == 1) {
                if (col == (0b11110000 & ~COL1)) return '4';
                if (col == (0b11110000 & ~COL2)) return '5';
                if (col == (0b11110000 & ~COL3)) return '6';
                if (col == (0b11110000 & ~COL4)) return 'B';
            } else if (row == 2) {
                if (col == (0b11110000 & ~COL1)) return '7';
                if (col == (0b11110000 & ~COL2)) return '8';
                if (col == (0b11110000 & ~COL3)) return '9';
                if (col == (0b11110000 & ~COL4)) return 'C';
            } else if (row == 3) {
                if (col == (0b11110000 & ~COL1)) return '*';
                if (col == (0b11110000 & ~COL2)) return '0';
                if (col == (0b11110000 & ~COL4)) return 'D';
                if (col == (0b11110000 & ~COL3)) return '#';
            }
        }
    }
    GPIO_PORTE_DATA_R = 0b00001111;
    return 0xFF;
}

void delay_time(uint32_t ms) {
    SysCtlDelay((SysCtlClockGet() / 3000) * ms);
}

void LED_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED | BLUE_LED | GREEN_LED);
    GPIOPinWrite(GPIO_PORTF_BASE, RED_LED | BLUE_LED | GREEN_LED, 0);
}

void LED_Update(void) {
    static bool redState = false;
    static uint32_t toggleCounter = 0;
    const uint32_t toggleInterval = 1000;
    const uint32_t delayPerCall = 50;
    const uint32_t ticksPerToggle = toggleInterval / delayPerCall;
    GPIOPinWrite(GPIO_PORTF_BASE, RED_LED | BLUE_LED | GREEN_LED, 0);

    if (!queue.isCreated || queue.size == 0) {
        GPIOPinWrite(GPIO_PORTF_BASE, GREEN_LED, GREEN_LED);
        toggleCounter = 0;
        redState = false;
    } else if (queue.size == queue.capacity) {
        toggleCounter++;
        if (toggleCounter >= ticksPerToggle) {
            redState = !redState;
            toggleCounter = 0;
        }
        if (redState) {
            GPIOPinWrite(GPIO_PORTF_BASE, RED_LED, RED_LED);
        } else {
            GPIOPinWrite(GPIO_PORTF_BASE, RED_LED, 0);
        }
    } else {
        GPIOPinWrite(GPIO_PORTF_BASE, BLUE_LED, BLUE_LED);
        toggleCounter = 0;
        redState = false;
    }
}

void Queue_Create(int size) {
    if (queue.isCreated) {
        sendUARTMessage("Queue already created!\r\n");
        return;
    }
    queue.data = (int *)malloc(size * sizeof(int));
    queue.capacity = size;
    queue.size = 0;
    queue.isCreated = true;
    sendUARTMessage("Queue created with size ");
    UART_SendChar('0' + size);
    sendUARTMessage("\r\n");
    LED_Update();
}

void Queue_Delete(void) {
    if (!queue.isCreated) {
        sendUARTMessage("Queue not defined\r\n");
        return;
    }
    free(queue.data);
    queue.data = NULL;
    queue.size = 0;
    queue.capacity = 0;
    queue.isCreated = false;
    sendUARTMessage("Queue deleted\r\n");
    LED_Update();
}

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void bubbleSort(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                swap(&arr[j], &arr[j + 1]);
            }
        }
    }
}

void Queue_Insert(int num) {
    if (!queue.isCreated) {
        sendUARTMessage("Queue not defined\r\n");
        return;
    }
    if (queue.size == queue.capacity) {
        sendUARTMessage("Queue is full\r\n");
        return;
    }
    queue.data[queue.size] = num;
    queue.size++;
    bubbleSort(queue.data, queue.size);
    sendUARTMessage("Inserted ");
    UART_SendChar('0' + num);
    sendUARTMessage("\r\n");
    Queue_Display();
    LED_Update();
}

int Queue_Get(void) {
    if (!queue.isCreated) {
        sendUARTMessage("Queue not defined\r\n");
        return -1;
    }
    if (queue.size == 0) {
        sendUARTMessage("Queue is empty\r\n");
        return -1;
    }
    int num = queue.data[0];
    for (int i = 0; i < queue.size - 1; i++) {
        queue.data[i] = queue.data[i + 1];
    }
    queue.size--;
    sendUARTMessage("Removed ");
    UART_SendChar('0' + num);
    sendUARTMessage("\r\n");
    Queue_Display();
    LED_Update();
    return num;
}

void Queue_Display(void) {
    if (!queue.isCreated) {
        sendUARTMessage("Queue not defined\r\n");
        return;
    }
    sendUARTMessage("Queue: ");
    for (int i = 0; i < queue.size; i++) {
        UART_SendChar('0' + queue.data[i]);
        UART_SendChar(' ');
    }
    sendUARTMessage("\r\n");
}

int validate_createq(void) {
    int size;
    if (sscanf(command, "createq %d", &size) == 1) {
        if (size >= 1 && size <= 9) {
            Queue_Create(size);
            return 1;
        } else {
            sendUARTMessage("Invalid size (must be 1-9)\r\n");
            return 1;
        }
    }
    return 0;
}

int validate_deleteq(void) {
    if (strcmp(command, "deleteq") == 0) {
        Queue_Delete();
        return 1;
    }
    return 0;
}

int validate_putnum(void) {
    int num;
    if (sscanf(command, "putnum %d", &num) == 1) {
        if (num >= 1 && num <= 9) {
            Queue_Insert(num);
            return 1;
        } else {
            sendUARTMessage("Invalid number (must be 1-9)\r\n");
            return 1;
        }
    }
    return 0;
}

int validate_getnum(void) {
    if (strcmp(command, "getnum") == 0) {
        Queue_Get();
        return 1;
    }
    return 0;
}

void run_command(void) {
    if (!validate_createq() && !validate_deleteq() && !validate_putnum() && !validate_getnum()) {
        sendUARTMessage("INVALID COMMAND. VALID COMMANDS:\r\n"
                        "[createq <size>]\r\n"
                        "[deleteq]\r\n"
                        "[putnum <number>]\r\n"
                        "[getnum]\r\n");
    }
    clearBuffer();
    uartCommandReady = false;
}

int main(void) {
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    UART_Init();
    LED_Init();
    Keypad_Init();
    IntMasterEnable();

    queue.isCreated = false;
    sendUARTMessage("Priority Queue Application\r\n");
    sendUARTMessage("UART Commands: [createq <size>] [deleteq] [putnum <number>] [getnum]\r\n");
    sendUARTMessage("Keypad: A=Create, B=Delete, C=Insert, D=Get, Numbers for size/value\r\n");

    char lastKey = 0xFF;
    bool waitingForNumber = false;
    char operation = 0;

    while (1) {
        if (uartCommandReady) {
            run_command();
        }

        char key = Keypad_Scan();
        if (key != 0xFF && key != lastKey) {
            sendUARTMessage("Key pressed: ");
            UART_SendChar(key);
            sendUARTMessage("\r\n");

            if (key >= '0' && key <= '9') {
                int num = key - '0';
                if (waitingForNumber) {
                    if (operation == 'A') {
                        Queue_Create(num);
                    } else if (operation == 'C') {
                        Queue_Insert(num);
                    }
                    waitingForNumber = false;
                    operation = 0;
                }
            } else if (key == 'A' || key == 'B' || key == 'C' || key == 'D') {
                if (key == 'A') {
                    sendUARTMessage("Enter queue size (1-9): ");
                    waitingForNumber = true;
                    operation = 'A';
                } else if (key == 'B') {
                    Queue_Delete();
                } else if (key == 'C') {
                    sendUARTMessage("Enter number to insert (1-9): ");
                    waitingForNumber = true;
                    operation = 'C';
                } else if (key == 'D') {
                    Queue_Get();
                }
            }
        }
        lastKey = key;
        LED_Update();
        delay_time(50);
    }
}
