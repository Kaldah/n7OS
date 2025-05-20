#include <n7OS/sys.h>
#include <n7OS/syscall_defs.h>
#include <n7OS/console.h>
#include <n7OS/irq.h>
#include <unistd.h>
#include  <n7OS/cpu.h>
#include <n7OS/processus.h>
#include <n7OS/time.h>

extern void handler_syscall();

void init_syscall() {
  // ajout de la fonction de traitement de l'appel systeme
  add_syscall(NR_example, sys_example);
  add_syscall(NR_shutdown, sys_shutdown);
  add_syscall(NR_write, sys_write);
  add_syscall(NR_fork, sys_fork);
  add_syscall(NR_getpid, sys_getpid);
  add_syscall(NR_exit, sys_exit);
  add_syscall(NR_sleep, sys_sleep);

  // initialisation de l'IT soft qui gère les appels systeme
  init_irq_entry(0x80, (uint32_t) handler_syscall);
}

// code de la fonction de traitement de l'appel systeme example
int sys_example() {
  // on ne fait que retourner 1
  return 1;
}

// code de la fonction de traitement de l'appel systeme shutdown
int sys_shutdown (int n) {
  if (n == 1) {
    outw (0x2000 , 0x604); // Poweroff qemu > 2.0
    return -1;
  } else return n;
}

// code de la fonction de traitement de l'appel systeme write
int sys_write(const char *s, int len) {
  console_putbytes(s, len);
  return len;
}

pid_t sys_fork() {
  printfk("=== Fork ===\n");
  // Create a new process
  pid_t ppid = current_process->pid; // Get the parent process ID
  pid_t pid_proc = create(dummy_process, current_process->name); // Create a new process
  activer(pid_proc); // Activate the new process
  
  // Copy the current process's state to the new process
  process_table[pid_proc]->regs = process_table[ppid]->regs; // Copy registers
  printfk("=== Forked process PID: %d ===\n", pid_proc);

  return pid_proc;
}


int sys_exit() {
  terminer(getpid());
  return 0;
}

pid_t sys_getpid() {
  return getpid();
}

void sys_sleep(int seconds) {
  suspend_process_timer(getpid(), seconds * 1000); // Convert seconds to milliseconds
}