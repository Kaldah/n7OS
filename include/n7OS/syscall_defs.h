#ifndef __SYSCALL_DEFS_H__
#define __SYSCALL_DEFS_H__

#define NB_SYSCALL 10

#include <n7OS/processus.h>

int sys_example();
int sys_shutdown(int n);
int sys_write(const char *s, int len);
int sys_fork();
int sys_get_pid();
int sys_exit();
int sys_sleep(int seconds);
int sys_spawn(void *entry_point, const char *name);
int sys_execve(const char *filename, char *const argv[], char *const envp[]);
int sys_vfork(void);

typedef int (*fn_ptr)();
extern fn_ptr syscall_table[NB_SYSCALL];

void add_syscall(int num, fn_ptr function);

#endif
