#include <stdio.h>
#include <n7OS/processus.h>
#include <stdbool.h>
#include <n7OS/cpu.h>
#include <n7OS/time.h> // Add for get_time function

// Forward declaration of functions used in this file
void terminer(pid_t pid);

void processus1() {
  pid_t pid = getpid(); // Récupère le PID du processus
  printf("Hello, world from P1 (PID=%d)\n", pid);
  
  while (1) {
    // printf("Processus 1 ========================================\n");

    while (get_time() % 5000 != 0) {
      printf("Processus 1 : PID=%d, Time=%u\n", pid, get_time());
      // Attendre jusqu'à la prochaine seconde
      hlt();
    }
    arreter(); // Arrête le processus
  }

  terminer(pid); // Termine le processus
}
