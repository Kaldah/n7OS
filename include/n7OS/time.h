#ifndef N7OS_TIME_H
#define N7OS_TIME_H

#include <inttypes.h>
#include <stdio.h>
#include <n7OS/processus.h>

#define HORLOGE 1000 // HZ pour avoir une interruption par milliseconde
#define f_osc 0x1234BD // 1,19 MHz 
#define FREQUENCE f_osc/HORLOGE // 1,19 MHz 

void init_time();
void handler_en_C_TIMER();
uint32_t get_time();
char *get_time_string();
void suspend_process_timer(pid_t pid, uint32_t ms);
#endif // N7OS_TIME_H