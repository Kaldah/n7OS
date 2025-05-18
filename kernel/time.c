#include <n7OS/time.h>
#include <n7OS/irq.h>
#include <n7OS/cpu.h>
#include <n7OS/kheap.h>
#include <stdio.h>
#include <string.h>
#include <n7OS/console.h>
#include <n7OS/processus.h>
#include <n7OS/sys.h>
#include <unistd.h>
#include <stdbool.h>
#include <n7OS/printk.h> // Add for printfk function

extern void handler_IT_TIMER();
extern struct process_t *current_process; // Processus en cours d'exécution
extern bool schedule_locked; // Variable pour vérifier si le planificateur est verrouillé
uint32_t current_process_duration = 0;


uint32_t time = 0;


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
    // Send acknowledgment to the PIC first
    outb(0x20, 0x20);
    
    // Increment time counter
    time++;

    // Display current time every second
    if (time % 1000 == 0) {
        console_puts_time(get_time_string());
    }
    
    // Only attempt scheduling if scheduler isn't locked and there's a current process
    // Use a simplified condition to avoid potential issues
    if (!schedule_locked && current_process != NULL) {
        current_process_duration++;
        
        // Check if it's time to schedule a new process
        if (current_process_duration >= TIME_SLOT) {
            // Reset duration counter
            current_process_duration = 0;
            
            // Call scheduler
            // schedule();
        }
    }
}

uint32_t get_time() {
    return time;
}

char *time_buffer = "00:00:00"; // Static buffer to prevent reallocations

char * get_time_string() {
    uint32_t seconds = time / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    seconds %= 60;
    minutes %= 60;
    hours %= 24;

    // Use the static buffer instead of a string literal
    snprintf(time_buffer, 9, "%02u:%02u:%02u", hours, minutes, seconds);
    return time_buffer;
}