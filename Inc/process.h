#ifndef PROCESS_H
#define PROCESS_H
#include "process_context.h"
struct Process {
	char* data; // выделенная под процесс память
	struct ProcessContext context;
};
#endif