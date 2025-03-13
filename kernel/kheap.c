#include <n7OS/kheap.h>

/**
 * @brief Lorsque tout le code du noyau est compilé, le tas du noyau commence juste après.
 * 
 */
extern uint32_t mem_heap;
uint32_t placement_address = (uint32_t)&mem_heap;

uint32_t kmalloc_int(uint32_t sz, int align, uint32_t *phys) {
    uint32_t address;
    if (align == 1 && (placement_address & 0xFFF) ) {
        // Aligne l'adresse si on n'est pas au début d'une page
        placement_address &= 0xFFFFF000;
        placement_address += 0x1000;
    }
    if (phys) {
        *phys = placement_address;
    }
    address= placement_address;
    placement_address += sz;
    return address;
}

uint32_t kmalloc_a(uint32_t sz) {
    return kmalloc_int(sz, 1, 0);
}

uint32_t kmalloc_p(uint32_t sz, uint32_t *phys) {
    return kmalloc_int(sz, 0, phys);
}

uint32_t kmalloc_ap(uint32_t sz, uint32_t *phys) {
    return kmalloc_int(sz, 1, phys);
}

uint32_t kmalloc(uint32_t sz) {
    return kmalloc_int(sz, 0, 0);
}

void kmalloc_init() {
    placement_address = (uint32_t)&mem_heap;
}   