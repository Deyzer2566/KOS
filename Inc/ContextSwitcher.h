#ifndef CONTEXT_SWITCHER_H
#define CONTEXT_SWITCHER_H

#include "process_context.h"

/*
Функции переключения контекста

Должны использоваться только в прерывании,
т.к. используют механизм восстановления контекста при выходе из прерывания

registers - указатель на сохраненные регистры, хранящиеся в следующем порядке:
R4-R11, R0-R3, R12, LR, PC, xPSR
*/

void saveContext(volatile int* registers, volatile struct ProcessContext* context);

void restoreContext(volatile int* registers, volatile struct ProcessContext* context);

void changeContext();

#endif
