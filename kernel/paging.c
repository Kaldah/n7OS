#include <n7OS/paging.h>
#include <stddef.h> // nécessaire pour NULL

PageDirectory page_directory;

void initialise_paging() {
    // On initialise le répertoire de pages
    PageDirectory page_directory = kmalloc_a(sizeof(PDE) * 1024);
    // On nettoie la mémoire du répertoire de pages
    memset(page_directory, 0, sizeof(PDE) * 1024);

    printf("Page Directory adresse : %x\n", page_directory);
    
    // On initialise les tables de pages
    for (int i = 0; i < 1024; i++) {
        // On alloue une table de page
        uint32_t page_table_adresse = kmalloc_a(sizeof(PTE) * 1024);

        // On initialise la table de page
        PageTable page_table = (PTE *) page_table_adresse;

        // On obtient l'adresse dont les 12 bits de poids faibles sont à 0
        int32_t page_adress = findfreePage();

        page_table_entry_t page_table_entry;
        // On marque l'adresse de la page dans la mémoire physique
        page_table_entry.page = page_adress >> 12;
        // On marque la page comme accessible en mode noyau
        page_table_entry.U = 0;
        // On marque la page comme accessible en écriture
        page_table_entry.W = 1;
        // On marque la page comme présente en mémoire
        page_table_entry.P = 1;

        // On initialise la table de page avec des pages libres
        for (int j = 0; j < 1024; j++) {
            // On initialise l'entrée de la table de page
            page_table_entry_t page_table_entry;

            // On marque l'adresse de la page dans la mémoire physique
            page_table_entry.page = page_adress >> 12;
            // Dirty Bit, page non modifiée lors de l'initialisation
            page_table_entry.D = 0;
            // Accessed bit, la page n'a pas été accédée lors de l'initialisation
            page_table_entry.A = 0;
            // On marque la page comme accessible en mode noyau
            page_table_entry.U = 0;
            // On marque la page comme accessible en écriture
            page_table_entry.W = 1;
            // On marque la page comme présente en mémoire
            page_table_entry.P = 1;

            page_table[j] = (PTE) page_table_entry ;
        }

        // On initialise l'entrée du répertoire de page
        page_directory[i] = (PDE) page_table_adresse;
    }
}

PageTable alloc_page_entry(uint32_t address, int is_writeable, int is_kernel) {
    PageTable pgtab = NULL;
    return pgtab;
}
