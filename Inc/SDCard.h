#ifndef SDCARD_H
#define SDCARD_H
#include "stm32f1xx.h"
#include <stdint.h>
#include <stddef.h>

struct SDCardPort{
	SPI_HandleTypeDef* hspi;
	GPIO_TypeDef* GPIOx;
	uint16_t pin;
	uint16_t blockSize;
	enum {NOTINITIALIZED=0, INITIALIZED=1} state;
	enum {MMC=0, SDSCV1, SDSCV2, SDHC} type;
};

int initializeSDCard(struct SDCardPort* sdport);

/*
Читает один блок из SD карты
Буфер должен иметь размер 512 байт
Возвращает 0 при успехе, -1 при неудаче
*/
int SDReadData(const struct SDCardPort* sdport, uint8_t* buffer, uint32_t address);

/*
Записывает один блок на SD карту
Буфер должен иметь размер 512 байт
Возвращает 0 при успехе, -1 при неудаче
*/
int SDWriteData(const struct SDCardPort* sdport, uint8_t* buffer, uint32_t address);
#endif