#include <n7OS/paging.h>
#include <stddef.h> // nécessaire pour NULL

void initialise_paging() {
    // On initialise la table de pages : CR3
    uint32_t page_directory_adresse_CR3 = kmalloc_int(sizeof(PDE), 1, 0);
    PageDirectory page_directory = (PDE *) page_directory_adresse_CR3;

    printf("Page Directory adresse (CR3): %x\n", page_directory_adresse_CR3);
    
    // On initialise les tables de pages
    for (int i = 0; i < 1024; i++) {
        // On alloue une table de page
        uint32_t page_table_adresse = kmalloc_a(1024 * sizeof(PTE));

        // On initialise la table de page
        PageTable *page_table = (PTE *) page_table_adresse;

        // On initialise la table de page avec des pages libres
        for (int j = 0; j < 1024; j++) {
            // On initialise l'entrée de la table de page
            PTE page_table_entry = {0};

            // On obtient l'adresse dont les 12 bits de poids faibles sont à 0
            int32_t page_adress = findfreePage();
            page_table[j] = page_table_entry ;
        }

        // On initialise l'entrée du répertoire de page
        page_directory[i] = (PDE) page_table_adresse;
    }
}

PageTable alloc_page_entry(uint32_t address, int is_writeable, int is_kernel) {
    PageTable pgtab = NULL;
    return pgtab;
}
