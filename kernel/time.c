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
#include <n7OS/printk.h> 

extern void handler_IT_TIMER();

uint32_t time = 0;
uint32_t suspended_process_wakeup_time[NB_PROC]; // Wake up time for suspended processes

// initialise la ligne num_line avec le traitant handler
void init_time() {
    for (int i = 0; i < NB_PROC; i++) {
        suspended_process_wakeup_time[i] = 0;
    }
    printfk("===Initialisation de l'horloge===\n");
    init_irq_entry(0x20, (uint32_t) handler_IT_TIMER);

    outb(0x34, 0x43);
    outb(FREQUENCE&0xFF, 0x40);
    outb((FREQUENCE>>8)&0xFF, 0x40);

    outb(inb(0x21)&0xfe, 0x21); // On active l'IT du Timer
}

void handler_en_C_TIMER() {
    extern uint32_t current_process_duration;
    extern struct process_t *current_process;
    extern bool schedule_locked;
    // Send acknowledgment to the PIC first
    outb(0x20, 0x20);
    
    // Increment time counter
    time++;
    
    // Display current time every second
    if (time % 1000 == 0) {
        console_puts_time(get_time_string());
    }

    if (!schedule_locked && current_process != NULL) current_process_duration++;
    if (current_process_duration >= TIME_SLOT) {
        // Trigger process scheduling when time slot has expired
        handle_scheduling_IT();
    }
    // Check if any suspended processes need to be activated
    for (int i = 0; i < NB_PROC; i++) {
        if (suspended_process_wakeup_time[i] != 0 && suspended_process_wakeup_time[i] <= get_time()) {
            suspended_process_wakeup_time[i] = 0; // Reset the wake-up time
            // Wake up the suspended process
            wakeup_process((pid_t) i);
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

// Function to suspend a process for a specified duration
void suspend_process_timer(pid_t pid, uint32_t duration) {
    if (pid < 0 || pid >= NB_PROC) {
        return; // Invalid PID
    }
    
    // Set the wake-up time for the suspended process
    suspended_process_wakeup_time[pid] = get_time() + duration;

    // Suspend the process
    suspendre(pid);
}