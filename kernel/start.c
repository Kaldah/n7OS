#include <n7OS/cpu.h>
#include <inttypes.h>
#include <n7OS/processor_structs.h>
#include <n7OS/console.h>

void kernel_start(void)
{
    init_console();
    setup_base(0 /* la memoire virtuelle n'est pas encore definie */);

    // lancement des interruptions
    sti();

    // affichage d'un caractere

    printf("\f");
    printf("===Initialisation de la mémoire===\n");

    kmalloc_init();

    init_mem();
    print_mem(32);

    printf("===Initialisation de la pagination===\n");
    initialise_paging();
    alloc_page_entry(0x1000, 1, 1);

    printf("Test de la pagination\n");
    printf("Autres tests\n");

    // on ne doit jamais sortir de kernel_start
    while (1) {
        // cette fonction arrete le processeur
        hlt();
    }
}
