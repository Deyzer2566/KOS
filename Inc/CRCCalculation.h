#ifndef CRCCALCULATION_H
#define CRCCALCULATION_H

#include <stddef.h>
#include <stdint.h>

/* 
	Вычисляет CRC7 сообщения
	data - указатель на данные
	size - размер в битах
*/
uint8_t CRC7(uint8_t* data, size_t size){ 
	size_t curBit=7;
	uint8_t curByte=(data[0]>>1)&0x7f;
	const uint8_t polynom = 0x89;
	const uint8_t zay = 7;
	while(curBit < size+zay) {
		curByte <<= 1;
		if(curBit < size)
			curByte |= (data[curBit/8]>>(7-(curBit%8)))&1;
		curBit++;
		if(curByte&0x80)
			curByte ^= polynom;
	}
	return curByte;
}

uint16_t CRC16(uint8_t* data, size_t size){
	size_t curBit=0;
	uint32_t curWord=0x0;
	const uint32_t polynom = 0x11021;
	const uint8_t zay = 16;
	while(curBit < size+zay) {
		curWord <<= 1;
		if(curBit < size)
			curWord |= (data[curBit/8]>>(7-(curBit%8)))&1;
		curBit++;
		if(curWord&0x10000)
			curWord ^= polynom;
	}
	return (uint16_t)curWord;
}
#endif