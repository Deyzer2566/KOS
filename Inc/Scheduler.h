#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <ff.h>
#include "process.h"
#include "process_settings.h"
#include "ProcessorContext.h"
#include "Object.h"
#include <stddef.h>
enum SchedulerResult{
	SCHEDULER_OK, SCHEDULER_CANT_SAVE, SCHEDULER_CANT_LOAD,
	SCHEDULER_NO_AVAILABLE_PID, SCHEDULER_CANT_INIT
};
struct Scheduler{
	int lastProcess;
	int inProcess;
	struct ProcessContext mainContext;
	struct Object* curObject;
	char* handlerData;
	size_t handlerDataSize;
	struct ProcessorContext mainProcContext;
};

enum SchedulerResult initializeScheduler();
enum SchedulerResult createProcess(char* filename, struct ProcessSettings* settings);
enum SchedulerResult runProcesses();
enum SchedulerResult addObject(struct Object* obj);
#endif