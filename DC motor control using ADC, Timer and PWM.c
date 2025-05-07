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
#include <driverlib/interrupt.h>
#include <driverlib/systick.h>
void sendUARTMessage(const char *message);

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

#define LED_GREEN  0x08
#define LED_BLUE   0x04
#define LED_CYAN   0x0C
#define LED_RED    0x02
#define LED_YELLOW 0x0A
#define LED_MAGENTA 0x06
#define LED_WHITE  0x0E
#define LED_OFF    0x00

#define BUFFER_SIZE 30
#define UART_BAUD_RATE 115200
#define SPACE ' '
#define NULL_CHAR '\0'
#define ENTER '\r'
#define BACKSPACE '\b'
#define ADC_DISPLAY_TIME 5 // seconds to display ADC value

char command[BUFFER_SIZE] = {NULL_CHAR};
int commandIndex = 0;

int segment_number[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
int time=0; //max 99
int adc_value = 0;
int prev_adc_value = 0;
int adc_display_counter = 0;
bool display_adc = false;

bool MOTOR_RUNINNING_SATUS_FLAG=false;
bool TIMEOUT_FLAG=false;
bool PLAY_PAUSE_FLAG=false;
bool START_STOP_FLAG=false;

int LED_status_flag=0;
uint32_t colour=0x00;

void toggler(void)
{
    switch(LED_status_flag)
    {
    case(1):
    {
        LED_status_flag=0;
        GPIO_PORTF_DATA_R=((GPIO_PORTF_DATA_R&0b11110001));
        break;
    }
    case(0):
    {
        LED_status_flag=1;
        GPIO_PORTF_DATA_R=((GPIO_PORTF_DATA_R&0b11110001)|0b1000);
        break;
    }
    }
}

void GPIOF_Handler(void)
{
    if (GPIO_PORTF_RIS_R & 0x10)
    {  // Check if PF4 caused the interrupt
        GPIO_PORTF_ICR_R = 0x10;    // Clear interrupt flag

        // Action for PF4 (SW1) START/STOP
        if (START_STOP_FLAG==false)
        {
            if (TIMEOUT_FLAG==false)
            {
                START_STOP_FLAG=true;//start
                PLAY_PAUSE_FLAG=true;
            }
        }
        else//stop
        {
            START_STOP_FLAG=false;
            PLAY_PAUSE_FLAG=false;
            time=0;
        }
    }

    if (GPIO_PORTF_RIS_R & 0x01)
    {  // Check if PF0 caused the interrupt
        GPIO_PORTF_ICR_R = 0x01;    // Clear interrupt flag

        // Action for PF0 (SW2) PLAY/PAUSE
        if(START_STOP_FLAG==true)
        {
            if (PLAY_PAUSE_FLAG==false) PLAY_PAUSE_FLAG=true;
            else PLAY_PAUSE_FLAG=false;
        }
    }
}

void SysTick_Handler(void)//systick 1 second handler
{
    // Handle ADC display timeout
    if(display_adc && adc_display_counter > 0)
    {
        adc_display_counter--;
        if(adc_display_counter == 0)
        {
            display_adc = false;
        }
    }

    if (PLAY_PAUSE_FLAG==true)
    {
        if (time>0)
        {   MOTOR_RUNINNING_SATUS_FLAG=true;//motor is running
            time--;
            GPIO_PORTA_DATA_R |= (1<<2);//PA2 ON
            toggler();//blinking green
        }
        else if(time==0)
        {
            MOTOR_RUNINNING_SATUS_FLAG=false;
            PLAY_PAUSE_FLAG=false;
            START_STOP_FLAG=false;
            TIMEOUT_FLAG=true;
            GPIO_PORTA_DATA_R &= ~(1<<2);//PA2 OFF
            GPIO_PORTF_DATA_R &= ~(0xE);//OFF ALL LED
            GPIO_PORTF_DATA_R |=0b10;//RED LED
            sendUARTMessage("TIME OUT!");
        }
        else
        {
            sendUARTMessage("ERROR : ISSUE WITH TIME VALUE!");
        }
    }

    if (PLAY_PAUSE_FLAG==false && START_STOP_FLAG==true)
    {
        //blue led
        GPIO_PORTF_DATA_R &= ~(0xE);//OFF ALL LED
        GPIO_PORTF_DATA_R |=0b100;//PF2
    }
}

void delay(long in)//simple delay
{
    unsigned long i;
    for (i = 0; i < in; i++);
}

void sendUARTMessage(const char *message)
{
    while (*message) {
        UARTCharPut(UART0_BASE, *message++);
    }
    UARTCharPut(UART0_BASE, '\r');
    UARTCharPut(UART0_BASE, '\n');
}

void clearBuffer(void)
{
    memset(command, NULL_CHAR, BUFFER_SIZE);
    commandIndex = 0;
}

void verification_Command(void)
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

int stopwatch_set(int load_time)
{
    if (load_time<99)
    {
        time=load_time;
        TIMEOUT_FLAG=false;
        GPIO_PORTF_DATA_R &= ~(0xE);//OFF ALL LED
        GPIO_PORTF_DATA_R |=0b1000;//ON GREEN
        LED_status_flag=1;//means led is on
        return 1;
    }
    else {
        sendUARTMessage("WARNING! TIME IS NOT SET AS IT IS OUT OF BOUND EXPECTED VALUE <0:99>");
        return 0;
    }
}

void stopwatch_start(void)
{
    if (START_STOP_FLAG==false) {
        if (time<=0)
        {
            sendUARTMessage("WARNING! TIME IS NOT SET");
        }
        else{
            START_STOP_FLAG=true;
            PLAY_PAUSE_FLAG=true;
            sendUARTMessage("SWT STARTED");
        }
    }
    else sendUARTMessage("ALREDY SWT IS RUNNING");
}

void stopwatch_stop(void)
{
    if(START_STOP_FLAG==true){
        START_STOP_FLAG=false;
        PLAY_PAUSE_FLAG=false;
        time=0;
        sendUARTMessage("SWT STOPED");
    }
    else sendUARTMessage("ALREDY SWT IS STOPED");
}

void stopwatch_pause(void)
{
    if(START_STOP_FLAG==true)
    {
        if (PLAY_PAUSE_FLAG==true) {
            PLAY_PAUSE_FLAG=false;
            sendUARTMessage("SWT PAUSED");
        }
        else sendUARTMessage("ALREDY SWT IS PAUSED");
    }
    else sendUARTMessage("STATUS! SWT IS OFF");
}

void stopwatch_resume(void)
{
    if(START_STOP_FLAG==true)
    {
        if (PLAY_PAUSE_FLAG==false) {
            PLAY_PAUSE_FLAG=true;
            sendUARTMessage("SWT RESUMED");
        }
        else sendUARTMessage("ALREDY SWT IS RESUMED");
    }
    else sendUARTMessage("STATUS! SWT IS OFF");
}

int validate_stopwatch_command(void)
{
    char entered_command[8];
    int Set_time=0;
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
    if(sscanf(command,"swt set %d",&Set_time)==1)
    {
        return stopwatch_set(Set_time);
    }
    return 0;
}

void run_command(void)
{
    if (!validate_stopwatch_command())
    {
        sendUARTMessage("\n\rGIVEN COMMAND IS INVALID. VALID COMMANDS:\n\r"
                       "[swt <start/stop/pause/resume/set <time> >]\n\r");
    }
    clearBuffer();
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

void init_uart(void)
{
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
    IntMasterEnable();
}

void int_systick_timer(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    SysTickPeriodSet(16000000); // 1 second interrupt
    SysTickIntRegister(SysTick_Handler);
    SysTickIntEnable();
    SysTickEnable();
}

void Init_Port(void)
{
    volatile unsigned long delay;

    SYSCTL_RCGC2_R |= 0x00000037;
    delay = SYSCTL_RCGC2_R;

    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R = 0x1F;
    GPIO_PORTF_AMSEL_R = 0x00;
    GPIO_PORTA_AMSEL_R = 0x00;
    GPIO_PORTB_AMSEL_R = 0x00;
    GPIO_PORTC_AMSEL_R = 0x00;
    GPIO_PORTE_AMSEL_R = 0x00;
    GPIO_PORTF_PCTL_R = 0x00000000;
    GPIO_PORTF_DIR_R = 0x0E;
    GPIO_PORTA_DIR_R |= 0b11110000;
    GPIO_PORTB_DIR_R |= 0b11111111;
    GPIO_PORTC_DIR_R &= 0b11001111;
    GPIO_PORTE_DIR_R |= 0b00000001;
    GPIO_PORTC_PUR_R |= 0b00110000;
    GPIO_PORTF_PUR_R = 0x11;
    GPIO_PORTA_DEN_R |= 0b11110000;
    GPIO_PORTB_DEN_R |= 0b11111111;
    GPIO_PORTC_DEN_R |= 0b00110000;
    GPIO_PORTE_DEN_R |= 0b00000001;
    GPIO_PORTF_DEN_R = 0x1F;
    GPIO_PORTF_DATA_R &= ~(0xE);
    GPIO_PORTF_DATA_R |=0b10;
    GPIOIntTypeSet(GPIO_PORTF_BASE, 0x11, GPIO_FALLING_EDGE);
    GPIOIntEnable(GPIO_PORTF_BASE, 0x11);
    IntEnable(INT_GPIOF);
}

void init_pwm_adc(void)
{
    SYSCTL_RCGCPWM_R |= 1;
    SYSCTL_RCGCGPIO_R |= 0x10;
    SYSCTL_RCC_R &= ~0x00100000;
    SYSCTL_RCC_R |= 0x000E0000;

    GPIO_PORTE_AFSEL_R |= 0x10;
    GPIO_PORTE_PCTL_R &= ~0x000F0000;
    GPIO_PORTE_PCTL_R |= 0x00040000;
    GPIO_PORTE_DEN_R |= 0x10;

    PWM0_2_CTL_R &= ~(1<<0);
    PWM0_2_CTL_R &= ~(1<<1);
    PWM0_2_GENA_R = 0x0000008C;
    PWM0_2_LOAD_R = 5000;
    PWM0_2_CMPA_R = 4999;
    PWM0_2_CTL_R |= 1;
    PWM0_ENABLE_R |= 0x10;

    SYSCTL_RCGCGPIO_R |= 0x01;
    GPIO_PORTA_DIR_R |= (1<<3) | (1<<2);
    GPIO_PORTA_DEN_R |= (1<<3) | (1<<2);
    GPIO_PORTA_DATA_R &= ~(1<<3);
    GPIO_PORTA_DATA_R &= ~(1<<2);

    SYSCTL_RCGCADC_R |= 0x01;
    SYSCTL_RCGCGPIO_R |= 0x10;

    ADC0_ACTSS_R &= ~8;
    ADC0_EMUX_R &= ~0xF000;
    ADC0_SSMUX3_R = 0;
    GPIO_PORTE_AFSEL_R |= 8;
    GPIO_PORTE_DEN_R &= ~8;
    GPIO_PORTE_AMSEL_R |= 8;
    ADC0_SSCTL3_R |= 6;
    ADC0_ACTSS_R |= 8;
}

void display_value(int value)
{
    delay(50);
    GPIO_PORTA_DATA_R &= ~(0xF0);
    GPIO_PORTA_DATA_R |= 0b00100000; //pa5 s
    GPIO_PORTB_DATA_R=segment_number[value/10];
    delay(50);
    GPIO_PORTA_DATA_R &= ~(0xF0);
    GPIO_PORTA_DATA_R |= 0b00010000; //pa4 s
    GPIO_PORTB_DATA_R=segment_number[value%10];
    delay(50);
    GPIO_PORTA_DATA_R &= ~(0xF0);//off 7 segment
}

int main()
{
    Init_Port();
    int_systick_timer();
    init_uart();
    init_pwm_adc();

    int x = 4999;
    float temp;
    int temp2;
    volatile int result;
    int mapped_adc_value = 0;

    sendUARTMessage("\n\rSystem Initialized Successfully!\n\r");

    while (1)
    {
        // Read ADC value
        ADC0_PSSI_R |= 8;
        while((ADC0_RIS_R & 8) == 0);
        result = ADC0_SSFIFO3_R;
        ADC0_ISC_R = 8;

        // Map ADC value to 0-99 range
        mapped_adc_value = (int)((result / 4095.0) * 99.0);

        // Check if ADC value has changed significantly (to avoid display flicker)
        if(abs(mapped_adc_value - prev_adc_value) > 2) // 2% change threshold
        {
            prev_adc_value = mapped_adc_value;
            display_adc = true;
            adc_display_counter = ADC_DISPLAY_TIME;
            adc_value = mapped_adc_value;
        }

        // Update PWM based on ADC
        temp = ((result / 4095.0) * 4999.0);
        temp2 = (int)temp;
        x = temp2;
        PWM0_2_CMPA_R = x;

        // Display either ADC value or timer value
        if(display_adc)
        {
            display_value(adc_value);
        }
        else
        {
            display_value(time);
        }
    }
}
