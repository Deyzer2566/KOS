#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <ff.h>
#include "process.h"
#include "process_settings.h"
enum SchedulerResult{
	SCHEDULER_OK, SCHEDULER_CANT_SAVE, SCHEDULER_CANT_LOAD,
	SCHEDULER_NO_AVAILABLE_PID, SCHEDULER_CANT_INIT
};
struct Scheduler{
	int lastProcess;
	struct Process* curProcess;
	struct ProcessSettings* curProcessSettings;
	int inProcess;
	struct ProcessContext mainContext;
};

enum SchedulerResult initializeScheduler();
enum SchedulerResult createProcess(char* filename, struct ProcessSettings* settings);
enum SchedulerResult addProcess(struct Process* proc, struct ProcessSettings* settings);
enum SchedulerResult deleteProcess(struct Process* proc);
enum SchedulerResult runProcesses();
#endif