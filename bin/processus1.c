#include <stdio.h>
#include <n7OS/processus.h>
#include <stdbool.h>
#include <n7OS/cpu.h>
#include <n7OS/time.h> // Add for get_time function
#include <n7OS/processus.h>
#include <n7OS/sys.h>
#include <unistd.h>


void processus1() {
  pid_t pid = getpid(); // Récupère le PID du processus

  printf("Hello, world from P1 (PID=%d)\n", pid);
  sleep(1); // Sleep for 1 seconds
  printf("Processus 1 (PID=%d) woke up after 1 seconds\n", pid);
  exit();
}
