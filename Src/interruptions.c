#include "stm32f1xx_it.h"

void SVC_Handler(void)
{
	__asm volatile("PUSH {r0-r7,lr}");
	__asm volatile("MOV r0, sp");
	__asm volatile("BL KOS_SVC_Handler");
	__asm volatile("POP {r0-r7,lr}");
	__asm volatile("STR r0, [sp]");
	__asm volatile("STR r1, [sp,4]");
}

void KOS_SVC_Handler(int* registers){
	registers[0]=0x11;
	registers[1]=0x22;
}