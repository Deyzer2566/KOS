#include "ContextSwitcher.h"
#include "Scheduler.h"
#include "stm32f1xx_hal.h"
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
	
}

void KOS_Context_Switcher(){
	__asm volatile("TST LR, 4");
	__asm volatile("ITE EQ\nMRSEQ R0, MSP\nMRSNE R0, PSP");
	__asm volatile("SUB R0, #32");
	__asm volatile("TST LR, 4");
	__asm volatile("ITE EQ\nMSREQ MSP, R0\nMSRNE PSP, R0"); // сохраняем в стек R4-R11
	__asm volatile("STM R0, {R4-R11}");
	
	__asm volatile("PUSH {LR}");
	__asm volatile("BL contextChanger"); // меняем контекст
	__asm volatile("POP {LR}");
	
	__asm volatile("TST LR, 4");
	__asm volatile("ITE EQ\nMRSEQ R0, PSP\nMRSNE R0, MSP"); // загружаем R4-R11
	__asm volatile("LDM R0, {R4-R11}");
	__asm volatile("ADD R0, #32");
	
	__asm volatile("TST LR, 4");
	__asm volatile("ITE EQ\nMSREQ PSP, R0\nMSRNE MSP, R0");
	__asm volatile("ITE EQ\nMOVEQ LR, 0xFFFFFFFD\nMOVNE LR, 0xFFFFFFF9");
}

void PendSV_Handler(void)
{	
	__asm volatile("B KOS_Context_Switcher");
}

extern volatile struct Scheduler processScheduler;
static int processTactsCounter = 0;
void SysTick_Handler(void)
{
	__asm volatile("CMP LR, #0xfffffff1"); // проверка на вызов прерывания из прерывания
	__asm volatile("IT EQ\nBEQ HAL_IncTick");
	if(processScheduler.inProcess && ++processTactsCounter >= processScheduler.curProcessSettings->tacts){
		PendSV_Handler();
		processTactsCounter=0;
	}
  HAL_IncTick();
}