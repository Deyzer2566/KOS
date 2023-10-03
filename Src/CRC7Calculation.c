#include <stddef.h>
#include <stdint.h>
uint8_t CRC7(uint8_t* data, size_t size){ // Size is in bits
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