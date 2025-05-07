/* AASHEIK SARAN FROM IISC BANGALORE */


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "./inc/tm4c123gh6pm.h"
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/uart.h>
#include <driverlib/interrupt.h>
#include <driverlib/adc.h>

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

#define UART_BAUD_RATE 115200
#define SYSTICK_FREQ 2000
#define ADC_MAX_VALUE 4095
#define SAMPLE_DELAY_MS 100

const uint8_t SEG_DIGITS[] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

const uint8_t LED_COLORS[] = {GREEN, BLUE, CYAN, RED, YELLOW, MAGENTA, WHITE};
const uint16_t BLINK_SPEEDS[] = {1000, 500, 333, 250, 200, 166, 142, 125};

#define COLOR_COUNT (sizeof(LED_COLORS) / sizeof(LED_COLORS[0]))
#define SPEED_COUNT (sizeof(BLINK_SPEEDS) / sizeof(BLINK_SPEEDS[0]))

volatile uint32_t sysTickCount = 0;
volatile uint32_t adc_value = 0;
volatile uint32_t toggleInterval = 500;
volatile uint32_t lastToggle = 0;
volatile bool ledState = false;
volatile uint8_t colorIndex = 0;
volatile uint8_t blinkSpeedIndex = 0;
volatile bool sw1Pressed = false;
volatile bool usePotentiometer = true;
volatile uint8_t displayIndex = 0;

void initSystem(void);
void delayMilliseconds(int n);
void sendUARTMessage(const char *message);
void SysTick_Handler(void);
void printADCandBlinkRate(uint32_t adc_val, uint32_t blink_rate);
uint32_t ADC0_Read(void);
uint32_t mapADCtoBlinkRate(uint32_t adc_val);
void ChangeLEDColor(void);
void ChangeBlinkSpeed(void);
void DisplayInfo(void);

int main(void)
{
    initSystem();
    sendUARTMessage("\n\rRGB LED Control Initialized!\n\r");
    sendUARTMessage("SW1/PC5: Blink Speed (1-8 Hz), SW2/PC4: Color, Potentiometer: Blink Speed\n\r");

    uint32_t adcValue;
    uint32_t lastSwitchCheck = 0;

    while (1)
    {
        adcValue = ADC0_Read();
        uint32_t potBlinkRate = mapADCtoBlinkRate(adcValue);

        if (sw1Pressed)
        {
            ChangeBlinkSpeed();
            usePotentiometer = false;
            sw1Pressed = false;
            delayMilliseconds(500);
        }
        else if (usePotentiometer)
        {
            blinkSpeedIndex = potBlinkRate - 1;
        }

        toggleInterval = BLINK_SPEEDS[blinkSpeedIndex] / 2;

        if ((sysTickCount - lastSwitchCheck) >= (SAMPLE_DELAY_MS * 2))
        {
            if (SW1_PRESSED || PC5_PRESSED)
            {
                sw1Pressed = true;
                while (SW1_PRESSED || PC5_PRESSED);
            }
            else if (!(SW1_PRESSED || PC5_PRESSED))
            {
                usePotentiometer = true;
            }

            if (SW2_PRESSED || PC4_PRESSED)
            {
                ChangeLEDColor();
                while (SW2_PRESSED || PC4_PRESSED);
            }

            lastSwitchCheck = sysTickCount;
        }

        printADCandBlinkRate(adcValue, blinkSpeedIndex + 1);

        delayMilliseconds(SAMPLE_DELAY_MS);
    }
}

void initSystem(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ);
    while(!SysCtlClockGet());

    SYSCTL_RCGCADC_R |= 0x01;
    SYSCTL_RCGCGPIO_R |= 0x10;
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));

    GPIO_PORTE_DIR_R &= ~0x08;
    GPIO_PORTE_AFSEL_R |= 0x08;
    GPIO_PORTE_DEN_R &= ~0x08;
    GPIO_PORTE_AMSEL_R |= 0x08;

    ADC0_ACTSS_R &= ~0x08;
    ADC0_EMUX_R &= ~0xF000;
    ADC0_SSMUX3_R = 0;
    ADC0_SSCTL3_R |= 0x06;
    ADC0_ACTSS_R |= 0x08;

    SYSCTL_RCGCGPIO_R |= 0x20;
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));

    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R = 0x1F;
    GPIO_PORTF_AMSEL_R = 0x00;
    GPIO_PORTF_PCTL_R = 0x00000000;
    GPIO_PORTF_DIR_R = 0x0E;
    GPIO_PORTF_AFSEL_R = 0x00;
    GPIO_PORTF_PUR_R = 0x11;
    GPIO_PORTF_DEN_R = 0x1F;

    SYSCTL_RCGCGPIO_R |= (1 << 2);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC));

    GPIO_PORTC_LOCK_R = 0x4C4F434B;
    GPIO_PORTC_CR_R |= (1 << 4) | (1 << 5);
    GPIO_PORTC_DIR_R &= ~((1 << 4) | (1 << 5));
    GPIO_PORTC_DEN_R |= (1 << 4) | (1 << 5);
    GPIO_PORTC_PUR_R |= (1 << 4) | (1 << 5);

    GPIO_PORTE_DIR_R |= (1 << 0);
    GPIO_PORTE_DEN_R |= (1 << 0);

    SYSCTL_RCGCGPIO_R |= 0x03;
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));

    GPIO_PORTB_DIR_R = 0xFF;
    GPIO_PORTB_DEN_R = 0xFF;
    GPIO_PORTA_DIR_R |= (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);
    GPIO_PORTA_DEN_R |= (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), UART_BAUD_RATE,
                       (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    UARTFIFOEnable(UART0_BASE);
    UARTEnable(UART0_BASE);

    SysTickPeriodSet(SysCtlClockGet() / SYSTICK_FREQ);
    SysTickIntEnable();
    SysTickEnable();

    IntMasterEnable();
}

void delayMilliseconds(int n)
{
    uint32_t startCount = sysTickCount;
    while ((sysTickCount - startCount) < (uint32_t)(n * 2));
}

void SysTick_Handler(void)
{
    sysTickCount++;

    static uint32_t lastDisplayToggle = 0;
    if ((sysTickCount - lastDisplayToggle) >= 1)
    {
        displayIndex = (displayIndex + 1) % 4;
        DisplayInfo();
        lastDisplayToggle = sysTickCount;
    }

    if ((sysTickCount - lastToggle) >= (toggleInterval * 2))
    {
        ledState = !ledState;
        if (ledState)
            LED_ON(LED_COLORS[colorIndex]);
        else
            LED_OFF();
        lastToggle = sysTickCount;
    }
}

void sendUARTMessage(const char *message)
{
    while (*message) {
        while(UARTSpaceAvail(UART0_BASE) == 0);
        UARTCharPut(UART0_BASE, *message++);
    }
}

uint32_t ADC0_Read(void)
{
    ADC0_PSSI_R = 0x08;
    while ((ADC0_RIS_R & 0x08) == 0);
    uint32_t result = ADC0_SSFIFO3_R;
    ADC0_ISC_R = 0x08;
    return result;
}

uint32_t mapADCtoBlinkRate(uint32_t adc_val)
{
    uint32_t interval = adc_val / 512;
    if (interval > 7) interval = 7;
    return (interval + 1);
}

void printADCandBlinkRate(uint32_t adc_val, uint32_t blink_rate)
{
    char str[64];
    int len = snprintf(str, sizeof(str),
                      "ADC: %lu Blink Rate: %lu Hz Color: %lu\r\n",
                      (unsigned long)adc_val, (unsigned long)blink_rate, (unsigned long)(colorIndex + 1));
    if (len > 0 && len < (int)sizeof(str)) {
        sendUARTMessage(str);
    }
}

void ChangeLEDColor(void)
{
    colorIndex = (colorIndex + 1) % COLOR_COUNT;
}

void ChangeBlinkSpeed(void)
{
    blinkSpeedIndex = (blinkSpeedIndex + 1) % SPEED_COUNT;
}

void DisplayInfo(void)
{
    uint8_t displayData[4] = {0x00, 0x00, 0x00, 0x00};
    displayData[1] = SEG_DIGITS[colorIndex + 1];
    displayData[3] = SEG_DIGITS[blinkSpeedIndex + 1];

    uint8_t nextDigit = displayData[displayIndex];
    GPIO_PORTA_DATA_R &= ~((1 << 4) | (1 << 5) | (1 << 6) | (1 << 7));
    GPIO_PORTB_DATA_R = nextDigit;
    __asm("NOP"); __asm("NOP");
    GPIO_PORTA_DATA_R |= (1 << (7 - displayIndex));
}
