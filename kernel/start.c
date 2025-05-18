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
#include <stdlib.h> // Add for shutdown function

extern void processus1();

// Forward declaration of functions used in this file
void display_scheduler_state(void);
void terminer(pid_t pid);

void idle() {
    // code de idle
    printf("Idle process started\n");
    schedule();
    // On ne doit jamais sortir de idle
    while (1) {
        printf("===============================Idle process time : %u\n", get_time());
        display_scheduler_state();

        while (get_time() % 5000 != 0) {
            // Attendre jusqu'à la prochaine seconde
            hlt();
        }

        arreter(); // Arrête le processus
        // Using hlt() instead of arreter() to prevent system overload
        // This pauses the CPU until the next interrupt
        // hlt();
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

    printf("Création processus 1\n");
    pid_t pid_proc1 = create(processus1, "Processus 1");
    activer(pid_proc1); // on active le processus 1 


    // Start the idle process
    idle();

    // This should never be reached
    while (1) {
        hlt();
    }
    // shutdown(0); // On ne doit jamais sortir de idle
}