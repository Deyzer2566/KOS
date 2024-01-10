#include "Scheduler.h"
#include "process_loader.h"
#include "fatfs_extend.h"
#include "ContextSwitcher.h"
#include <stdlib.h>
#include <stdio.h>
#include "ObjectSaverLoader.h"

TCHAR* PROC_DIR = _T("KOS/proc");

enum SchedulerResult addObject(struct Object* obj);
enum SchedulerResult deleteObject(struct Object* obj);
void swapContext(volatile int readMem[16], volatile int writeMem[16], volatile struct ProcessContext* readContext, volatile struct ProcessContext* writeContext);
volatile struct Scheduler processScheduler={0};
enum SchedulerResult initializeScheduler(){
	volatile int* CCR = (int*)0xE000ED14;
	*CCR |= 1;
	
	FILINFO filInfo;
	FRESULT res=f_stat(PROC_DIR, &filInfo);
	if(res == FR_OK)
		res = f_recursiveRemoveDir(PROC_DIR);
	else if(res == FR_NO_FILE || res == FR_NO_PATH)
		res = f_mkdir(PROC_DIR);
	if(res == FR_OK)
		return SCHEDULER_OK;
	else
		return SCHEDULER_CANT_INIT;
}

enum SchedulerResult changeActiveObject(struct Object* obj){
	enum LoadObjectResult res = loadObject(obj,FATFS_storage);
	if(res != LOAD_OBJECT_OK){
		deleteObject(obj);
		return SCHEDULER_CANT_LOAD;
	}
	
	processScheduler.curObject = obj;
	return SCHEDULER_OK;
}

void enterProcess(){
	changeContext();
}

void changeContextToObject(volatile int registers[19]){
	volatile int* SHCSR = (int*)0xE000ED24;
	processScheduler.mainProcContext.SHCSR = *SHCSR;
	*SHCSR = processScheduler.curObject->process.context.processorContext.SHCSR;
	*SHCSR &= ~0x800;
	*SHCSR |= 0x400;
	
	saveContext(registers+3, &processScheduler.mainContext);
	processScheduler.mainContext.sp = registers[0];
	
	registers[1] = processScheduler.curObject->process.context.sp;
	if(processScheduler.handlerDataSize != 0){
		registers[0] = (int)processScheduler.handlerData;
		registers[2] = 0xFFFFFFF1;
		restoreContext((int*)(registers[0]), &processScheduler.curObject->process.context);
	}
	else{
		registers[2] = 0xFFFFFFFD;
		restoreContext((int*)(registers[1]), &processScheduler.curObject->process.context);
	}
	processScheduler.inProcess=1;
}

void changeContextToSystem(volatile int registers[19]){
	volatile int* SHCSR = (int*)0xE000ED24;
	processScheduler.curObject->process.context.processorContext.SHCSR = *SHCSR;
	*SHCSR = processScheduler.mainProcContext.SHCSR;
	*SHCSR &= ~0x400;
	*SHCSR |= 0x800;
	
	swapContext(registers+3, (int*)processScheduler.mainContext.sp, &processScheduler.curObject->process.context, &processScheduler.mainContext);
	processScheduler.curObject->process.context.sp = registers[1];
	
	processScheduler.handlerData = (char*)(registers[0]);
	processScheduler.handlerDataSize = (processScheduler.mainContext.sp-registers[0]-0xC);
		
	registers[0] = processScheduler.mainContext.sp;

	processScheduler.inProcess=0;
	registers[2] = 0xFFFFFFF9;
	
	saveObject(processScheduler.curObject,FATFS_storage);
}

void swapContext(volatile int readMem[16], volatile int writeMem[16], volatile struct ProcessContext* readContext, volatile struct ProcessContext* writeContext){
	saveContext(readMem, readContext);
	
	restoreContext(writeMem, writeContext);
}

void contextChanger(volatile int registers[19]){
	if(processScheduler.inProcess != 1)
		changeContextToObject(registers);
	else
		changeContextToSystem(registers);
}

enum SchedulerResult createProcess(char* filename, struct ProcessSettings* settings){
	struct Object obj = {0};
	obj.process.data = (char*)(PROGRAM_ENTRY);
	if(loadProcessFromElfFile(filename, &obj.process) != LOAD_ELF_OK)
		return SCHEDULER_CANT_LOAD;
	obj.settings = *settings;
	return addObject(&obj);
}

enum SchedulerResult saveNewObject(struct Object* obj, int id){
	obj->objId = id;
	obj->process.context.spStart = 0x20005000;
	obj->process.context.sp = obj->process.context.spStart - 0x40;
	obj->process.context.xPSR = 0x01000000;
	obj->process.context.processorContext.SHCSR = 0x400;
	obj->state = WORK;
	return saveObject(obj,FATFS_storage)==SAVE_OBJECT_OK?SCHEDULER_OK:SCHEDULER_CANT_SAVE;
}

enum SchedulerResult addObject(struct Object* obj){
	FILINFO finfo;
	for(int newpid=0;newpid < processScheduler.lastProcess; newpid++){
		char fname[30];
		sprintf(fname, "%s/%d",PROC_DIR,newpid);
		FRESULT res = f_stat(fname,&finfo);
		if(res == FR_NO_FILE){
			return saveNewObject(obj, newpid);
		}
	}
	if(processScheduler.lastProcess == 0x300)
		return SCHEDULER_NO_AVAILABLE_PID;
	processScheduler.lastProcess ++;
	return saveNewObject(obj, processScheduler.lastProcess-1);
}

enum SchedulerResult deleteObject(struct Object* obj){
	FIL file;
	char fname[30];
	sprintf(fname, "%s/%d",PROC_DIR,obj->objId);
	f_unlink(fname);
	if(obj->objId == processScheduler.lastProcess-1){
		processScheduler.lastProcess--;
	}
	return SCHEDULER_OK;
}

enum SchedulerResult runProcesses(){
	DIR dir;
	FILINFO fileinfo;
	FRESULT opendirRes = f_opendir(&dir, PROC_DIR);
	if(opendirRes != FR_OK)
		return SCHEDULER_CANT_LOAD;
	struct Object obj;
	enum SchedulerResult schedulerResult;
	while(f_readdir(&dir,&fileinfo) == FR_OK){
		if(fileinfo.fname[0] == 0) break;
		obj.objId = atoi(fileinfo.fname);
		schedulerResult = changeActiveObject(&obj);
		if(schedulerResult != SCHEDULER_OK){
			f_closedir(&dir);
			return schedulerResult;
		}
		if(obj.state == WAITING)
			continue;
		enterProcess();
	}
	f_closedir(&dir);
	return SCHEDULER_OK;
}
