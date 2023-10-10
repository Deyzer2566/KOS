#include "SDCard.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "stm32f1xx.h"
#include "../CRCCalculation.h"
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
uint8_t r1tobyte(struct SDResponse1 r){
	return (uint8_t)(((r.parameterError&1)<<6)|((r.addressError&1)<<5)|((r.eraseSequenceError&1)<<4)|((r.comCRCError&1)<<3)|((r.illegalCommand&1)<<2)
		| ((r.eraseReset&1)<<1) | (r.inIdleState&1));
}
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
	uint8_t OCR[4];
	/*bool cardPowerUpStatus;
	bool cardCapacityStatus;
	bool UHSIICardStatus;
	uint16_t supportedVoltage;*/
};
struct SDResponse7{
	struct SDResponse1 r1;
	uint8_t commandVersion;
	uint8_t voltageAccepted;
	uint8_t checkPattern;
};
struct DataResponseToken {
	uint8_t status;
	
};
void sendBlock(struct SDCardPort port, uint8_t* block, size_t size){
	HAL_GPIO_WritePin(port.GPIOx, port.pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(port.hspi, block, size, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(port.GPIOx, port.pin, GPIO_PIN_SET);//?
}
void sendCommand(struct SDCardPort port, struct SDCommand command){
	HAL_GPIO_WritePin(port.GPIOx, port.pin, GPIO_PIN_SET);
	uint8_t x = 0xff;
	HAL_SPI_Receive(port.hspi, &x, 1, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(port.GPIOx, port.pin, GPIO_PIN_RESET);
	HAL_SPI_Receive(port.hspi, &x, 1, HAL_MAX_DELAY);
	uint8_t data[6];
	data[0] = (1<<6)|(command.commandIndex&0x3f);
	data[1] = (command.args>>24)&0xff;
	data[2] = (command.args>>16)&0xff;
	data[3] = (command.args>>8)&0xff;
	data[4] = command.args&0xff;
	data[5] = (CRC7(data, 40)<<1) | 1;
	sendBlock(port, data, 6);
}

void readBlock(struct SDCardPort port, uint8_t* buffer, size_t buffSize){
	HAL_GPIO_WritePin(port.GPIOx, port.pin, GPIO_PIN_RESET);
	uint8_t x = 0xff;
	while(buffSize > 0) {
			HAL_SPI_TransmitReceive(port.hspi, &x, buffer, 1,
															HAL_MAX_DELAY);
			buffer++;
			buffSize--;
	}
	HAL_GPIO_WritePin(port.GPIOx, port.pin, GPIO_PIN_SET);
}

struct SDResponse1 readR1(struct SDCardPort port){
	struct SDResponse1 response = {1};
	uint8_t x = 0x80;
	for(int i = 0;i<2 && (x&0x80);i++)
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

struct SDResponse2 readR2(struct SDCardPort port){
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
struct SDResponse3 readR3(struct SDCardPort port){
	struct SDResponse3 response;
	response.r1 = readR1(port);
	readBlock(port, response.OCR, 4);
	return response;
}
struct SDResponse7 readR7(struct SDCardPort port){
	struct SDResponse7 response;
	response.r1 = readR1(port);
	uint8_t data[4];
	readBlock(port, data, 4);
	response.commandVersion = (data[0]>>4)&0xf;
	response.voltageAccepted = data[2]&0xf;
	response.checkPattern = data[3];
	return response;
}

void sendCommand0(struct SDCardPort port){
	struct SDCommand command;
	command.commandIndex=0;
	command.args=0;
	sendCommand(port, command);
}
void sendCommand8(struct SDCardPort port){
	struct SDCommand command;
	command.commandIndex=8;
	command.args=(0b1<<8)|(0b10101010); // Напряжение и checkPattern
	sendCommand(port, command);
}
void sendCommand58(struct SDCardPort port){
	struct SDCommand command;
	command.commandIndex=58;
	command.args=0;
	sendCommand(port, command);
}
void sendCommand55(struct SDCardPort port){
	struct SDCommand command;
	command.commandIndex=55;
	command.args=0;
	sendCommand(port, command);
}
void sendACommand41(struct SDCardPort port){
	struct SDCommand command;
	command.commandIndex=41;
	command.args=0x40000000;
	sendCommand(port, command);
}
void sendCommand16(struct SDCardPort port, uint32_t blockSize){
	struct SDCommand command;
	command.commandIndex=16;
	command.args=blockSize;
	sendCommand(port, command);
}
int initializeSDSCV1Card(struct SDCardPort sdport){
	return -1;
}
int initializeSDV2Card(struct SDCardPort sdport){
	sendCommand58(sdport);
	struct SDResponse3 resp2;
	resp2 = readR3(sdport);
	if(resp2.r1.illegalCommand || !(resp2.OCR[1]&0b00110000))
		return -1;
	for(int i = 0;i<200;i++) { // 200 попыток инициализировать карту
		sendCommand55(sdport);
		struct SDResponse1 resp3 = readR1(sdport);
		if(!resp3.inIdleState || resp3.addressError || resp3.comCRCError || resp3.eraseReset
		|| resp3.eraseSequenceError || resp3.illegalCommand || resp3.parameterError)
			return -1;
		sendACommand41(sdport);
		struct SDResponse1 resp4 = readR1(sdport);
		if(resp4.addressError || resp4.comCRCError || resp4.eraseReset || resp4.eraseSequenceError
		|| resp4.illegalCommand || resp4.parameterError)
			return -1;
		if(!resp4.inIdleState) {
			sendCommand58(sdport);
			struct SDResponse3 resp5 = readR3(sdport);
			if(resp5.OCR[0]&0xC0)
				sdport.type = SDHC;
			else
				sdport.type = SDSCV2;
			sdport.state = INITIALIZED;
			return 0;
		}
	}
	return -1;
}
int initializeSDCard(struct SDCardPort sdport){
	sendCommand0(sdport);
	struct SDResponse1 resp = readR1(sdport);
	if(!(resp.inIdleState && !resp.addressError && !resp.comCRCError && !resp.eraseReset
					&& !resp.eraseSequenceError && !resp.illegalCommand && !resp.parameterError))
		return -1;
	sendCommand8(sdport);
	struct SDResponse7 resp1;
	resp1 = readR7(sdport);
	if(resp1.r1.illegalCommand)
		return initializeSDSCV1Card(sdport);
	if((resp1.checkPattern == 0xAA) && !resp1.r1.illegalCommand && (resp1.voltageAccepted == 0x01))
		return initializeSDV2Card(sdport);
	return -1;
}
/*
Читает один блок из SD карты
Буфер должен иметь размер 512 байт
Возвращает 0 при успехе, -1 при неудаче
*/
int SDReadData(struct SDCardPort sdport, uint8_t* buffer, uint32_t address) {
	sendCommand(sdport,(struct SDCommand){.args=address, .commandIndex = 17});//Отправляем 17 команду для чтения 1 блока
	struct SDResponse1 resp = readR1(sdport);
	if(resp.addressError || resp.comCRCError || resp.eraseReset || resp.eraseSequenceError || resp.illegalCommand ||
			resp.inIdleState || resp.parameterError) return -1;
	uint8_t x = 0;
	int i = 0;
	for(;i<100 && x != 0xFE; i++)
		readBlock(sdport,&x,1);
	if(i == 100 && x != 0xFE)
		return -1;
	readBlock(sdport, buffer, 512);
	uint16_t crc = 0;
	readBlock(sdport, (uint8_t*)&crc, 2);
	return 0;
}

/*
Записывает один блок в SD карты
Буфер должен иметь размер 512 байт
Возвращает 0 при успехе, -1 при неудаче
*/
int SDWriteData(struct SDCardPort sdport, uint8_t* buffer, uint32_t address) {
	sendCommand(sdport,(struct SDCommand){.args=address, .commandIndex = 24});//Отправляем 24 команду для записи 1 блока
	struct SDResponse1 resp = readR1(sdport);
	if(resp.addressError || resp.comCRCError || resp.eraseReset || resp.eraseSequenceError || resp.illegalCommand ||
			resp.inIdleState || resp.parameterError) return -1;
	uint8_t x = 0xFE;
	sendBlock(sdport, &x, 1);
	sendBlock(sdport, buffer, 512);
	uint8_t responseToken = 0;
	readBlock(sdport, (uint8_t*)&responseToken, 1);
	if(responseToken != 0x5)
		return -1;
	while(x != 0xFF)
		readBlock(sdport, &x, 1); // busy
	return 0;
}