#include "SDCard.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "stm32f1xx.h"
#include "CRCCalculation.h"
struct SDCommand{
	uint8_t commandIndex;
	uint32_t args;
};
struct SDResponse1{
	bool parameterError; // The command’s argument (e.g. address, block length) was outside the allowed range for this card. 
	bool addressError; // A misaligned address that did not match the block length was used in the command. 
	bool eraseSequenceError; // An error in the sequence of erase commands occurred. 
	bool comCRCError; // The CRC check of the last command failed. 
	bool illegalCommand; // An illegal command code was detected.
	bool eraseReset; // An erase sequence was cleared before executing because an out of erase sequence command was received.
	bool inIdleState; // The card is in idle state and running the initializing process. 
};
struct SDResponse2{
	struct SDResponse1 r1;
	bool outOfRange;
	bool eraseParameter; // An invalid selection for erase, sectors or groups. 
	bool writeProtectionViolation; // The command tried to write a write-protected block.
	bool cardECCFailed; // Card internal ECC was applied but failed to correct the data
	bool cardControllerError; // Internal card controller error.
	bool error; // A general or an unknown error occurred during the operation. 
	bool writeProtectionErase; // This status bit has two functions overloaded. It is set when the host attempts to erase a write-protected sector or makes a sequence or password errors during card lock/unlock operation. 
	bool cardIsLocked; // Set when the card is locked by the user. Reset when it is unlocked.
};
struct SDResponse3{
	struct SDResponse1 r1;
	uint16_t supportedVoltage;
	uint8_t ccs;
};
struct SDResponse7{
	struct SDResponse1 r1;
	uint8_t commandVersion;
	uint8_t voltageAccepted;
	uint8_t checkPattern;
};
void flush(const struct SDCardPort* port);
void sendBlock(const struct SDCardPort* port, uint8_t* block, size_t size){
	HAL_SPI_Transmit(port->hspi, block, size, HAL_MAX_DELAY);
}
void sendCommand(const struct SDCardPort* port, struct SDCommand command){
	uint8_t x = 0xff;
	uint8_t data[6];
	data[0] = (1<<6)|(command.commandIndex&0x3f);
	data[1] = (command.args>>24)&0xff;
	data[2] = (command.args>>16)&0xff;
	data[3] = (command.args>>8)&0xff;
	data[4] = command.args&0xff;
	data[5] = (CRC7(data, 40)<<1) | 1;
	sendBlock(port, data, 6);
}
void select(const struct SDCardPort* port){
	uint8_t x = 0xff;
	HAL_GPIO_WritePin(port->GPIOx, port->pin, GPIO_PIN_RESET);
	sendBlock(port, &x, 1);
}
void deselect(const struct SDCardPort* port){
	uint8_t x = 0xff;
	HAL_GPIO_WritePin(port->GPIOx, port->pin, GPIO_PIN_SET);
	sendBlock(port, &x, 1);
}
void readBlock(const struct SDCardPort* port, uint8_t* buffer, size_t buffSize){
	uint8_t x = 0xff;
	while(buffSize > 0) {
			HAL_StatusTypeDef res = HAL_SPI_TransmitReceive(port->hspi, &x, buffer, 1,
															HAL_MAX_DELAY);
			if(res != HAL_OK)
				return;
			buffer++;
			buffSize--;
	}
}

struct SDResponse1 readR1(const struct SDCardPort* port){
	struct SDResponse1 response = {1,1,1,1,1,1,1};
	uint8_t x = 0x80;
	for(int i = 0;i<16 && (x&0x80);i++)
		readBlock(port, &x, 1);
	if(x & 0x80)
		return response;
	response.inIdleState = x&1;
	response.eraseReset = (x>>1)&1;
	response.illegalCommand = (x>>2)&1;
	response.comCRCError = (x>>3)&1;
	response.eraseSequenceError = (x>>4)&1;
	response.addressError = (x>>5)&1;
	response.parameterError = (x>>6)&1;
	return response;
}
void flush(const struct SDCardPort* port){
	uint8_t x = 0xff;
	sendBlock(port, &x, 1);
}
struct SDResponse2 readR2(const struct SDCardPort* port){
	struct SDResponse2 response;
	response.r1 = readR1(port);
	uint8_t data=0;
	readBlock(port, &data, 1);
	response.cardIsLocked = data&1;
	response.writeProtectionErase = (data>>1)&1;
	response.error = (data>>2)&1;
	response.cardControllerError = (data>>3)&1;
	response.cardECCFailed = (data>>4)&1;
	response.writeProtectionViolation = (data>>5)&1;
	response.eraseParameter = (data>>6)&1;
	response.outOfRange = (data>>7)&1;
	return response;
}
struct SDResponse3 readR3(const struct SDCardPort* port){
	struct SDResponse3 response;
	response.r1 = readR1(port);
	uint32_t OCR = 0;
	uint8_t x = 0;
	for(int i = 0;i<4;i++){
		readBlock(port, &x, 1);
		OCR <<= 8;
		OCR |= x;
	}
	response.supportedVoltage = (OCR>>15)&0x1ff;
	response.ccs = (OCR>>30)&0x3;
	return response;
}
struct SDResponse7 readR7(const struct SDCardPort* port){
	struct SDResponse7 response;
	response.r1 = readR1(port);
	uint8_t data[4];
	readBlock(port, data, 4);
	response.commandVersion = (data[0]>>4)&0xf;
	response.voltageAccepted = data[2]&0xf;
	response.checkPattern = data[3];
	return response;
}

struct SDResponse1 sendCommand0(const struct SDCardPort* port){
	sendCommand(port, (struct SDCommand){.args=0, .commandIndex=0});
	struct SDResponse1 resp = readR1(port);
	flush(port);
	return resp;
}
struct SDResponse7 sendCommand8(const struct SDCardPort* port){
	sendCommand(port, (struct SDCommand){.args=((0b1<<8)|(0b10101010)), .commandIndex=8});
	struct SDResponse7 resp = readR7(port);
	flush(port);
	return resp;
}
struct SDResponse3 sendCommand58(const struct SDCardPort* port){
	sendCommand(port, (struct SDCommand){.args=0, .commandIndex=58});
	struct SDResponse3 resp = readR3(port);
	flush(port);
	return resp;
}
struct SDResponse1 sendCommand55(const struct SDCardPort* port){
	sendCommand(port, (struct SDCommand){.args=0, .commandIndex=55});
	struct SDResponse1 resp = readR1(port);
	flush(port);
	return resp;
}
struct SDResponse1 sendCommand41(const struct SDCardPort* port){
	sendCommand(port, (struct SDCommand){.args=0x40000000, .commandIndex=41});
	struct SDResponse1 resp = readR1(port);
	flush(port);
	return resp;
}
struct SDResponse1 sendCommand1(const struct SDCardPort* port){
	sendCommand(port, (struct SDCommand){.args=0x40000000, .commandIndex=1});
	struct SDResponse1 resp = readR1(port);
	flush(port);
	return resp;
}
struct SDResponse1 sendCommand16(const struct SDCardPort* port, uint32_t blockSize){
	sendCommand(port, (struct SDCommand){.args=blockSize, .commandIndex=16});
	struct SDResponse1 resp = readR1(port);
	flush(port);
	return resp;
}
int initializeSDSCV1Card(struct SDCardPort* sdport){
	return -1;
}
int initializeSDV2Card(struct SDCardPort* sdport){
	struct SDResponse3 resp2 = sendCommand58(sdport);
	if(resp2.r1.illegalCommand || resp2.r1.addressError || resp2.r1.comCRCError || resp2.r1.eraseReset || resp2.r1.eraseSequenceError
			|| resp2.r1.parameterError || !(resp2.supportedVoltage&0b01100000))
		return -1;
	for(int k = HAL_GetTick(); HAL_GetTick() - k < 1000;) { // пытаемся инициализировать карту в течении 1 секунды
		struct SDResponse1 resp3 = sendCommand55(sdport);
		if(!resp3.inIdleState || resp3.addressError || resp3.comCRCError || resp3.eraseReset
		|| resp3.eraseSequenceError || resp3.illegalCommand || resp3.parameterError)
			return -1;
		struct SDResponse1 resp4 = sendCommand41(sdport);
		if(resp4.addressError || resp4.comCRCError || resp4.eraseReset || resp4.eraseSequenceError
		|| resp4.illegalCommand || resp4.parameterError)
			return -1;
		if(!resp4.inIdleState) {
			struct SDResponse3 resp5 = sendCommand58(sdport);
			if(resp5.ccs){
				sdport->type = SDHC;
				sdport->blockSize = 512;
			}
			else
				sdport->type = SDSCV2;
			sdport->state = INITIALIZED;
			if(sdport->type != SDHC){
				struct SDResponse1 resp6 = sendCommand16(sdport, 512);
				if(resp6.addressError || resp6.comCRCError || resp6.eraseReset || resp6.eraseSequenceError
				|| resp6.illegalCommand || resp6.parameterError)
					return -1;
				sdport->blockSize = 512;
			}
			sendCommand(sdport, (struct SDCommand){.args=1, .commandIndex=59});
			struct SDResponse1 resp7 = readR1(sdport);
			if(resp7.addressError || resp7.comCRCError || resp7.eraseReset || resp7.eraseSequenceError || resp7.illegalCommand ||
			resp7.inIdleState || resp7.parameterError)
				return -1;
			sendCommand(sdport, (struct SDCommand){.args=0, .commandIndex=10});
			struct SDResponse1 resp = readR1(sdport);
			if(resp.addressError || resp.comCRCError || resp.eraseReset || resp.eraseSequenceError || resp.illegalCommand ||
			resp.inIdleState || resp.parameterError)
				return -1;
			uint8_t x = 0;
			int i = 0;
			for(;i<100 && x != 0xFE; i++)
				readBlock(sdport,&x,1);
			if(i == 100 && x != 0xFE)
				return -1;
			uint8_t buffer[16];
			readBlock(sdport, buffer, 16);
			uint16_t crc = 0;
			readBlock(sdport, (uint8_t*)&crc, 2);
			return 0;
		}
	}
	return -1;
}
int initializeSDCard(struct SDCardPort* sdport){
	deselect(sdport);
	HAL_Delay(1);
	for(int i = 0;i<10;i++)
		flush(sdport);
	select(sdport);
	struct SDResponse1 resp = sendCommand0(sdport);
	if(!(resp.inIdleState && !resp.addressError && !resp.comCRCError && !resp.eraseReset
					&& !resp.eraseSequenceError && !resp.illegalCommand && !resp.parameterError))
		return -1;
	struct SDResponse7 resp1 = sendCommand8(sdport);
	int c = -1;
	if(resp1.r1.illegalCommand)
		c = initializeSDSCV1Card(sdport);
	if((resp1.checkPattern == 0xAA) && !resp1.r1.illegalCommand && (resp1.voltageAccepted == 0x01))
		c = initializeSDV2Card(sdport);
	deselect(sdport);
	return c;
}
/*
Читает один блок из SD карты
Буфер должен иметь размер 512 байт
Возвращает 0 при успехе, -1 при неудаче
*/
int SDReadData(const struct SDCardPort* sdport, uint8_t* buffer, uint32_t address) {
	if(sdport->state != INITIALIZED)
		return -1;
	if(sdport->type == SDHC)
		address /= 512;
	select(sdport);
	flush(sdport);
	sendCommand(sdport,(struct SDCommand){.args=address, .commandIndex = 17});//Отправляем 17 команду для чтения 1 блока
	struct SDResponse1 resp = readR1(sdport);
	if(resp.addressError || resp.comCRCError || resp.eraseReset || resp.eraseSequenceError || resp.illegalCommand ||
	resp.inIdleState || resp.parameterError){
		deselect(sdport);
		return -1;
	}
	uint8_t x = 0;
	int i = 0;
	for(;i<100 && x != 0xFE; i++)
		readBlock(sdport,&x,1);
	if(i == 100 && x != 0xFE){
		deselect(sdport);
		return -1;
	}
	readBlock(sdport, buffer, 512);
	uint16_t crc = 0;
	for(int i = 0;i<2;i++){
		crc<<=8;
		readBlock(sdport, (uint8_t*)&x, 1);
		crc|=x;
	}
	flush(sdport);
	uint16_t crcCorrect = CRC16(buffer, 512*8);
	deselect(sdport);
	if(crcCorrect == crc)
		return 0;
	else
		return -1;
}

/*
Записывает один блок в SD карту
Буфер должен иметь размер 512 байт
Возвращает 0 при успехе, -1 при неудаче
*/
int SDWriteData(const struct SDCardPort* sdport, uint8_t* buffer, uint32_t address) {
	if(sdport->state != INITIALIZED)
		return -1;
	if(sdport->type == SDHC)
		address /= 512;
	select(sdport);
	flush(sdport);
	sendCommand(sdport,(struct SDCommand){.args=address, .commandIndex = 24});//Отправляем 24 команду для записи 1 блока
	struct SDResponse1 resp = readR1(sdport);
	if(resp.addressError || resp.comCRCError || resp.eraseReset || resp.eraseSequenceError || resp.illegalCommand ||
	resp.inIdleState || resp.parameterError){
		deselect(sdport);
		return -1;
	}
	uint8_t x = 0xFE;
	sendBlock(sdport, &x, 1);
	sendBlock(sdport, buffer, 512);
	uint8_t responseToken = 0;
	readBlock(sdport, (uint8_t*)&responseToken, 1);
	if(responseToken != 0x5){
		deselect(sdport);
		return -1;
	}
	while(x != 0xFF)
		readBlock(sdport, &x, 1); // busy
	deselect(sdport);
	return 0;
}