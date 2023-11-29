#ifndef FATFS_EXTEND_H
#define FATFS_EXTEND_H
#include <ff.h>
#include <stdbool.h>
#include <string.h>
FRESULT f_recursiveRemoveDir(const TCHAR* path);
#endif