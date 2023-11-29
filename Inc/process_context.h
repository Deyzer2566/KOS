#ifndef PROCESS_CONTEXT_H
#define PROCESS_CONTEXT_H
struct ProcessContext{
	int r[13];
	int lr;
	int pc;
	int* sp;
	int xPSR;
	int* spStart; // значение SP при запуске процесса
};
#endif