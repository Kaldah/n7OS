#include <stdio.h>
#include <n7OS/processus.h>
#include <stdbool.h>
#include <n7OS/cpu.h>
#include <n7OS/time.h> // Add for get_time function
#include <n7OS/processus.h>
#include <n7OS/sys.h>


void processus1() {
  pid_t pid = getpid(); // Récupère le PID du processus

  printf("Hello, world from P1 (PID=%d)\n", pid);
  sleep(2); // Sleep for 2 seconds
  // display_scheduler_state(); // Affiche l'état du planificateur
  printf("Processus 1 (PID=%d) woke up after 2 seconds\n", pid);

  terminer(pid); // Termine le processus
}
