#ifndef PROCESS_H
#define PROCESS_H
#include "process_context.h"
struct Process {
	char* data; // выделенная под процесс память
	int id; // ID процесса
	struct ProcessContext context;
};
#endif