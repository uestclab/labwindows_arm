#ifndef EXCEPTION_H
#define EXCEPTION_H
#include "zlog.h"

void InitTimer(zlog_category_t * log_handler);
void StartTimer();
void StopTimer();
void closeTimer();

int check_variable(int cmd, int value);

#endif//EXCEPTION_H
