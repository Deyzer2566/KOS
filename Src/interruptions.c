#include "ContextSwitcher.h"
#include "Scheduler.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>
#include "ObjectSaverLoader.h"
void SVC_Handler(void);
void KOS_SVC_Handler(volatile int registers[8]);
void KOS_Context_Switcher(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
extern volatile struct Scheduler processScheduler;
void SVC_Handler(void)
{
	__asm volatile("PUSH {r0-r7,lr}");
	__asm volatile("MOV r0, sp");
	__asm volatile("BL KOS_SVC_Handler");
	__asm volatile("POP {r0-r7,lr}");
	__asm volatile("STR r0, [sp]");
	__asm volatile("STR r1, [sp,4]");
}
extern TCHAR* PROC_DIR;
int searchObjectWithName(char* name){
	FILINFO finfo;
	for(int pid=0;pid < processScheduler.lastProcess; pid++){
		char fname[30];
		sprintf(fname, "%s/%d",PROC_DIR,pid);
		FRESULT res = f_stat(fname,&finfo);
		if(res != FR_NO_FILE){
			struct Object obj;
			enum LoadObjectResult loadRes = loadObjectInfo(&obj, FATFS_storage);
			if(loadRes != LOAD_OBJECT_OK){
				continue;
			}
			if(strcmp(name, obj.name) != 0){
				continue;
			}
			return pid;
		}
	}
	return -1;
}
extern enum SaveObjectResult saveObject(struct Object* obj, enum Storage to);
extern enum LoadObjectResult loadObject(struct Object* obj, enum Storage to);
extern void enterProcess();
enum SchedulerResult changeActiveObject(struct Object* obj);
void swapContext(volatile int readMem[16], volatile int writeMem[16], volatile struct ProcessContext* readContext, volatile struct ProcessContext* writeContext);
	
void KOS_SVC_Handler(volatile int registers[8]){
	switch (registers[0]){
		case 0: {			
			char* objName = (char*)(registers[1]);
			registers[0] = searchObjectWithName(objName);
		}
		break;
		case 1:{
			// проверить существование метода
			struct Object obj;
			obj.objId = registers[1];
			
			FILINFO finfo;
			char fname[30];
			sprintf(fname, "%s/%d",PROC_DIR,obj.objId);
			FRESULT res = f_stat(fname,&finfo);
			if(res == FR_NO_FILE){
				registers[0] = -1;
				return;
			}
			FIL file;
			f_open(&file, fname, FA_READ);
			if(loadObjectInfo(&obj, FATFS_storage) != SCHEDULER_OK){
				f_close(&file);
				registers[0] = -1;
				return;
			}
			f_close(&file);
			
			if(obj.methodsCount <= registers[2]){
				registers[0] = -1;
				return;
			}
			// сохранить этот процесс (и сохранить буфер)
			struct Object* oldObj = processScheduler.curObject;
			struct ProcessSettings oldSettings = obj.settings;
			obj.settings = oldObj->settings;
			uint8_t buffer[512];
			size_t size = (registers[4]>512)?512:registers[4];
			for(size_t i = 0;i<size;i++)
				buffer[i] = ((uint8_t*)(registers[3]))[i];
			oldObj->state = WAITING;
			saveObject(oldObj,FATFS_storage);
			// загружаем вызываемый объект
			changeActiveObject(&obj);
			// вызываем метод
			registers[0] = obj.methods[registers[2]].pointerToFunc(buffer, size);
			// после выхода из метода, сохраняем объект и буфер
			obj.settings = oldSettings;
			saveObject(&obj,FATFS_storage);
			// загружаем исходный процесс
			changeActiveObject(oldObj);
			oldObj->state = WORK;
			// записываем ответ в буфер
			for(size_t i = 0;i<size;i++)
				((uint8_t*)(registers[3]))[i] = buffer[i];
			// выход
		}
		break;
		case 2:{
			char* objName = (char*)(registers[1]);
			registers[0] = -1;
			for(size_t i = 0;i<OBJECT_NAME_LEN;i++)
				if(objName[i] == '\0' && (searchObjectWithName(objName) == -1)){
					strcpy(processScheduler.curObject->name, objName);
					registers[0] = 0;
					break;
				}
		}
		break;
		case 3:{
			if(processScheduler.curObject->methodsCount != OBJECT_METHOD_COUNT_MAX){
				processScheduler.curObject->methods[processScheduler.curObject->methodsCount] = (struct Method){.pointerToFunc=(int(*)(uint8_t*, size_t))(registers[1])};
				processScheduler.curObject->methodsCount ++;
				registers[0] = 0;
			} else registers[0] = -1;
		}
		break;
	}
}

void KOS_Context_Switcher(void){
	__asm volatile("TST LR, 4");
	__asm volatile("ITE EQ\nMRSEQ R0, MSP\nMRSNE R0, PSP");
	__asm volatile("SUB R0, #44");
	__asm volatile("TST LR, 4");
	__asm volatile("ITE EQ\nMSREQ MSP, R0\nMSRNE PSP, R0"); // сохраняем в стек значения MSP, PSP и R4-R11
	__asm volatile("MRS R1, MSP");
	__asm volatile("MRS R2, PSP");
	__asm volatile("MOV R3, LR");
	__asm volatile("TST LR, 4");
	__asm volatile("ITE EQ\nADDEQ R1, #0xC\nADDNE R2, #0xC");
	__asm volatile("STM R0, {R1-R11}");
	
	__asm volatile("PUSH {R0,LR}");
	__asm volatile("BL contextChanger"); // меняем контекст
	__asm volatile("POP {R0,LR}");
	
	__asm volatile("LDM R0!, {R1-R3}");
	
	__asm volatile("TST R3, 4");
	__asm volatile("ITE EQ\nMOVEQ R0, R1\nMOVNE R0, R2"); // загружаем R4-R11
	__asm volatile("LDM R0!, {R4-R11}");
	
	__asm volatile("ITE NE\nSUBNE R1, #0xC\nSUBEQ R2, #0xC");
	__asm volatile("MSR MSP, R1");
	__asm volatile("MSR PSP, R2");
	__asm volatile("MOV LR, R3");
	
	__asm volatile("TST LR, 4");
	__asm volatile("ITE EQ\nMSREQ MSP, R0\nMSRNE PSP, R0");
	
	__asm volatile("MRS R0, CONTROL");
	__asm volatile("ITE EQ\nMOVEQ R0, 0\nMOVNE R0, 1");
	__asm volatile("MSR CONTROL, R0");
}

void PendSV_Handler(void)
{	
	__asm volatile("B KOS_Context_Switcher");
}

static volatile int processTactsCounter = 0;
volatile int inIOOperation = 0;
void SystemTact(int LR, int SP){
	HAL_IncTick();
	if(inIOOperation && processScheduler.inProcess)
		inIOOperation |= 0x2;
	if(processScheduler.inProcess && ++processTactsCounter >= processScheduler.curObject->settings.tacts && !inIOOperation/* && LR != 0xFFFFFFF1*/){
		processTactsCounter = 0;
		__asm volatile("MOV SP, %0	\n"
									 "POP {LR}		\n"
									 "POP {R4-R11}\n"
									 "B KOS_Context_Switcher"::"r"(SP));
	}
}

void SysTick_Handler(void){
	__asm volatile("PUSH {R4-R11}		\n"
								 "PUSH {R0,LR}		\n"
								 "MOV R0, LR 			\n"
								 "ADD R1, SP, 4		\n"
								 "BL SystemTact		\n"
								 "POP {R0,LR}			\n"
								 "POP {R4-R11}");
}