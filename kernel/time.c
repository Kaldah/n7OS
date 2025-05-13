#include <n7OS/time.h>
#include <n7OS/irq.h>
#include  <n7OS/cpu.h>
#include <n7OS/kheap.h>
#include <stdio.h>
#include <string.h>
#include <n7OS/console.h>

extern void handler_IT_TIMER();

uint64_t time = 0;


// initialise la ligne num_line avec le traitant handler
void init_time() {
    printfk("===Initialisation de l'horloge===\n");
    init_irq_entry(0x20, (uint32_t) handler_IT_TIMER);

    outb(0x34, 0x43);
    outb(FREQUENCE&0xFF, 0x40);
    outb((FREQUENCE>>8)&0xFF, 0x40);

    outb(inb(0x21)&0xfe, 0x21); // On active l'IT du Timer
}

void handler_en_C_TIMER() {
    // On incrémente toutes les millisecondes
    outb(0x20, 0x20); // On envoie un ack au PIC
    time++;

    // On affiche le temps courant
    if ((uint32_t)time % 1000 == 0) {
        // On affiche l'heure courante dans la console
        console_puts_time(get_time_string());
    }
}

uint64_t get_time() {
    return time;
}

char * get_time_string() {
    uint32_t seconds = (uint32_t) time / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    seconds %= 60;
    minutes %= 60;
    hours %= 24;

    char *time_str = "00:00:00";
    snprintf(time_str, 9, "%02u:%02u:%02u", hours, minutes, seconds);
    return time_str;
}