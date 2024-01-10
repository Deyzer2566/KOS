#ifndef OBJECT_H
#define OBJECT_H
#include <stddef.h>
#include "process.h"
#include "process_settings.h"
#include <stdint.h>
#define OBJECT_NAME_LEN 32
#define OBJECT_METHOD_COUNT_MAX 4
struct Method{
	int (*pointerToFunc)(uint8_t*, size_t);
};
struct Object{
	char name[OBJECT_NAME_LEN];
	int objId;
	struct Process process;
	struct ProcessSettings settings;
	size_t methodsCount;
	struct Method methods[OBJECT_METHOD_COUNT_MAX];
	enum {WAITING, WORK} state;
};
#endif