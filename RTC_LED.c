/* AASHEIK SARAN IISC,BANGALORE */


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"

// LCD Definitions
#define LCD_DATA GPIO_PORTB_BASE
#define LCD_CTRL GPIO_PORTA_BASE
#define RS GPIO_PIN_6
#define EN GPIO_PIN_7

// RTC Definitions
#define DS1307_ADDR 0x68            // DS1307 I2C address
#define I2C_START_ADDR 0x08         // Starting address for general data storage

// Function Prototypes - LCD
void delay_ms(int ms);
void delay_us(int us);
void LCD_Cmd(uint8_t cmd);
void LCD_Write_Char(char c);
void LCD_Write_String(char *str);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_init(void);
void LCD_Write_Byte(uint8_t data, bool isData);

// Function Prototypes - I2C and RTC
void I2C2_init(void);
char I2C2_byteWrite(int slaveAddr, char memAddr, char data);
char I2C2_read(int slaveAddr, char memAddr, int byteCount, char* data);
void RTC_SetDateTime(uint8_t day, uint8_t month, uint8_t year, uint8_t hours, uint8_t minutes, uint8_t seconds);
void RTC_GetDateTime(uint8_t *day, uint8_t *month, uint8_t *year, uint8_t *hours, uint8_t *minutes, uint8_t *seconds);
uint8_t BCD_to_DEC(uint8_t bcd);
uint8_t DEC_to_BCD(uint8_t dec);

int main(void) {
    uint8_t hours, minutes, seconds;
    uint8_t day, month, year;
    char dateStr[16];
    char timeStr[16];

    // Set clock to 50 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // Enable ports
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);  // For LCD control
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);  // For LCD data
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);  // For I2C2
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);  // For LED

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));

    // Set PA6, PA7 as output (RS, EN)
    GPIOPinTypeGPIOOutput(LCD_CTRL, RS | EN);

    // Set PB0–PB7 as output (D0–D7)
    GPIOPinTypeGPIOOutput(LCD_DATA, 0xFF);

    // PF1 for blinking LED (red)
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);

    // Initialize LCD
    LCD_init();
    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_Write_String("Initializing...");

    // Initialize I2C2 for RTC
    I2C2_init();

    // Set initial date time (April 9, 2025 11:00:00)
    RTC_SetDateTime(9, 4, 25, 11, 0, 0);

    LCD_Clear();

    while (1) {
        // Get date and time from RTC
        RTC_GetDateTime(&day, &month, &year, &hours, &minutes, &seconds);

        // Format date (DD/MM/YY)
        sprintf(dateStr, "Date: %02d/%02d/%02d", day, month, year);

        // Format time (HH:MM:SS)
        sprintf(timeStr, "Time: %02d:%02d:%02d", hours, minutes, seconds);

        // Display date on first line
        LCD_SetCursor(0, 0);
        LCD_Write_String(dateStr);

        // Display time on second line
        LCD_SetCursor(1, 0);
        LCD_Write_String(timeStr);

        // Toggle LED to indicate activity
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);
        delay_ms(500);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
        delay_ms(500);
    }
}

// LCD Functions
void LCD_init(void) {
    delay_ms(50);              // LCD power on delay
    LCD_Cmd(0x38);             // Function Set: 8-bit, 2-line, 5x8 dots
    delay_ms(5);
    LCD_Cmd(0x38);
    delay_us(100);
    LCD_Cmd(0x38);
    LCD_Cmd(0x0C);             // Display ON, Cursor OFF
    LCD_Cmd(0x01);             // Clear display
    delay_ms(2);
    LCD_Cmd(0x06);             // Entry Mode Set: increment cursor
}

void LCD_Cmd(uint8_t cmd) {
    LCD_Write_Byte(cmd, false);
    if (cmd == 0x01 || cmd == 0x02)
        delay_ms(2);
    else
        delay_us(40);
}

void LCD_Write_Char(char c) {
    LCD_Write_Byte(c, true);
    delay_us(40);
}

void LCD_Write_String(char *str) {
    while(*str) {
        LCD_Write_Char(*str++);
    }
}

void LCD_Write_Byte(uint8_t data, bool isData) {
    // RS: 1 for data, 0 for command
    GPIOPinWrite(LCD_CTRL, RS, isData ? RS : 0);

    // Send data to Port B
    GPIOPinWrite(LCD_DATA, 0xFF, data);

    // EN pulse
    GPIOPinWrite(LCD_CTRL, EN, EN);
    delay_us(1);
    GPIOPinWrite(LCD_CTRL, EN, 0);
}

void LCD_Clear(void) {
    LCD_Cmd(0x01);
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t address;

    // Calculate DDRAM address based on row and column
    if (row == 0)
        address = 0x80 + col;  // First line starts at 0x80
    else
        address = 0xC0 + col;  // Second line starts at 0xC0

    LCD_Cmd(address);
}

void delay_ms(int ms) {
    SysCtlDelay((SysCtlClockGet() / 3000) * ms);
}

void delay_us(int us) {
    SysCtlDelay((SysCtlClockGet() / 3000000) * us);
}

// I2C and RTC Functions
void I2C2_init(void) {
    // Enable I2C2 peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);

    // Configure GPIO pins for I2C2
    GPIOPinConfigure(GPIO_PE4_I2C2SCL);
    GPIOPinConfigure(GPIO_PE5_I2C2SDA);

    // Configure pins for I2C function
    GPIOPinTypeI2CSCL(GPIO_PORTE_BASE, GPIO_PIN_4);
    GPIOPinTypeI2C(GPIO_PORTE_BASE, GPIO_PIN_5);

    // Initialize I2C2 master
    I2C2_MCR_R = 0x0010;      // Master mode
    I2C2_MTPR_R = 0x09;       // 100kHz @ 50MHz clock

    // Wait for I2C2 to be ready
    while(I2C2_MCS_R & 0x01);
}

/* Wait until I2C master is not busy and return error code */
static int I2C_wait_till_done(void) {
    while(I2C2_MCS_R & 1)
        ;  // Wait until I2C master is not busy
    return I2C2_MCS_R & 0xE;  // Return I2C error code
}

/* Write one byte only */
char I2C2_byteWrite(int slaveAddr, char memAddr, char data) {
    char error;

    // Send slave address and starting address
    I2C2_MSA_R = slaveAddr << 1;
    I2C2_MDR_R = memAddr;
    I2C2_MCS_R = 3;  // S-(saddr+w)-ACK-maddr-ACK
    error = I2C_wait_till_done();
    if(error) return error;

    // Send data
    I2C2_MDR_R = data;
    I2C2_MCS_R = 5;  // -data-ACK-P
    error = I2C_wait_till_done();

    // Wait for bus not busy
    while(I2C2_MCS_R & 0x40)
        ;

    return I2C2_MCS_R & 0xE;
}

/* Read memory */
char I2C2_read(int slaveAddr, char memAddr, int byteCount, char* data) {
    char error;

    if (byteCount <= 0)
        return -1;  // No read was performed

    // Send slave address and starting address
    I2C2_MSA_R = slaveAddr << 1;
    I2C2_MDR_R = memAddr;
    I2C2_MCS_R = 3;  // S-(saddr+w)-ACK-maddr-ACK
    error = I2C_wait_till_done();
    if(error) return error;

    // Set slave address for read
    I2C2_MSA_R = (slaveAddr << 1) + 1;  // R-(saddr+r)-ACK

    if(byteCount == 1) {
        I2C2_MCS_R = 7;  // -data-NACK-P
    } else {
        I2C2_MCS_R = 0xB;  // -data-ACK-
    }

    error = I2C_wait_till_done();
    if(error) return error;

    *data++ = I2C2_MDR_R;  // Store received data

    if(--byteCount == 0) {
        while(I2C2_MCS_R & 0x40)
            ;  // Wait until bus not busy
        return 0;
    }

    // Read remaining bytes
    while(byteCount > 1) {
        I2C2_MCS_R = 9;  // -data-ACK-
        error = I2C_wait_till_done();
        if(error) return error;

        *data++ = I2C2_MDR_R;
        byteCount--;
    }

    // Read last byte
    I2C2_MCS_R = 5;  // -data-NACK-P
    error = I2C_wait_till_done();
    *data = I2C2_MDR_R;

    // Wait until bus not busy
    while(I2C2_MCS_R & 0x40)
        ;

    return 0;
}

// Set date and time in the DS1307 RTC
void RTC_SetDateTime(uint8_t day, uint8_t month, uint8_t year, uint8_t hours, uint8_t minutes, uint8_t seconds) {
    // Convert to BCD format
    uint8_t bcdSeconds = DEC_to_BCD(seconds);
    uint8_t bcdMinutes = DEC_to_BCD(minutes);
    uint8_t bcdHours = DEC_to_BCD(hours);
    uint8_t bcdDay = DEC_to_BCD(day);
    uint8_t bcdMonth = DEC_to_BCD(month);
    uint8_t bcdYear = DEC_to_BCD(year);

    // Write seconds (address 0)
    I2C2_byteWrite(DS1307_ADDR, 0, bcdSeconds);

    // Write minutes (address 1)
    I2C2_byteWrite(DS1307_ADDR, 1, bcdMinutes);

    // Write hours (address 2)
    I2C2_byteWrite(DS1307_ADDR, 2, bcdHours);

    // Write day of week (address 3) - not used, set to 1
    I2C2_byteWrite(DS1307_ADDR, 3, 1);

    // Write day of month (address 4)
    I2C2_byteWrite(DS1307_ADDR, 4, bcdDay);

    // Write month (address 5)
    I2C2_byteWrite(DS1307_ADDR, 5, bcdMonth);

    // Write year (address 6)
    I2C2_byteWrite(DS1307_ADDR, 6, bcdYear);
}

// Get date and time from the DS1307 RTC
void RTC_GetDateTime(uint8_t *day, uint8_t *month, uint8_t *year, uint8_t *hours, uint8_t *minutes, uint8_t *seconds) {
    char timeData[7];

    // Read all time registers in one go
    I2C2_read(DS1307_ADDR, 0, 7, timeData);

    // Convert from BCD to decimal
    *seconds = BCD_to_DEC(timeData[0] & 0x7F);  // Mask out CH bit
    *minutes = BCD_to_DEC(timeData[1]);
    *hours = BCD_to_DEC(timeData[2] & 0x3F);  // Mask out 24-hour mode bits
    // timeData[3] is day of week - we don't need it
    *day = BCD_to_DEC(timeData[4]);
    *month = BCD_to_DEC(timeData[5]);
    *year = BCD_to_DEC(timeData[6]);
}

// Convert BCD to decimal
uint8_t BCD_to_DEC(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// Convert decimal to BCD
uint8_t DEC_to_BCD(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}
