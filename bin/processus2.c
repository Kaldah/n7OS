#include <stdio.h>
#include <n7OS/processus.h>
#include <stdbool.h>
#include <n7OS/cpu.h>
#include <n7OS/time.h> // Add for get_time function
#include <n7OS/sys.h>
#include <unistd.h>
#include <n7OS/console.h>


void processus2() {
  pid_t pid = getpid(); // Récupère le PID du processus
  sleep(3);
  printf("Hello, world from P2 (PID=%d)\n", pid);
  sleep(3); // Sleep for 3 seconds
  scroll_up();
  sleep(3);
  // printf("Processus P2 (PID=%d) is scrolling down...\n", pid);
  // sleep(1);
  scroll_down();
    
  while (1) {
    hlt(); // Processus en attente, simule un travail
  }
  exit();
}
