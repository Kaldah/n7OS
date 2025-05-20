#include <stdio.h>
// #include <n7OS/processus.h>
#include <stdbool.h>
#include <n7OS/cpu.h>
#include <n7OS/time.h> // Add for get_time function
#include <n7OS/processus.h>

// Forward declaration of functions used in this file
void terminer(pid_t pid);

void processus1() {

  pid_t pid = getpid(); // Récupère le PID du processus
  printf("Hello, world from P1 (PID=%d)\n", pid);
  
  while (1) {
    // printf("Processus 1 ========================================\n");
    while (get_time() % TIME_SLOT/2 != 0) {
      hlt();
    }
  }

  terminer(pid); // Termine le processus
}
