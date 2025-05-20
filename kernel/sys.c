#include <n7OS/sys.h>
#include <n7OS/syscall_defs.h>
#include <n7OS/console.h>
#include <n7OS/irq.h>
#include <unistd.h>
#include <n7OS/cpu.h>
#include <n7OS/processus.h>
#include <n7OS/time.h>
#include <string.h>

extern void handler_syscall();

void suspend_process_timer(pid_t pid, uint32_t ms);

void init_syscall() {
  // ajout de la fonction de traitement de l'appel systeme
  add_syscall(NR_example, sys_example);
  add_syscall(NR_shutdown, sys_shutdown);
  add_syscall(NR_write, sys_write);
  add_syscall(NR_fork, sys_fork);
  add_syscall(NR_get_pid, sys_get_pid);
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

int sys_fork() {
  extern struct process_t *current_process;
  extern struct process_t *process_table[NB_PROC];
  
  printf("=== Fork started ===\n");
  
  // Get the parent process ID
  pid_t ppid = current_process->pid;
  
  // Create a new process (but don't activate it yet)
  pid_t pid_child = create(dummy_process, current_process->name);
  
  struct process_t *child = process_table[pid_child];
  struct process_t *parent = process_table[ppid];
  
  // FULL stack copy from parent to child
  // This is the actual memory copy of the stack contents
  memcpy(child->stack, parent->stack, STACK_SIZE * sizeof(uint32_t));
  
  // Calculate direct stack address offset between parent and child
  int32_t stack_offset = (int32_t)child->stack - (int32_t)parent->stack;
  
  // Adjust ESP (stack pointer) - add the direct offset to point to the same relative position
  child->context.s.esp = parent->context.s.esp + stack_offset;
  
  // Adjust EBP (base pointer) only if it's pointing to the parent's stack
  uint32_t parent_stack_top = (uint32_t)parent->stack + STACK_SIZE * sizeof(uint32_t);
  if (parent->context.s.ebp >= (uint32_t)parent->stack && parent->context.s.ebp < parent_stack_top) {
    // EBP points within the stack area, so adjust it with the stack offset
    child->context.s.ebp = parent->context.s.ebp + stack_offset;
  } else {
    // EBP doesn't point to stack (unusual but possible), keep it as is
    child->context.s.ebp = parent->context.s.ebp;
  }
  
  // Set child's EBX to 0 - this is what will make fork() return 0 in the child process
  child->context.s.ebx = 0;
  
  // Copy other registers
  child->context.s.esi = parent->context.s.esi;
  child->context.s.edi = parent->context.s.edi;
  
  // Now activate the child process so it can be scheduled
  activer(pid_child);
  
  printf("=== Forked process PID: %d (parent: %d) ===\n", pid_child, ppid);
  
  // Return child PID to the parent process
  return pid_child;
}


int sys_exit() {
  terminer(getpid());
  return 0;
}

int sys_get_pid() {
  return getpid();
}

int sys_sleep(int seconds) {
  suspend_process_timer(getpid(), seconds * 1000); // Convert seconds to milliseconds
  return 0;
}