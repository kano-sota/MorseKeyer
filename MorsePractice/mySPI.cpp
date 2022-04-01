/*
 * mySPI.cpp
 *
 * Created: 2021/02/12 9:37:51
 * Author: JJ1MDY
 *
 * for ATmega328p
 */ 

#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include<avr/io.h>
#include<util/delay.h>

#define DDR_SPI DDRB
#define DD_MOSI 3
#define DD_MISO 4
#define DD_SCK  5

void SPI_MasterInit(char mode) {
	DDR_SPI |= (1<<DD_MOSI) | (1<<DD_SCK);	// output: MOSI, SCK	input: other
	SPCR = (1<<SPE) | (1<<MSTR) | (mode<<CPHA) | (1<<SPR1);	// SPI enable, master, div64 
}

void SPI_MasterTransmit(char cData) {
	SPDR = cData;	// start SPI
	while (!(SPSR&(1<<SPIF)));	// wait
}