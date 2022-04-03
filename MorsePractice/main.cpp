/*
 * MorsePractice.cpp
 *
 * Created: 2021/11/13 23:51:42
 * Author : JJ1MDY
 */ 
#define F_CPU 8000000UL	// System clock = 8MHz
// Fuses E:FF, H:DF, L:E2 (ATmega328p)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/twi.h>
#include <stdio.h>
#include "myI2C.h"
#include "myLCD_ST7032.h"

#define DOT 1
#define DASH 2
#define TICK_INTERVAL 1000	// Timer1 interrupt interval time [us]
#define TONE_TABLE_N 45			// Tone Frequency = 700 Hz (fixed)
#define RISETIME 4				// Rise/Fall in 4 period
#define TONE_BEGIN_TABLE_N TONE_TABLE_N * RISETIME

volatile uint8_t pwm_ovf_flag = 0;
volatile uint8_t dot_input_flag = 0;
volatile uint8_t dash_input_flag = 0;
volatile uint16_t blank_count = 0;

const uint8_t tone_table[TONE_TABLE_N] PROGMEM = {248, 246, 243, 237, 229, 219, 208, 195, 180, 165, 148, 132, 115, 98, 83, 68, 54, 41, 30, 22, 15, 10, 8, 8, 10, 15, 22, 30, 41, 54, 67, 83, 98, 115, 132, 148, 165, 180, 195, 208, 219, 229, 237, 243, 246};
const uint8_t tone_begin_table[TONE_BEGIN_TABLE_N] PROGMEM = {0, 0, 1, 3, 3, 5, 6, 6, 7, 7, 8, 7, 7, 7, 5, 5, 4, 4, 2, 2, 1, 0, 1, 1, 1, 1, 2, 4, 6, 8, 10, 14, 16, 20, 24, 27, 32, 36, 40, 44, 47, 51, 54, 56, 59, 62, 61, 62, 63, 61, 60, 58, 55, 52, 49, 45, 40, 36, 31, 26, 22, 18, 14, 10, 7, 5, 2, 3, 3, 4, 5, 8, 12, 16, 21, 28, 35, 41, 49, 57, 65, 73, 81, 89, 96, 102, 109, 113, 117, 121, 124, 123, 123, 122, 118, 115, 110, 103, 97, 90, 82, 73, 64, 56, 47, 38, 31, 24, 17, 13, 9, 5, 5, 5, 6, 9, 14, 19, 26, 35, 44, 56, 66, 78, 90, 102, 114, 126, 138, 148, 157, 166, 173, 178, 183, 186, 185, 184, 182, 176, 170, 162, 152, 142, 131, 119, 106, 93, 81, 67, 55, 45, 35, 25, 18, 13, 8, 7, 7, 9, 13, 19, 27, 37, 48, 61, 76, 91, 107, 123, 139, 156, 171, 187, 200, 212, 224, 232, 239, 244};

const char decoded_char_table[] PROGMEM =
	"# ETIANMSURWDKGOHVF\x9aL\x8ePJBXCYZQ\x99#"
	"54#3\x90##2##+##\x8f#16=/#\x80#(#7##\x9c""8#90"
	"^###########?_####\"##.####@###'#"
	"#-###########)#####,####:#######";
char line_buffer[] = " MORSE PRACTICE ";

void GPIO_init();
void Timer_init();

void Tick_on();
void Tick_off();

void Tone_begin();
void Tone_repeat();
void Tone_end();

void PWM_valueset();

uint8_t Dot_function(uint16_t period);
uint8_t Dash_function(uint16_t period);

uint16_t WPM_to_wavenum(uint8_t WPM);

uint8_t ReadInput_dot()  { return !(PIND&(1<<PIND6)); }
uint8_t ReadInput_dash() { return !(PIND&(1<<PIND7)); }
void WriteOutput_keyout(uint8_t bit) { bit?(PORTB |= 1<<PINB0):(PORTB &= ~(1<<PINB0)); return; }

void PrintChar (uint8_t char_id);

int main(void) {
	uint8_t WPM = 15;
	uint16_t BLANK_CHAR = 1200 / WPM, BLANK_WORD = 3600 / WPM;
	uint8_t state = 0;
	uint8_t char_id = 1;
	
	GPIO_init();
	Timer_init();
	I2C_Init();
	I2C_LCD_Init();
	sei();
	
	Tick_on();
	I2C_LCD_SendString(0x00, line_buffer, 16);
	I2C_LCD_Cursor_OnOff(1, 0);
	
	while (1) {
		
		if (state == 0) {
			if (dot_input_flag)  state = DOT;
			if (dash_input_flag) state = DASH;
		}
		
		if (state == DOT)  {
			state = Dot_function(WPM_to_wavenum(WPM));
			char_id <<= 1;
			blank_count = 0;
		} else if (state == DASH) {
			state = Dash_function(WPM_to_wavenum(WPM));
			char_id = (char_id << 1) + 1;
			blank_count = 0;
		} else {
			if (blank_count > BLANK_CHAR && char_id != 1) {
				PrintChar(char_id);
				char_id = 1;
			}
			if (blank_count == BLANK_WORD) PrintChar(1);
		}
		dot_input_flag = 0;	dash_input_flag = 0;
		
	}
}

void GPIO_init() {
	DDRB = 0xff; // LED: PB0
	DDRC = 0xfe; // Volume: PC0
	DDRD = 0x3f; // Key-input: PD6(dot) & PD7(dash)
	PORTB = 0x00;
	PORTC = 0x00;
	PORTD = 0xc0; // Pull-up: PB0,PB1
}

void Timer_init() {
	// Timer0 settings
	// For generate PWM sine-wave
	TCCR0A = 0x23;	// COM0B = 0b10 (output from OC0B-pin), WGM0 = 0b011 (8bit PWM)
	TCCR0B = 0x00;	// not divide (stop at first)
	OCR0B = 0;
	
	// Timer1 settings
	// For make tick
	TCCR1A = 0x00;	// standard mode (not PWM)
	TCCR1B = 0x08;	// not divide (stop at first)
	OCR1A = 8 * TICK_INTERVAL;	// [us]
	
	TIMSK0 = (1<<TOIE0);	// Timer0 Overflow Interrupt
	TIMSK1 = (1<<OCIE1A);	// Timer1 Compare A Match Interrupt
}
void Tick_on() {
	TCCR1B |= 1<<CS10;	// start Timer1
}
void Tick_off() {
	TCCR1B &= ~(1<<CS10);	// stop Timer1
}

void PWM_valueset(uint8_t value) {
	OCR0B = value;
}

void Tone_begin() {
	TCCR0B |= 1<<CS00;	// start Timer0
	
	WriteOutput_keyout(1);
		
	for (uint8_t i = 0; i < TONE_BEGIN_TABLE_N; i++){
		while (!pwm_ovf_flag) {}
		pwm_ovf_flag = 0;
		PWM_valueset(pgm_read_byte(tone_begin_table + i));
	}
}

void Tone_repeat(uint16_t wavenum) {
	for (uint16_t j = 0; j < wavenum; j++) {
		for (uint8_t i = 0; i < TONE_TABLE_N; i++){
			while (!pwm_ovf_flag) {}
			pwm_ovf_flag = 0;
			PWM_valueset(pgm_read_byte(tone_table + i));
		}
	}
}

void Tone_end() {
	for (uint8_t i = 1; i <= TONE_BEGIN_TABLE_N; i++){
		while (!pwm_ovf_flag) {}
		pwm_ovf_flag = 0;
		PWM_valueset(pgm_read_byte(tone_begin_table + TONE_BEGIN_TABLE_N - i));
	}
	
	TCCR0B &= ~(1<<CS00);	// stop Timer0
	
	WriteOutput_keyout(0);
}

void Tone_space(uint16_t wavenum) {
	for (uint16_t j = 0; j < wavenum; j++) {
		for (uint8_t i = 0; i < TONE_TABLE_N; i++){
			_delay_us(32);
		}
	}
}


uint8_t Dot_function(uint16_t wavenum) {
	// Tone (1 period) + Wait (1 period)
	// Receive DASH input during last 1 period (for squeeze-keying)
	
	uint8_t nextstate = 0;
	
	Tone_begin();
	Tone_repeat(wavenum);
	Tone_end();
	
	dash_input_flag = 0;
	
	Tone_space(wavenum);
	
	if (dash_input_flag) nextstate = DASH;
	
	return nextstate;
}
uint8_t Dash_function(uint16_t wavenum) {
	// Tone (3 period) + Wait (1 period)
	// Accept DOT-input during last 2 period (for squeeze-keying)
	
	uint8_t nextstate = 0;
	
	Tone_begin();
	Tone_repeat(wavenum<<1);
	
	dot_input_flag = 0;
	
	Tone_repeat(wavenum);
	Tone_end();
	
	Tone_space(wavenum);
	
	nextstate = dot_input_flag ? DOT : 0;
	
	return nextstate;
}

uint16_t WPM_to_wavenum(uint8_t WPM) {
	// Dot[s] = 60[s] / (50 * WPM)
	// rep = Dot[us] / ovf_interval(=32us) / SINE_TABLE_N
	return 37500 / (uint16_t)WPM / TONE_TABLE_N - RISETIME;
}

void PrintChar(uint8_t char_id) {
	static uint8_t cursor_pos = 0x0F;
	char c;
	
	if (char_id == 0 && cursor_pos > 0x00) { // backspace
		cursor_pos = cursor_pos == 0x40 ? 0x0F : cursor_pos - 1;

		c = ' ';
		I2C_LCD_SendString(cursor_pos, &c, 1);
		I2C_LCD_Cursor_Shift(0, 1);
	} else if (char_id != 0) {
		c = char_id <= 128 ? pgm_read_byte(decoded_char_table + char_id) : '#';
		I2C_LCD_SendString(cursor_pos, &c, 1);
		line_buffer[cursor_pos & 0x0F] = c;
	
		++cursor_pos;
	}
	
	if (cursor_pos == 0x50) {
		I2C_LCD_Clear();
		I2C_LCD_SendString(0x00, line_buffer, 16);
	}
	if (cursor_pos & 0x10) cursor_pos = 0x40;
}

// Timer Interrupt
ISR(TIMER0_OVF_vect) {
	pwm_ovf_flag = 1;
}

ISR(TIMER1_COMPA_vect) {
	if (ReadInput_dot()) dot_input_flag = 1;
	if (ReadInput_dash()) dash_input_flag = 1;
	++blank_count;
}
