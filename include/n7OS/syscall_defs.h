#ifndef __SYSCALL_DEFS_H__
#define __SYSCALL_DEFS_H__

#define NB_SYSCALL 7

#include <n7OS/processus.h>

int sys_example();
int sys_shutdown(int n);
int sys_write(const char *s, int len);
int sys_fork();
int sys_get_pid();
int sys_exit();
int sys_sleep(int seconds);

typedef int (*fn_ptr)();
extern fn_ptr syscall_table[NB_SYSCALL];

void add_syscall(int num, fn_ptr function);

#endif
