/*
 * myLCD_ST7032.h
 *
 * Created: 2021/02/12 0:51:44
 *  Author: JJ1MDY
 */ 


#ifndef MYLCD_ST7032_H_
#define MYLCD_ST7032_H_

uint8_t I2C_LCD_SendControls(uint8_t data[], uint8_t num);
uint8_t I2C_LCD_SendString(uint8_t addr, char *data, uint8_t num);
uint8_t I2C_LCD_Init();
uint8_t I2C_LCD_Clear();
uint8_t I2C_LCD_Cursor_OnOff (uint8_t enable, uint8_t blink);
uint8_t I2C_LCD_Cursor_ReturnToHome ();
uint8_t I2C_LCD_Cursor_Shift (uint8_t direction, uint8_t times);

#endif /* MYLCD_ST7032_H_ */