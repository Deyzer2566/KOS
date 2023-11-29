#include "Scheduler.h"
#include "process_loader.h"
#include "fatfs_extend.h"
#include "ContextSwitcher.h"
#include <stdlib.h>
#include <stdio.h>

TCHAR* PROC_DIR = _T("KOS/proc");

enum SchedulerResult saveProcess(struct Process* proc, struct ProcessSettings* settings);
enum SchedulerResult loadProcess(struct Process* proc, struct ProcessSettings* settings);
volatile struct Scheduler processScheduler={0};
enum SchedulerResult initializeScheduler(){
	FILINFO filInfo;
	FRESULT res=f_stat(PROC_DIR, &filInfo);
	if(res == FR_OK) {
		res = f_recursiveRemoveDir(PROC_DIR);
	}
	if(res == FR_OK || res == FR_NO_FILE || res == FR_NO_PATH)
		res = f_mkdir(PROC_DIR);
	if(res == FR_OK)
		return SCHEDULER_OK;
	else
		return SCHEDULER_CANT_INIT;
}

enum SchedulerResult changeActiveProcess(struct Process* proc, struct ProcessSettings* procSettings){
	enum SchedulerResult res = loadProcess(proc, procSettings);
	if(res != SCHEDULER_OK){
		deleteProcess(proc);
		return res;
	}
	
	processScheduler.curProcess = proc;
	processScheduler.curProcessSettings = procSettings;
	return SCHEDULER_OK;
}

void enterProcess(){
	changeContext();
}

void changeContextToProcess(int registers[16]){
	saveContext(registers, &processScheduler.mainContext);
	
	__asm volatile("MSR PSP, %0"::"r"(processScheduler.curProcess->context.sp-16));
	registers = processScheduler.curProcess->context.sp-16;
	
	restoreContext(registers, &processScheduler.curProcess->context);
	processScheduler.inProcess=1;
}

void changeContextToSystem(int registers[16]){
	saveContext(registers, &processScheduler.curProcess->context);
	
	registers = processScheduler.mainContext.sp-16;
	
	restoreContext(registers, &processScheduler.mainContext);
	processScheduler.inProcess=0;
}

void contextChanger(int registers[16]){
	if(!processScheduler.inProcess)
		changeContextToProcess(registers);
	else
		changeContextToSystem(registers);
}

enum SchedulerResult createProcess(char* filename, struct ProcessSettings* settings){
	struct Process proc = {0};
	proc.data = (char*)(PROGRAM_ENTRY);
	if(loadProcessFromElfFile(filename, &proc) != LOAD_OK)
		return SCHEDULER_CANT_LOAD;
	addProcess(&proc, settings);
	return SCHEDULER_OK;
}

enum SchedulerResult saveNewProcess(struct Process* proc, struct ProcessSettings* settings, int id){
	proc->id = id;
	proc->context.spStart = (int*)0x20002500;
	proc->context.sp = proc->context.spStart;
	__asm volatile("MRS %0, xPSR":"=r"(proc->context.xPSR));
	return saveProcess(proc, settings);
}

enum SchedulerResult addProcess(struct Process* proc, struct ProcessSettings* settings){
	FILINFO finfo;
	for(int newpid=0;newpid < processScheduler.lastProcess; newpid++){
		char fname[30];
		sprintf(fname, "%s/%d",PROC_DIR,newpid);
		FRESULT res = f_stat(fname,&finfo);
		if(res == FR_NO_FILE){
			return saveNewProcess(proc, settings, newpid);
		}
	}
	if(processScheduler.lastProcess == 0x300)
		return SCHEDULER_NO_AVAILABLE_PID;
	processScheduler.lastProcess ++;
	return saveNewProcess(proc, settings, processScheduler.lastProcess-1);
}

enum SchedulerResult saveProcess(struct Process* proc, struct ProcessSettings* settings){
	FIL file;
	char fname[30];
	sprintf(fname, "%s/%d",PROC_DIR,proc->id);
	f_open(&file, fname, FA_CREATE_ALWAYS | FA_WRITE);
	
	UINT wrote;
	f_write(&file, proc, sizeof(struct Process), &wrote);
	if(wrote != sizeof(struct Process)){
		f_close(&file);
		return SCHEDULER_CANT_SAVE;
	}
	
	f_write(&file, proc->data, (int)proc->context.spStart - (int)(proc->data) + 1, &wrote);
	if(wrote != (int)proc->context.spStart - (int)(proc->data) + 1){
		f_close(&file);
		return SCHEDULER_CANT_SAVE;
	}
	f_write(&file, settings, sizeof(struct ProcessSettings), &wrote);
	f_close(&file);
	if(wrote != PROGRAM_SIZE)
		return SCHEDULER_CANT_SAVE;
	return SCHEDULER_OK;
}

enum SchedulerResult loadProcess(struct Process* proc, struct ProcessSettings* settings){
	FIL file;
	char fname[30];
	sprintf(fname, "%s/%d",PROC_DIR,proc->id);
	f_open(&file, fname, FA_READ);
	
	UINT read;
	f_read(&file, proc, sizeof(struct Process),&read);
	if(read != sizeof(struct Process)){
		f_close(&file);
		return SCHEDULER_CANT_LOAD;
	}
	
	f_read(&file, proc->data, (int)proc->context.spStart - (int)(proc->data) + 1, &read);
	if(read != (int)proc->context.spStart - (int)(proc->data) + 1){
		f_close(&file);
		return SCHEDULER_CANT_LOAD;
	}
	
	f_read(&file, settings, sizeof(struct ProcessSettings), &read);
	f_close(&file);
	if(read != sizeof(struct ProcessSettings))
		return SCHEDULER_CANT_SAVE;
	return SCHEDULER_OK;
}

enum SchedulerResult deleteProcess(struct Process* proc){
	FIL file;
	char fname[30];
	sprintf(fname, "%s/%d",PROC_DIR,proc->id);
	f_unlink(fname);
	if(proc->id == processScheduler.lastProcess-1){
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
	struct Process proc;
	struct ProcessSettings settings;
	proc.data = (char*)PROGRAM_ENTRY;
	while(f_readdir(&dir,&fileinfo) == FR_OK){
		if(fileinfo.fname[0] == 0) break;
		proc.id = atoi(fileinfo.fname);
		changeActiveProcess(&proc, &settings);
		enterProcess();
		saveProcess(&proc, &settings);
	}
	f_closedir(&dir);
	return SCHEDULER_OK;
}
