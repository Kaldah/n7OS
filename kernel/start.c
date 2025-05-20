#include <n7OS/cpu.h>
#include <inttypes.h>
#include <n7OS/processor_structs.h>
#include <n7OS/console.h>
#include <n7OS/start.h>
#include <stdio.h>
#include <n7OS/mem.h>
#include <n7OS/paging.h>
#include <n7OS/kheap.h>
#include <n7OS/irq.h>
#include <n7OS/time.h>
#include <n7OS/sys.h>
#include <unistd.h>
#include <n7OS/processus.h>
#include <stdlib.h>
#include <stdbool.h>

extern void processus1();

void idle() {
    // code de idle
    printf("Idle process started\n");

    pid_t pid = fork();
    if (pid == 0) {
        // Code du processus fils
        printf("Processus fils cree avec PID %d\n", getpid());
        while (1) {
            processus1();
        }
    } else if (pid > 0) {
        // Code du processus père
        printf("Processus pere en cours d'exécution...\n");
        printf("PID du processus pere : %d\n", getpid());
        printf("PID du processus fils : %d\n", pid);
        display_scheduler_state();
        // Boucle infinie pour le processus père
        while (1) {
            while (get_time() % TIME_SLOT/2 != 0) {
                hlt();
            }
        }
    }

    // Ne doit jamais sortir de idle
    terminer(getpid()); // Termine le processus
}


void kernel_start(void)
{
    init_console();

    // printf("===Initialisation de la mémoire===\n");

    kmalloc_init();

    // printf("===Initialisation de la pagination===\n");
    uint32_t page_directory = (uint32_t) initialise_paging();
    
    setup_base(page_directory);
    // print_mem(4096);

    // Test de la pagination
    alloc_page_entry(0xA0000000, 0, 0);
    uint32_t *test = (uint32_t *) 0xA0000000;
    uint32_t do_page_fault = *test;
    do_page_fault ++;
    
    // Initialisation des appels systèmes
    init_syscall();

    // initialisation de l'horloge
    init_time();
    
    // lancement des interruptions
    sti();

    // Create the idle process first
    printf("Création du processus Idle\n");
    pid_t pidmain = create(idle, "Idle process");
    
    // Set the idle process as active
    activer(pidmain);

    if (example() == 1) {
        printf("=== Appel système example réussi ===\n");
    } else {
        printf("=== Appel système example échoué ===\n");
    }

    schedule();

    // Start the idle process
    idle();

    // This should never be reached
    shutdown(0);
}