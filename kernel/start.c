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
#include <n7OS/keyboard.h>
#include <stdlib.h>
#include <stdbool.h>
#include <n7OS/printk.h>
#include <n7OS/mini_shell.h>

extern void processus1();
extern void mini_shell_process();

void idle() {
    // code de idle
    printf("Idle process started\n");

    printf("Processus pere en cours d'exécution...\n");
    printf("PID du processus pere : %d\n", getpid());

    // Démarrer le processus normal
    pid_t pidfils = spawn(processus1, "Processus 1");
    if (pidfils == -1) {
        printf("Erreur lors de la création du processus fils\n");
    } else {
        printf("Processus fils cree avec PID %d\n", pidfils);
        activer(pidfils);
    }
    
    // Démarrer le mini-shell comme un processus
    pid_t shell_pid = spawn(mini_shell_process, "Mini-Shell");
    if (shell_pid == -1) {
        printf("Erreur lors de la création du processus mini-shell\n");
    } else {
        printf("Mini-shell démarré avec PID %d\n", shell_pid);
        activer(shell_pid);
    }

    // Démo vfork (conservée pour démonstratio)
    pid_t v = vfork();
    if (v == 0) {
        // je suis l'enfant
        execve(processus1);
    } else {
        // je suis le parent, j'attends que l'enfant exec/exit
        printf("Je suis le parent du processus %d\n", v);
    }



    while (1) {
        hlt(); // Met le CPU en attente
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
    // alloc_page_entry(0xA0000000, 0, 0);
    // uint32_t *test = (uint32_t *) 0xA0000000;
    // uint32_t do_page_fault = *test;
    // do_page_fault ++;
    
    // Initialisation des appels systèmes
    init_syscall();

    // initialisation de l'horloge
    init_time();
    
    // initialisation du clavier
    init_keyboard();
    
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