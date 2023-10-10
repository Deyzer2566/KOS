#ifndef SDCARD_H
#define SDCARD_H
#include "stm32f1xx.h"
#include <stdint.h>
#include <stddef.h>

struct SDCardPort{
	SPI_HandleTypeDef* hspi;
	GPIO_TypeDef* GPIOx;
	uint16_t pin;
	enum {NOTINITIALIZED=0, INITIALIZED=1} state;
	enum {MMC=0, SDSCV1, SDSCV2, SDHC} type;
};

struct SDCommand;

struct SDResponse1;

uint8_t r1tobyte(struct SDResponse1 r);

struct SDResponse2;

struct SDResponse3;

struct SDResponse7;

struct DataResponseToken;

void sendBlock(struct SDCardPort port, uint8_t* block, size_t size);

void sendCommand(struct SDCardPort port, struct SDCommand command);

void readBlock(struct SDCardPort port, uint8_t* buffer, size_t buffSize);

struct SDResponse1 readR1(struct SDCardPort port);

struct SDResponse2 readR2(struct SDCardPort port);

struct SDResponse3 readR3(struct SDCardPort port);

struct SDResponse7 readR7(struct SDCardPort port);

void sendCommand0(struct SDCardPort port);

void sendCommand8(struct SDCardPort port);
	
void sendCommand58(struct SDCardPort port);

void sendCommand55(struct SDCardPort port);

void sendACommand41(struct SDCardPort port);

void sendCommand16(struct SDCardPort port, uint32_t blockSize);

int initializeSDSCV1Card(struct SDCardPort sdport);

int initializeSDV2Card(struct SDCardPort sdport);

int initializeSDCard(struct SDCardPort sdport);

/*
Читает один блок из SD карты
Буфер должен иметь размер 512 байт
Возвращает 0 при успехе, -1 при неудаче
*/
int SDReadData(struct SDCardPort sdport, uint8_t* buffer, uint32_t address);

/*
Записывает один блок в SD карты
Буфер должен иметь размер 512 байт
Возвращает 0 при успехе, -1 при неудаче
*/
int SDWriteData(struct SDCardPort sdport, uint8_t* buffer, uint32_t address);
#endif