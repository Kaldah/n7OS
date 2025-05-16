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

extern void processus1();

void idle() {

    // code de idle
    printf("Idle process\n");
    pid_t pid_proc1 = create(processus1, "Processus 1");
    activer(pid_proc1); // on active le processus 1 
    schedule();

    for (int i = 0; i < 3; i++) {
        printf("Idle goes brr\n");

        arreter(); // cette fonction arrête le processeur élu
    }
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


    // // Exemple d'envoie d'une interruption 50
    // __asm__ volatile("int $50");

    // affichage de la console

    // On affiche le temps 0
    console_puts_time("00:00:00");
    
    // affichage d'une chaine de caracteres
    printf("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

    if (example() == 1) {
        printf ("Appel systeme example ok\n");
    }

    if (shutdown(2) == -1) {
        printf ("Appel systeme shutdown ok\n");
    }
    else {
        printf ("Appel systeme shutdown ko\n");
    }

    pid_t pidmain = create(idle, "Idle process");
    // on active le processus idle

    activer(pidmain); // on active le processus idle
    // On lance idle
    idle();

    // on ne doit jamais sortir de kernel_start
    while (1) {
        // cette fonction arrête le processeur
        hlt();
    }
}