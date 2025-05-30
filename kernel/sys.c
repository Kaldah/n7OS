#include <n7OS/sys.h>
#include <n7OS/syscall_defs.h>
#include <n7OS/console.h>
#include <n7OS/irq.h>
#include <unistd.h>
#include <n7OS/cpu.h>
#include <n7OS/processus.h>
#include <n7OS/time.h>
#include <string.h>
#include <stdio.h>

extern void handler_syscall();
extern struct process_t *current_process;
extern struct process_t process_table[NB_PROC];
extern uint32_t process_stacks[NB_PROC][STACK_SIZE];

void suspend_process_timer(pid_t pid, uint32_t ms);

void init_syscall() {
  // ajout de toutes les fonctions de traitement des appels systemes
  add_syscall(NR_example, sys_example);
  add_syscall(NR_shutdown, sys_shutdown);
  add_syscall(NR_write, sys_write);
  add_syscall(NR_fork, sys_fork);
  add_syscall(NR_get_pid, sys_get_pid);
  add_syscall(NR_exit, sys_exit);
  add_syscall(NR_sleep, sys_sleep);
  add_syscall(NR_spawn, sys_spawn);
  add_syscall(NR_execve, sys_execve);
  add_syscall(NR_vfork, sys_vfork);

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
    outw (0x2000 , 0x604); // envoi de la commande de shutdown au BIOS
    return -1;
  } else return n;
}

// code de la fonction de traitement de l'appel systeme write
int sys_write(const char *s, int len) {
  console_putbytes(s, len);
  return len;
}

int sys_spawn(void *entry_point, const char *name) {
    // crée un processus sans copier la mémoire du parent
    pid_t pid = create(entry_point, (char*)name);
    activer(pid);
    return pid;
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

// Fonctions permettant d'implanter le fork UNIX
// J'ai encore besoin de faire des recherches
// pour savoir comment faire un fork UNIX correct sur N7OS

// TODO:give the name of the command to execute instead of the entry point address
// Signature based on the Linux execve
int sys_execve(const char *filename, char *const argv[], char *const envp[]) {
    /* Avoid unused parameter warnings */
    (void)filename;
    (void)argv;
    (void)envp;

    pid_t pid = getpid();
    struct process_t *p __attribute__((unused)) = &process_table[pid];

    if (process_table[pid].pid == 0) {
        printf("Error: process not found for pid %d\n", pid);
        return -1; // Process not found
    }
    if (process_table[pid].ppid > 0) {
      wakeup_process(process_table[pid].ppid);
    } else {
      // au prochain schedule(), ce processus repartira sur entry_point
      schedule();
    }
    // ne devrait jamais revenir ici
    return -1;
}

int sys_vfork() {
    pid_t parent_pid = getpid();
    
    // créer un enfant qui partage la même stack
    pid_t child_pid = create(resume_child_process, process_table[parent_pid].name);
    
    // partage pur de la pile et du contexte
    process_table[child_pid].stack = process_table[parent_pid].stack;
    process_table[child_pid].context = process_table[parent_pid].context;

    // suspendre le parent jusqu'à exec/exit de l'enfant
    suspendre(parent_pid);
    // le parent est suspendu, l'enfant est actif

    // démarrage immédiat de l'enfant
    activer(child_pid);
    schedule();

    // quand l'enfant fera execve() ou sys_exit(), il devra
    // réactiver le parent via wakeup_process(parent_pid);
    return child_pid;  // dans le parent, retourne pid de l'enfant
}


int sys_fork() {
  printf("=== Fork started ===\n");

  uint32_t current_parent_esp;
  __asm__ volatile (
    "movl %%esp, %0"
    : "=r" (current_parent_esp)
    : /* no inputs */
    : "memory"
  );

  printf("=== Current parent ESP: %x ===\n", current_parent_esp);
  // Get the parent process ID
  pid_t ppid = current_process->pid;
  
  // Create a new process (but don't activate it yet)
  pid_t pid_child = create(resume_child_process, current_process->name);

  uint32_t line_size = sizeof(uint32_t);
  
  // FULL stack copy from parent to child
  // This is the actual memory copy of the stack contents
  memcpy(process_table[pid_child].stack, process_table[ppid].stack, STACK_SIZE * sizeof(uint32_t));
  process_table[ppid].context.s.esp = current_parent_esp; // Restore the parent's ESP

  // This is the offset between the top of the stack and the current ESP
  int32_t child_parent_offset = (uint32_t) process_table[pid_child].stack - (uint32_t) process_table[ppid].stack;
  printf("=== Stack offset: %d ===\n", child_parent_offset);

  // Adjust ESP (stack pointer) - add the direct offset to point to the same relative position
  process_table[pid_child].context.s.esp = child_parent_offset + (uint32_t) process_table[ppid].context.s.esp;

  // Calculate the stack index more safely
  uint32_t stack_start = (uint32_t)process_table[pid_child].stack;
  uint32_t stack_position = ((uint32_t)process_table[pid_child].context.s.esp - stack_start) / line_size;
  printf("=== Child stack position: %d ===\n", stack_position);
  // If needed to access the stack as an array:
  // uint32_t *stack_array = (uint32_t *)process_table[pid_child].stack;
  // stack_array[stack_position] = 0;

  printf("=== Child ESP: %x ===\n", process_table[pid_child].context.s.esp);
  printf("=== Parent ESP: %x ===\n", process_table[ppid].context.s.esp);
  printf("=== Child Stack: %x ===\n", (uint32_t) process_table[pid_child].stack);
  printf("=== Parent Stack: %x ===\n", (uint32_t) process_table[ppid].stack);

  // Now activate the child process so it can be scheduled
  activer(pid_child);
  
  printf("=== Forked process PID: %d (parent: %d) ===\n", pid_child, ppid);

  // Return child PID to the parent process
  return pid_child;
}