#include <n7OS/irq.h>
#include <n7OS/cpu.h>
#include <stdio.h>


extern void handler_IT();
extern void handler_IT50();


void init_irq() {
    init_irq_entry(50, (uint32_t) handler_IT50);
}

void handler_en_C() {

}

int handler_en_C_50() {
    // on ne fait que retourner 1
    printf("sys_50 traitee\n");
    return 1;
}