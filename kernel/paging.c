#include <n7OS/paging.h>
//#include <stddef.h> // nécessaire pour NULL
#include <string.h>
#include <stdio.h>
#include <n7OS/kheap.h>
#include <n7OS/mem.h>

#define ACTIVATE_PAGING MEMORY_32_BITS_LENGTH

PageDirectory page_directory;
int32_t compteur_pages = 0;

void loadPageDirectory(unsigned int *page_directory) {
    __asm__ __volatile__("mov %0, %%cr3":: "r"(page_directory));
}

uint32_t lire_cr3() {
    uint32_t cr3;
    __asm__ __volatile__("mov %%cr3, %0": "=r"(cr3));
    return cr3;
}

void enablePaging() {
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= ACTIVATE_PAGING;
    __asm__ __volatile__("mov %0, %%cr0":: "r"(cr0));
}

uint32_t lire_cr0() {
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0": "=r"(cr0));
    return cr0;
}

void setPageEntry(PTE *page_table_entry, uint32_t new_page, int is_writeable, int is_kernel) {
    page_table_entry->page_entry.P = 1;
    page_table_entry->page_entry.A = 0;
    page_table_entry->page_entry.D = 0;
    page_table_entry->page_entry.RW = is_writeable;
    page_table_entry->page_entry.U= ~is_kernel;
    page_table_entry->page_entry.page= new_page>>12;
}

PageDirectory initialise_paging() {

    uint32_t last_index = 0;

    // On initialise la mémoire
    init_mem();

    // On initialise le répertoire de pages
    page_directory =  (PageDirectory) kmalloc_a(PAGE_SIZE);
    // On nettoie la mémoire du répertoire de pages
    memset(page_directory, 0, PAGE_SIZE);
    
    printf("Page Directory adresse : %x\n", (uint32_t) page_directory);
    
    // On initialise les tables de pages
    for (int i = 0; i < TABLE_NUMBER; i++) {

        // On alloue une table de page
        PageTable page_table= (PageTable) kmalloc_a(PAGE_SIZE);
        // On nettoie la mémoire de la table de page
        memset(page_table, 0, PAGE_SIZE);
        page_directory_entry_t page_directory_entry;
        // On marque la page comme accessible en mode noyau
        page_directory_entry.U = 0;
        // On marque la page comme accessible en écriture
        page_directory_entry.RW = 1;
        // On marque la page comme présente en mémoire
        page_directory_entry.P = 1;
        // On donne l'adresse de la table de page
        page_directory_entry.page_table = (uint32_t) page_table >> 12;

        // On initialise l'entrée du répertoire de page
        page_directory[i] = (PDE) page_directory_entry;

        last_index= (uint32_t) page_table + PAGE_SIZE;
    }

    printf("Dernière adresse de la table de page : %x\n", last_index);
    // On initialise la table de page avec des pages libres
    for (uint32_t i= 0; i<last_index; i += PAGE_SIZE) {
        alloc_page_entry(i, 1, 1);
    }

    printf("Nombre de pages allouées : %d\n", compteur_pages);

    loadPageDirectory((unsigned int *)page_directory);
    uint32_t cr3 = lire_cr3();
    printf("cr3 : %x\n", cr3);

    uint32_t cr0 = lire_cr0();
    printf("cr0 : %x\n", cr0);

    enablePaging();

    cr0 = lire_cr0();
    printf("cr0 : %x\n", cr0);

    printf("page directory : %x\n", (uint32_t) page_directory);

    return page_directory;
}

PageTable alloc_page_entry(uint32_t address, int is_writeable, int is_kernel) {
    // address = adresse virtuelle à allouer 
    // address = idx_PDE | idx_PTE | offset
    //             10    |    10   |   12

    // On garde les 10 premiers bits
    int idx_pagedir = (address >> 22); // calcul de la position de la table de page dans le répertoire de page
    
    PageTable page_table;

    PDE page_dir_entry = (PDE) page_directory[idx_pagedir]; // on recupere l'entree dans le répertoire de page
    // une entree contient l'adresse de la table de page + bits de controle
    
    page_table = (PTE *) (page_dir_entry.page_entry.page_table << 12); // on recupere l'adresse de la page de table dans le répertoire de page

    int32_t phy_page = findfreePage(); // recherche d'une page libre dans la memoire physique
    if (phy_page == -1) {
        printf("Erreur : pas de page libre\n");
        return NULL;
    } else {
        compteur_pages++;
        //printf("Page allouée : %x\n", phy_page);
    }

    int idx_pagetab = ((address & 0x003FF000) >> 12); // calcul de la position de la page dans la table de page
    
    // mise a jour de l'entree dans la page de table
    setPageEntry(&page_table[idx_pagetab], phy_page, is_writeable, is_kernel);

    return page_table;
}
