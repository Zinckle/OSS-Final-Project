#pragma once
#include <Arduino.h>

// Defines Variables
#define RS DDB1 // Register Select is connected to PB4
#define EN DDB0 // Enable pin is connected to PB2

#define D4 DDD4 // D0 is connected to PD4
#define D5 DDD5 // D0 is connected to PD5
#define D6 DDD6 // D0 is connected to PD6
#define D7 DDD7 // D0 is connected to PD7

// Functions Prototypes
void delayMs(uint16_t ms);

// initialize the library with the numbers of the interface pins
void LCD_nibble_write(uint8_t data, uint8_t control);
void LCD_command(uint8_t command);
void LCD_data(uint8_t data);
void LCD_init(void);
void PORTS_init(void);
void LCD_string(char *str);