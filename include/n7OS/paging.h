/**
 * @file paging.h
 * @brief Gestion de la pagination dans le noyau
 */
#ifndef _PAGING_H
#define _PAGING_H

#include <inttypes.h>

#define PRESENT 0x1
#define WRITE 0x2
#define USER 0x4
#define RESERVED1 0x8
#define ACCESSED 0x20
#define DIRTY 0x40
#define RESERVED2 0x80
#define AVAILABLE 0x200
#define PAGE 0x1000
#define TABLE_NUMBER 1024

/**
 * @brief Description d'une ligne de la table de page
 * 
 */
typedef struct {

    uint8_t P:1; // Present
    uint8_t RW:1; // Read/Write
    uint8_t U:1; // User or Kernel mode
    uint8_t RSVD1:2; // Reserved
    uint8_t A:1; // Accessed
    uint8_t D:1; // Dirty
    uint8_t RSVD2:2; // Reserved
    uint8_t available:3;
    uint32_t page:20;

} page_table_entry_t;


/**
 * @brief Une entrée dans la table de page peut être manipulée en utilisant
 *        la structure page_table_entry_t ou directement la valeur
 */
typedef union {
    page_table_entry_t page_entry;
    uint32_t value;
} PTE; // PTE = Page Table Entry 

/**
 * @brief Une table de page (PageTable) est un tableau d'entrée de table de page
 * 
 */
typedef PTE * PageTable;

/**
 * @brief Description d'une entrée dans le répertoire de page
 * 
 */
typedef struct {
    uint8_t P:1; // Present
    uint8_t RW:1; // Read/Write
    uint8_t U:1; // User/Supervisor
    uint16_t RSVD:9; // Reserved
    uint32_t page_table:20;
} page_directory_entry_t;

/**
 * @brief Une entrée dans le répertoire de page peut être manipulée en utilisant
 *        la structure page_directory_entry_t ou directement la valeur
 */
typedef union {
    page_directory_entry_t page_entry;
    uint32_t value;
} PDE; // PDE = Page Directory Entry 


/**
 * @brief Un répertoire de page (PageDirectory) est un tableau de descripteurs de table de page
 * 
 */
typedef PDE * PageDirectory;

/**
 * @brief Cette fonction initialise le répertoire de page, alloue les pages de table du noyau
 *        et active la pagination
 * 
 */
PageDirectory initialise_paging();

/**
 * @brief Cette fonction alloue une page de la mémoire physique à une adresse de la mémoire virtuelle
 * 
 * @param address       Adresse de la mémoire virtuelle à mapper
 * @param is_writeable  Si is_writeable == 1, la page est accessible en écriture
 * @param is_kernel     Si is_kernel == 1, la page ne peut être accédée que par le noyau
 * @return PageTable    La table de page modifiée
 */
PageTable alloc_page_entry(uint32_t address, int is_writeable, int is_kernel);
#endif
