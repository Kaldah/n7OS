#include <inttypes.h>
#include <n7OS/irq.h>
#include <n7OS/printk.h>
#include <unistd.h>

// initialise la ligne num_line avec le traitant handler
void init_irq_entry(int irq_num, uint32_t addr_handler) {
    printfk("===Initialisation de l'IT %d===\n", irq_num);
    // on initialise la ligne d'interruption
    idt_entry_t *idt_entry = (idt_entry_t *) &idt[irq_num];
    idt_entry->offset_inf = (uint16_t) (addr_handler & 0xFFFF);
    idt_entry->sel_segment = KERNEL_CS;
    idt_entry->zero = 0;
    idt_entry->type_attr = 0b10000000 | 14; // present, DPL=0, type=14 (interrupt gate)
    idt_entry->offset_sup = (uint16_t) ((addr_handler >> 16) & 0xFFFF);
}
