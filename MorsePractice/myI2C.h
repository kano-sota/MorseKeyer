/*
 * myI2C.h
 *
 * Created: 2021/02/12 0:43:41
 *  Author: JJ1MDY
 */ 


#ifndef MYI2C_H_
#define MYI2C_H_

void I2C_Init();
void I2C_Start();
void I2C_Stop();
void I2C_Send(uint8_t data);
uint8_t I2C_Recv(int ack);

#endif /* MYI2C_H_ */