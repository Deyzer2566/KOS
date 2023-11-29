#include "ContextSwitcher.h"
#include <stddef.h>

void saveContext(volatile int* registers, volatile struct ProcessContext* context){	
	for(size_t i = 4;i<=11;i++)
		context->r[i]=registers[i-4];
	context->sp = registers + 8 + 8; // 0x20 - размер фрейма + 0x20 - регистры R4-R11
	context->pc = registers[6+8]; // PC находится в 0x18 относительно PSP
	context->lr = registers[5+8]; // LR находится в 0x14 относительно PSP
	context->xPSR = registers[7+8]; // xPSR находится в 0x1C относительно PSP
	for(size_t i = 0;i<=3;i++)
		context->r[i]=registers[i+8];
	context->r[12] = registers[4+8];
}

void restoreContext(volatile int* registers, volatile struct ProcessContext* context){
	for(size_t i = 4;i <= 11; i++)
		registers[i-4] = context->r[i];
	for(size_t i = 0;i<=3;i++)
		registers[i+8] = context->r[i];
	registers[4+8] = context->r[12];
	registers[5+8] = context->lr;
	registers[6+8] = context->pc;
	registers[7+8] = context->xPSR | 0x01000000; //  | 0x01000000 нужен для записи 1 в бит Thumb регистра(он не сохраняется при 
}

void changeContext() {
	volatile int* ICSR = (int*)0xE000ED04;
	*ICSR |= 0x10000000;
}