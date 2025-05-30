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
extern void processus2();
extern void mini_shell_process();

void idle() {
    // code de idle
    printf("Idle process started\n");

    printf("Processus pere en cours d'execution...\n");
    printf("PID du processus pere : %d\n", getpid());

    // Démarrer le processus 1 avec un spawn
    int pidfils = spawn(processus1, "Processus 1");
    if (pidfils == -1) {
        printf("Erreur lors de la creation du processus fils\n");
    } else {
        printf("Processus fils cree avec PID %d\n", pidfils);
    }

    // Démarrer le processus 2 avec un spawn
    pidfils = spawn(processus2, "Processus 2");
    if (pidfils == -1) {
        printf("Erreur lors de la creation du processus fils\n");
    } else {
        printf("Processus fils cree avec PID %d\n", pidfils);
    }
    
    // Attendre 5 secondes pour que les processus 1 et 2 finissent d'afficher des choses
    // A but visuel
    sleep(4);
    // Demarrer le mini-shell dans un processus
    int shell_pid = spawn(mini_shell_process, "Mini-Shell");
    if (shell_pid == -1) {
        printf("Erreur lors de la creation du processus mini-shell\n");
    } else {
        printf("Mini-shell demarre avec PID %d\n", shell_pid);
    }


    while (1) {
        hlt(); // Met le CPU en attente
        // TODO: Implanter une verification du minishell
        // Proposer de la relancer s'il est éteint
        // en testant l'input d'une touche
        // utiliser le shell_pid pour voir son état
    }

    // Ne doit jamais sortir de idle
    exit();
}


void kernel_start(void)
{
    init_console();

    // printfk("===Initialisation de la memoire===\n");

    kmalloc_init();

    // printfk("===Initialisation de la pagination===\n");
    uint32_t page_directory = (uint32_t) initialise_paging();
    

    setup_base(page_directory);
    // print_mem(4096);

    // Test de la pagination
    // alloc_page_entry(0xA0000000, 0, 0);
    // uint32_t *test = (uint32_t *) 0xA0000000;
    // uint32_t do_page_fault = *test;
    // do_page_fault ++;
    
    // Initialisation des appels systemes
    init_syscall();

    // initialisation de l'horloge
    init_time();
    
    // initialisation du clavier
    init_keyboard();
    
    // lancement des interruptions
    sti();

    // Create the idle process first
    printf("Creation du processus Idle\n");
    pid_t pidmain = create(idle, "Idle process");
    
    // Set the idle process as active
    activer(pidmain);

    if (example() == 1) {
        printf("=== Appel systeme example reussi ===\n");
    } else {
        printf("=== Appel systeme example echoue ===\n");
    }

    schedule();

    // Start the idle process who manages everything from this point
    idle();

    // This should never be reached
    shutdown(0);
}