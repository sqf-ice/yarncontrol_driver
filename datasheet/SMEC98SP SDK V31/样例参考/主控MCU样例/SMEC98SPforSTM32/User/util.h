/*
*
@file		util.h
@brief
*/

#ifndef _UTIL_H
#define _UTIL_H

#include <stdio.h>


void Set_network(void);
void Reset_W5200(void);

void RED_onoff(uint8_t on_off);
void GREEN_onoff(uint8_t on_off);
void shine(void);

void USART1_Init(void);

void Delay_us(uint8 time_us);
void Delay_ms(uint16 time_ms);

int put_char(int ch);
int get_char(void);
int fputc(int ch, FILE *f);
uint32_t time_return(void);
void PrintHex(unsigned char *str,unsigned int len);


#endif
