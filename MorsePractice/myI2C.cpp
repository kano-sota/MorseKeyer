/*
 * myI2C.cpp
 *
 * Created: 2021/02/12 0:06:34
 *  Author: JJ1MDY
 */ 

#include <avr/io.h>
#include <util/twi.h>

#ifndef F_CPU
#define F_CPU 8000000UL
#endif

void I2C_Init() {
	/* f_SCL = f_CPU / (16 + 2 * TWBR * N) */
	/* N: Prescaler <-- TWPS1,0 (1, 4, 16, 64) */
	/* e.g. f_CPU = 8MHz, f_SCL = 100kHz --> TWSR[TWPS]=0b00 & TWBR=32 */
	
	TWSR = 0x00;	// N = 1
	TWBR = (uint8_t) F_CPU / 200000UL - 8;	// f_SCL = 100kHz
}

void I2C_Start() {
	TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
	loop_until_bit_is_set(TWCR, TWINT);
}

void I2C_Stop() {
	TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN);
	loop_until_bit_is_set(TWCR, TWSTO);
}

void I2C_Send(uint8_t data) {
	TWDR = data;
	TWCR = _BV(TWINT) | _BV(TWEN);
	loop_until_bit_is_set(TWCR, TWINT);
}

uint8_t I2C_Recv(int ack) {
	if (ack) {
		TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);
		} else {
		TWCR = _BV(TWINT) | _BV(TWEN);
	}
	loop_until_bit_is_set(TWCR, TWINT);
	return TWDR;
}