#include <n7OS/mem.h>

uint32_t *free_page_bitmap_table;

/**
 * @brief Marque la page allouée
 * 
 * Lorsque la page a été choisie, cette fonction permet de la marquer allouée
 * 
 * @param addr Adresse de la page à allouer
 */
void setPage(uint32_t addr) {

    // On divise l'adresse par 32 pour obtenir l'index de la page
    uint32_t page_index = addr / 32;
    // On récupère l'emplacement du bit de l'adresse
    uint8_t bit_index = addr % 32;

    // On vérifie que la page n'est pas déjà allouée
    if (free_page_bitmap_table[page_index] & (1 << bit_index)) {
        printf("Page already allocated\n");
        return;
    }

    // On marque la page comme allouée
    free_page_bitmap_table[page_index] = free_page_bitmap_table[page_index] | (1 << bit_index);
}

/**
 * @brief Désalloue la page
 * 
 * Libère la page allouée.
 * 
 * @param addr Adresse de la page à libérer
 */
void clearPage(uint32_t addr) {
     // On divise l'adresse par 32 pour obtenir l'index de la page
    uint32_t page_index = addr / 32;
    // On récupère l'emplacement du bit de l'adresse
    uint8_t bit_index = addr % 32;

    // On libère la page : ~ = NOT, pour inverser les bits
    free_page_bitmap_table[page_index] = free_page_bitmap_table[page_index] & (~(1 << bit_index));
}

/**
 * @brief Fourni la première page libre de la mémoire physique tout en l'allouant
 * 
 * @return uint32_t Adresse de la page sélectionnée
 */
uint32_t findfreePage() {
    uint32_t adresse = 0x0;

    for (int i = 0; i < PAGES_TABLE_SIZE; i++) {
        if (free_page_bitmap_table[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32  ; j++) {
                if ((free_page_bitmap_table[i] & (1 << j)) == 0) {
                    adresse = i * 32 + j;
                    // On marque la page comme allouée
                    setPage(adresse);
                    break;
                }
            }
            break;
        }
    }

    return adresse;
}

/**
 * @brief Initialise le gestionnaire de mémoire physique
 * 
 */
void init_mem() {
    free_page_bitmap_table[PAGES_TABLE_SIZE];

    for (int i = 0; i < PAGES_TABLE_SIZE; i++) {
        free_page_bitmap_table[i] = 0x0;
    }
}

/**
 * @brief Affiche l'état de la mémoire physique
 * 
 */
void print_mem(uint32_t nb_pages) {
    for (int i = 0; i < nb_pages; i++) {
        printf("Page %d : ", i);
        // Affiche 1 si la page est allouée, 0 sinon
        printf("%x\n", free_page_bitmap_table[i]);
        printf("\n");
    }
    
}