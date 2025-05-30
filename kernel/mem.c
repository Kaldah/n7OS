#include <n7OS/mem.h>
#include <string.h>
#include <stdbool.h>
#include <n7OS/printk.h> // Add for printfk function
#include <n7OS/kheap.h>

uint32_t *free_page_bitmap_table;

/**
 * @brief Marque la page allouée
 * 
 * Lorsque la page a été choisie, cette fonction permet de la marquer allouée
 * 
 * @param adresse Adresse de la page à allouer
 */
void setPage(uint32_t adresse) {
    if (free_page_bitmap_table == NULL) {
        printfk("Error: Memory manager not initialized.\n");
        return;
    }
    
    // On récupère l'index global de la page à partir de l'adresse
    uint32_t index_global = adresse / PAGE_SIZE;

    // On divise l'index_global par 32 pour obtenir l'index de la page
    uint32_t page_index = index_global / 32;
    // On récupère l'emplacement du bit de l'adresse
    uint8_t bit_index = index_global % 32;

    // On vérifie que la page n'est pas déjà allouée
    if (free_page_bitmap_table[page_index] & (0x1 << bit_index)) {
        printfk("Page already allocated\n");
        return;
    }

    // On marque la page comme allouée
    free_page_bitmap_table[page_index] |= (1 << bit_index);
}

/**
 * @brief Désalloue la page
 * 
 * Libère la page allouée.
 * 
 * @param adresse Adresse de la page à libérer
 */
void clearPage(uint32_t adresse) {
    if (free_page_bitmap_table == NULL) {
        printfk("Error: Memory manager not initialized.\n");
        return;
    }
    
    uint32_t index_global = adresse / PAGE_SIZE;
    // On divise l'adresse par 32 pour obtenir l'index de la page
    uint32_t page_index = index_global / 32;
    // On récupère l'emplacement du bit de l'adresse
    uint8_t bit_index = index_global % 32;

    // On libère la page : ~ = NOT, pour inverser les bits
    free_page_bitmap_table[page_index] = free_page_bitmap_table[page_index] & (~(1 << bit_index));
}

/**
 * @brief Fourni la première page libre de la mémoire physique tout en l'allouant
 * 
 * @return uint32_t Adresse de la page sélectionnée
 */
uint32_t findfreePage() {
    if (free_page_bitmap_table == NULL) {
        printfk("Error: Memory manager not initialized.\n");
        return -1;
    }
    
    uint32_t adresse = -1;
    bool found = false;

    for (int i = 0; i < BIT_MAP_SIZE && !found; i++) {
        // On vérifie si la case est libre
        if (free_page_bitmap_table[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32 && !found; j++) {
                uint32_t bit = 0x1 << j;
                if (!(free_page_bitmap_table[i] & bit)) {
                    adresse = (i * 32 + j) * PAGE_SIZE;
                    // On marque la page comme allouée
                    // printfk("Page %d allocated at address %x\n", i * 32 + j, adresse);
                    setPage(adresse);
                    found = true;
                }
            }
        }
    }

    return adresse;
}

/**
 * @brief Initialise le gestionnaire de mémoire physique
 * 
 */
void init_mem() {
    printfk("Nombre de pages : %d\n", PAGE_NUMBER);
    printfk("Taille du bitmap : %d\n", BIT_MAP_SIZE);
    // free_page_bitmap_table = (uint32_t *) kmalloc_a(BIT_MAP_SIZE);
    free_page_bitmap_table = (uint32_t *) kmalloc(sizeof(uint32_t) * BIT_MAP_SIZE);
    if (free_page_bitmap_table == NULL) {
        printfk("Error: Failed to allocate memory for free_page_bitmap_table.\n");
        return;
    }
    memset(free_page_bitmap_table, 0, sizeof(uint32_t) * BIT_MAP_SIZE);
}

/**
 * @brief Affiche l'état de la mémoire physique
 * 
 */
void print_mem(uint32_t nb_pages) {

    uint32_t nb_cases_affichee = (nb_pages / 32);
    uint32_t nb_pages_allocated = 0;

    // Check if nb_cases_affichee is within valid range
    if (nb_cases_affichee > BIT_MAP_SIZE) {
        nb_cases_affichee = BIT_MAP_SIZE;
    }

    if (free_page_bitmap_table == NULL) {
        printfk("Error: free_page_bitmap_table has not been initialized.\n");
        return;
    }

    printfk("Affichage des %d premières pages\n", nb_cases_affichee * 32);
    for (uint32_t i = 0; i < nb_cases_affichee; i++) {
        printfk("\n%d -> %d", i * 32, (i + 1) * 32 - 1);
        for (int j = 0; j < 32; j++) {
            if (free_page_bitmap_table[i] & (0x1 << j)) {
                printfk(" 1");
                nb_pages_allocated++;
            } else {
                printfk(" 0");
            }
        }
    }
    printfk("\nNombre de pages allouées : %d\n", nb_pages_allocated);
}