/**
 * @file kheap.h
 * @brief Interface d'allocation de mémoire dans la zone du tas du noyau 
 */
#ifndef _KHEAP_H
#define _KHEAP_H

#include <inttypes.h>

/**
 * @brief Alloue une zone mémoire de taille sz. C'est une version simplifiée de kmalloc().
 * 
 * @param sz        Taille de la zone mémoire à allouer
 * @param align     align == 1 : la zone doit être alignée sur une page.
 * @param phys      phys != 0 : le résultat de l'allocation est stocké dans phys
 * @return uint32_t Adresse de début de la zone allouée
 */
uint32_t kmalloc_int(uint32_t sz, int align, uint32_t *phys);

/**
 * @brief Alloue une zone mémoire de taille sz, alignée sur une page.
 * 
 * @param sz        Taille de la zone mémoire à allouer
 * @return uint32_t Adresse de début de la zone allouée
 */
uint32_t kmalloc_a(uint32_t sz);

/**
 * @brief Alloue une zone mémoire de taille sz, le résultat sera fourni dans phys.
 * 
 * @param sz        Taille de la zone mémoire à allouer
 * @return uint32_t Adresse de début de la zone allouée
 */
uint32_t kmalloc_p(uint32_t sz, uint32_t *phys);

/**
 * @brief Alloue une zone mémoire de taille sz, alignée sur une page; le résultat sera fourni dans phys.
 * 
 * @param sz        Taille de la zone mémoire à allouer
 * @param phys      Adresse de début de la zone allouée. Doit être un pointeur valide de type uint32_t
 * @return uint32_t Adresse de début de la zone allouée
 */
uint32_t kmalloc_ap(uint32_t sz, uint32_t *phys);

/**
 * @brief Fonction d'allocation d'une zone mémoire générique (pas d'alignement mémoire,
 *        le résultat est l'adresse de la zone)
 * 
 * @param sz        Taille de la zone mémoire à allouer
 * @return uint32_t Adresse de début de la zone allouée
 */
uint32_t kmalloc(uint32_t sz);

void kmalloc_init();   

#endif