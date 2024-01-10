#ifndef PROCESS_CONTEXT_H
#define PROCESS_CONTEXT_H
#include "ProcessorContext.h"
struct ProcessContext{
	int r[13];
	int lr;
	int pc;
	int sp;
	int xPSR;
	int spStart; // значение SP при создании процесса
	struct ProcessorContext processorContext;
};
#endif