/*
 * myLCD_ST7032.cpp
 *
 * Created: 2021/02/12 0:49:42
 *  Author: JJ1MDY
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <util/twi.h>
#include "myI2C.h"

#define LCD_ADDR 0x7c		// LCD(ST7032) I2Cslave address

uint8_t I2C_LCD_SendControls(uint8_t data[], uint8_t num) {
	I2C_Start();

	I2C_Send(LCD_ADDR);
	if (TW_STATUS != TW_MT_SLA_ACK) return -1;

	I2C_Send(0x00); // Co:0(continuous) RS:0
	if (TW_STATUS != TW_MT_DATA_ACK) return -1;

	for (uint8_t i = 0; i < num; i++) {
		I2C_Send(data[i]);
		if (TW_STATUS != TW_MT_DATA_ACK) {
			return -1;
		}
		_delay_us(100);
	}

	I2C_Stop();
	
	_delay_ms(1);

	return 0;
}

uint8_t I2C_LCD_SendString(uint8_t addr, char *data, uint8_t num) {
	I2C_Start();

	I2C_Send(LCD_ADDR);
	if (TW_STATUS != TW_MT_SLA_ACK) return -1;

	I2C_Send(0x80); // Co:1(one shot) RS:0
	if (TW_STATUS != TW_MT_DATA_ACK) return -1;

	I2C_Send(0x80 | addr); // Set DDRAM address
	if (TW_STATUS != TW_MT_DATA_ACK) return -1;

	I2C_Send(0x40); // Co:0(continuous) RS:1
	if (TW_STATUS != TW_MT_DATA_ACK) return -1;

	for (uint8_t i = 0; i < num; i++) {
		I2C_Send(data[i]);
		if (TW_STATUS != TW_MT_DATA_ACK) return -1;
		_delay_us(100);
	}

	I2C_Stop();
	
	_delay_ms(1);

	return 0;
}

uint8_t I2C_LCD_Init() {
	uint8_t init_data1[] = {0x38, 0x39, 0x14, 0x70, 0x56, 0x6c};
	uint8_t init_data2[] = {0x38, 0x0C, 0x01};
	uint8_t st;

	_delay_ms(40);

	I2C_Init();

	st = I2C_LCD_SendControls(init_data1, sizeof(init_data1));
	if (st != 0) return st;
	_delay_ms(200);

	st = I2C_LCD_SendControls(init_data2, sizeof(init_data2));
	if (st != 0) return st;
	_delay_ms(20);

	return 0;
}

uint8_t I2C_LCD_Clear() {
	uint8_t command[1] = {0x01};
	uint8_t st = I2C_LCD_SendControls(command, 1);
	
	_delay_us(100);
	return st;
}

uint8_t I2C_LCD_Cursor_OnOff (uint8_t enable, uint8_t blink) {
	uint8_t command[1] = { 0x0c | (enable<<1) | (blink<<0) };
	uint8_t st = I2C_LCD_SendControls(command, 1);
	
	_delay_us(100);
	return st;		
}

uint8_t I2C_LCD_Cursor_ReturnToHome () {
	uint8_t command[1] = { 0x02 };
	uint8_t st = I2C_LCD_SendControls(command, 1);
	
	_delay_us(100);
	return st;
}

uint8_t I2C_LCD_Cursor_Shift (uint8_t direction, uint8_t times) {
	// direction: 0-->left, 1-->right
	
	uint8_t command[1] = { 0x10 | (direction<<2) };
	uint8_t st;
	
	for (uint8_t i = 0; i < times; i++) {
		st = I2C_LCD_SendControls(command, 1);
		if (st != 0) return st;
	}
	
	return 0;
}
