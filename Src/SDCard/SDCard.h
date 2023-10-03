#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "stm32f1xx_hal_spi.h"
#include "../CRC7Calculation.h"

struct SDCardPort{
	SPI_HandleTypeDef* hspi;
	GPIO_TypeDef* GPIOx;
	uint16_t pin;
};
struct SDCommand{
	uint8_t commandIndex;
	uint32_t args;
};
struct SDCommandR1{
	bool parameterError; // The command’s argument (e.g. address, block length) was outside the allowed range for this card. 
	bool addressError; // A misaligned address that did not match the block length was used in the command. 
	bool eraseSequenceError; // An error in the sequence of erase commands occurred. 
	bool comCRCError; // The CRC check of the last command failed. 
	bool illegalCommand; // An illegal command code was detected.
	bool eraseReset; // An erase sequence was cleared before executing because an out of erase sequence command was received.
	bool inIdleState; // The card is in idle state and running the initializing process. 
};
struct SDCommandR2{
	struct SDCommandR1 r1;
	bool outOfRange;
	bool eraseParameter; // An invalid selection for erase, sectors or groups. 
	bool writeProtectionViolation; // The command tried to write a write-protected block.
	bool cardECCFailed; // Card internal ECC was applied but failed to correct the data
	bool cardControllerError; // Internal card controller error.
	bool error; // A general or an unknown error occurred during the operation. 
	bool writeProtectionErase; // This status bit has two functions overloaded. It is set when the host attempts to erase a write-protected sector or makes a sequence or password errors during card lock/unlock operation. 
	bool cardIsLocked; // Set when the card is locked by the user. Reset when it is unlocked.
};
struct SDCommandR3{
	struct SDCommandR1 r1;
	uint32_t OCR; 
	/*bool cardPowerUpStatus;
	bool cardCapacityStatus;
	bool UHSIICardStatus;
	uint16_t supportedVoltage;*/
};
struct SDCommandR7{
	struct SDCommandR1 r1;
	uint8_t commandVersion;
	uint8_t voltageAccepted;
	uint8_t checkPattern;
};
struct DataResponseToken {
	uint8_t status;
	
};

void sendCommand(struct SDCardPort port, struct SDCommand command){
	uint8_t data[6];
	HAL_GPIO_WritePin(port.GPIOx, port.pin, GPIO_PIN_RESET);
	data[0] = (1<<6)|(command.commandIndex&0x3f);
	data[1] = (command.args>>24)&0xff;
	data[2] = (command.args>>16)&0xff;
	data[3] = (command.args>>8)&0xff;
	data[4] = command.args&0xff;
	data[5] = (CRC7(data, 40)<<1) | 1;
	HAL_SPI_Transmit(port.hspi, data, 6, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(port.GPIOx, port.pin, GPIO_PIN_SET);//?
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

struct SDCommandR1 readR1(struct SDCardPort port){
	struct SDCommandR1 response;
	uint8_t x = 0x80;
	for(int i = 0;i<2 && (x&0x80);i++)
		readBlock(port, &x, 1);
	if(x == 0x80)
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

struct SDCommandR2 readR2(struct SDCardPort port){
	struct SDCommandR2 response;
	response.r1 = readR1(port);
	uint8_t data;
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
struct SDCommandR3 readR3(struct SDCardPort port){
	struct SDCommandR3 response;
	response.r1 = readR1(port);
	uint8_t data[4];
	readBlock(port, data, 4);
	response.OCR = *((uint32_t*)&data);
	return response;
}
struct SDCommandR7 readR7(struct SDCardPort port){
	struct SDCommandR7 response;
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