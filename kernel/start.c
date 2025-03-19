#include <n7OS/cpu.h>
#include <inttypes.h>
#include <n7OS/processor_structs.h>
#include <n7OS/console.h>
#include <n7OS/start.h>
#include <stdio.h>
#include <n7OS/mem.h>
#include <n7OS/paging.h>
#include <n7OS/kheap.h>

void kernel_start(void)
{
    init_console();
    //setup_base(0 /* la memoire virtuelle n'est pas encore definie */);

    extern PageDirectory page_directory;
    setup_base((uint32_t)&page_directory);
    //setup_base(0);

    // lancement des interruptions
    sti();

    // affichage d'un caractere
    printf("\f");
    printf("===Initialisation de la mémoire===\n");

    kmalloc_init();

     // init_mem();
    // uint32_t page = findfreePage();
    // printf("Page allouée : %x\n", page);
    // page = findfreePage();
    // printf("Page allouée : %x\n", page);
    // printf("\n");
    // printf(findfreePage());
    // printf("\n");

    printf("===Initialisation de la pagination===\n");
    initialise_paging();
    //alloc_page_entry(0x1000, 1, 1);

    printf("Test de la pagination\n");
    //print_mem(4096);
    printf("Autres tests\n");

    // on ne doit jamais sortir de kernel_start
    while (1) {
        // cette fonction arrete le processeur
        hlt();
    }
}
