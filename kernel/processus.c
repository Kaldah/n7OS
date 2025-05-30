#include <n7OS/processus.h>
#include <stdio.h>
#include <stdbool.h>
#include <n7OS/printk.h>
#include <unistd.h>
#include <n7OS/cpu.h>

// Define static arrays for all process components
struct process_t process_table[NB_PROC];  // Static array of process structures
uint32_t process_stacks[NB_PROC][STACK_SIZE]; // Static array of process stacks to reserve memory
RESOURCE_ID process_resources[NB_PROC][MAX_RESOURCES]; // Static array for process resources
struct process_t *ready_active_process[NB_PROC]; // File d'attente des processus prets
uint32_t nb_ready_active_process; // Nombre de processus prets

// Cette file devrait etre une liste de pointeurs vers les files d'attente de chaque ressource
// mais pour simplifier, on va la considerer comme un tableau de pointeurs
// vers les processus bloques sur une ressource pour le moment
struct process_t *blocked_process[NB_PROC]; // File d'attente des processus bloques
struct process_t *current_process = NULL; // Pointeur vers le processus en cours d'execution

uint32_t current_process_duration = 0; // Durée d'execution du processus en cours

bool schedule_locked = false; // Variable pour verifier si le planificateur est verrouille

extern void ctx_sw(void *ctx_old, void *ctx_new);


/* Fonctions utilitaires */
pid_t Allouer_Pid() {
    // Recherche d'un PID disponible
    for (pid_t i = 0; i < NB_PROC; i++) {
        // Check if this process slot is unused (state will be 0/ELU by default for unused slots)
        if (i > 0 && process_table[i].pid == 0) { // Skip PID 0 as it's reserved for kernel
            return i;
        }
    }
    return -1; // Aucun PID disponible
}

/* pid of the running process */
pid_t getpid() {
    // Renvoie le PID du processus en cours d'execution
    // Implementation specifique a la gestion des processus
    return current_process->pid; // Recupere le PID du processus en cours
}

/* pid of the parent process */
pid_t getppid(pid_t pid) {
    // Renvoie le PID du processus parent
    // Implementation specifique a la gestion des processus
    if (pid > 0) {
        return process_table[pid].ppid; // Renvoie le PID du parent
    }
    if (pid == 0) {
        return 0; // Renvoie le PID du processus lui-meme car 0 est le PID du noyau
    }
    return -1; // Aucun parent trouve
}

// Initialize resources for a process using our static array
RESOURCES_LIST init_resources(pid_t pid) {
    // Clear all resources for this process
    for (int i = 0; i < MAX_RESOURCES; i++) {
        process_resources[pid][i] = 0;
    }
    return process_resources[pid]; // Return pointer to this process's resource array
}

// Get stack for a process from our static array
void* init_stack(pid_t pid) {
    // Reset the stack for this process
    for (int i = 0; i < STACK_SIZE; i++) {
        process_stacks[pid][i] = 0; // Initialize the stack with zeros
    }
    // Return pointer to this process's stack array
    return (void*)process_stacks[pid];
}


/* Creation d'un processus */
pid_t create(void * program, char * name) {
    pid_t pid;
    if ((pid = Allouer_Pid()) == (pid_t)-1) {
        printfk("Cannot create new process\n");
        shutdown(1); // Shutdown if no PID available
    }
    
    // Initialize the process in the process table directly
    process_table[pid].name = name;
    process_table[pid].priority = nb_ready_active_process;
    process_table[pid].pid = pid;
    process_table[pid].ppid = current_process ? current_process->pid : 0;
    process_table[pid].stack = init_stack(pid);
    process_table[pid].state = PRET_SUSPENDU;
    process_table[pid].resources = init_resources(pid);
    
    // Calculate stack top (end of the array)
    void *stack_top = (void*)((uint32_t)process_table[pid].stack + STACK_SIZE * sizeof(uint32_t) - sizeof(uint32_t));
    // void *stack_top = (void*)((uint32_t)process_table[pid].stack + (STACK_SIZE - 1) * sizeof(uint32_t));
    // printf("Stack top for PID %d: %x\n", pid, (uint32_t)stack_top);
    // printf("Stack bottom for PID %d: %x\n", pid, (uint32_t)process_table[pid].stack);
    // Put the program pointer at the top of the stack

    *(uint32_t*)stack_top = (uint32_t)program;

    // Fix: properly access the stack array as uint32_t array
    // uint32_t *stack_array = (uint32_t *)process_table[pid].stack;
    // stack_array[STACK_SIZE - 1] = (uint32_t)program; // Set the program pointer at the top of the stack
    
    // Initialize context directly in the process structure (no temporary context needed)
    process_table[pid].context.s.ebx = 0;
    process_table[pid].context.s.esp = (uint32_t)stack_top;
    // process_table[pid].context.s.esp = (uint32_t)&stack_array[STACK_SIZE - 1];
    process_table[pid].context.s.ebp = 0;
    process_table[pid].context.s.esi = 0;
    process_table[pid].context.s.edi = 0;
    // Initialiser les flags avec interruptions activées (IF=1, bit 9)
    // 0x200 est le bit IF (Interrupt Flag)
    process_table[pid].context.s.eflags = 0x200;
    
    // Process is already initialized in the process table
    
    return pid;
}

void addProcess(pid_t pid) {
    // Ajoute le processus pid a la file d'attente des processus prets
    // en maintenant le tableau trie par priorite (priorite croissante)
    int i;
    int insert_pos = -1;
    
    // Priorite du processus a ajouter
    uint32_t new_priority = process_table[pid].priority;

    /* 
    J'ai choisis d'utiliser un tri par insertion pour maintenir la liste triee
    par priorite.
    La complexite de cette methode est O(n) dans le meilleur des cas et O(n^2)
    dans le pire des cas.
    Cependant, la liste des processus prets est generalement assez courte (NB_PROC)
    et la complexite est donc acceptable.
    De plus, de cette maniere la liste est toujours triee et on peut facilement
    ajouter un processus a la bonne position.
    On pourrait aussi utiliser un tri par selection ou un tri par fusion, mais
    cela necessiterait plus de memoire et de temps de calcul.

    Une autre solution serait d'utiliser une liste chaînee, mais cela
    necessiterait de gerer la memoire dynamique et de faire des allocations
    et desallocations de memoire, ce qui n'est pas souhaitable dans un noyau.
    
    On pourrait aussi realiser le tri par insertion dans la fonction schedule()
    mais cela necessiterait de trier la liste a chaque fois qu'un processus
    est ajoute ou supprime, ce qui n'est pas optimal car lorsqu'on supprime un
    processus, la liste reste triee.
    */
    
    // 1. Trouver la position d'insertion en fonction de la priorite
    for (i = 0; i < NB_PROC; i++) {
        if (ready_active_process[i] == NULL) {
            // Si on atteint la fin de la liste, on insere ici
            insert_pos = i;
            break;
        } 
        else if (ready_active_process[i]->priority > new_priority) {
            // On a trouve un processus de priorite inferieure
            // (valeur numerique plus grande = priorite plus basse)
            insert_pos = i;
            break;
        }
    }
    
    // 2. Si on n'a pas trouve de position, c'est que la liste est pleine
    if (insert_pos == -1) {
        return; // Liste pleine, impossible d'ajouter
    }
    
    // 3. Decaler tous les processus apres la position d'insertion
    for (i = NB_PROC - 1; i > insert_pos; i--) {
        if (i > 0) {
            ready_active_process[i] = ready_active_process[i-1];
        }
    }
    
    // 4. Inserer le processus a la position trouvee
    ready_active_process[insert_pos] = &process_table[pid];
    nb_ready_active_process++;
}

void removeProcess(pid_t pid) {
    int removed_index = -1;
    
    // Find and remove the process
    for (int i = 0; i < NB_PROC; i++) {
        if (ready_active_process[i] != NULL && ready_active_process[i]->pid == pid) {
            removed_index = i;
            ready_active_process[i] = NULL;
            break;
        }
    }
    
    // Compact the array by shifting everything down
    /* Cette etape est indispensable pour eviter des cas limites 
    et des erreurs avec l'ajout de nouveaux processus par insertion */
    if (removed_index != -1) {
        for (int i = removed_index; i < NB_PROC - 1; i++) {
            ready_active_process[i] = ready_active_process[i + 1];
        }
        ready_active_process[NB_PROC - 1] = NULL;
        
        // Only decrease counter if we actually removed something
        if (nb_ready_active_process > 0) {
            nb_ready_active_process--; // Decrement the count of ready processes
        }
    } else {
        // printfk("Processus %d non trouve dans la file d'attente\n", pid);
    }
}


/* Gestion de l’unite centrale :*/
void schedule() {
    // Implementation du planificateur de processus
    // Choisit le prochain processus a executer
    // Met a jour l'etat des processus et la file d'attente

    schedule_locked = true; // Verrouille le planificateur pour eviter les interruptions
    
    // On verifie si la file d'attente des processus prets est vide
    if (ready_active_process[0] == NULL) {
        // Si la file d'attente est vide, on ne fait rien
        // printfk("File d'attente vide, aucun processus a planifier\n");
        schedule_locked = false; // Deverrouille le planificateur
        return;
    }
    
    // On choisit le processus avec la plus haute priorite (valeur numerique la plus basse)
    struct process_t *next_process = ready_active_process[0];

    if (current_process == NULL) {
        // printfk("Aucun processus en cours d'execution, on demarre le processus %d\n", next_process->pid);
        // Si aucun processus n'est en cours d'execution, on le demarre
        current_process = next_process;
        current_process->state = ELU; // On le passe a l'etat ELU
        current_process->priority = nb_ready_active_process; // On lui donne une priorite de 2 qui sera la plus basse si un processus arrive
        removeProcess(next_process->pid); // On le retire de la file d'attente
        
        // Important: Unlock scheduler before executing the first process
        schedule_locked = false;
        // Reset duration counter
        current_process_duration = 0;
        return;
    }
    
    // In fact, we should used another function if the state is TERMINE to liberate the resources like the regs array
    else if (current_process->state != ELU || current_process->priority >= next_process->priority) {
        // printfk("==SWITCHING==\n");

        // Save old process info before the switch
        pid_t old_pid = current_process->pid;
        
        // Special handling for terminated processes - immediately clean them up
        if (current_process->state == TERMINE) {
            // Make a local copy of the context registers we need for switching
            stack_context_t old_context;
            // Only copy the registers needed for context switch
            old_context = current_process->context;
            
            // Free the process resources completely
            // Clear resources array
            for (int i = 0; i < MAX_RESOURCES; i++) {
                process_resources[old_pid][i] = 0;
            }
            
            // Reset process table entry
            process_table[old_pid].pid = 0;
            
            // Remove the new process from the ready queue
            removeProcess(next_process->pid);
            
            // Update the state of the new process
            next_process->state = ELU;
            next_process->priority = nb_ready_active_process;
            
            // Update current process to the new one
            current_process = next_process;
            
            // Reset duration counter
            current_process_duration = 0;
            // CRITICAL: Unlock scheduler before context switch
            schedule_locked = false;
            
            // Use the copied context for the switch
            ctx_sw(old_context.regs, current_process->context.regs);
        }
        // Normal case: process is not terminated
        else if (current_process->state == ELU) {
            // We stop the current process
            current_process->state = PRET_ACTIF; // Set the state to ready
            
            // Set the priority to the lowest, to avoid monopoly
            // Using explicit if-else to avoid generating CMOV instruction
            if (nb_ready_active_process > 2) {
                current_process->priority = nb_ready_active_process;
            } else {
                current_process->priority = 2;
            }
            
            // Add the current process back to the ready queue
            addProcess(old_pid);
            
            // Remove the new process from the ready queue
            removeProcess(next_process->pid);

            // Update the state of the new process
            next_process->state = ELU;
            next_process->priority = nb_ready_active_process;

            // Update current process to the new one
            current_process = next_process;
            
            // Update priorities of remaining processes in ready queue
            for (int i = 0; i < NB_PROC; i++) {
                if (ready_active_process[i] != NULL) {
                    // Priority 0 is reserved for waking up processes or to start a process directly
                    if (ready_active_process[i]->priority > 1) {
                        ready_active_process[i]->priority--;
                    }
                } else {
                    break; // No more processes to handle
                }
            }
            
            // Reset duration counter
            current_process_duration = 0;
            // CRITICAL: Unlock scheduler before context switch
            schedule_locked = false;
                    
            // This is the point of no return - after this the function will not continue
            ctx_sw((process_table[old_pid].context.regs), (process_table[next_process->pid].context.regs));
        }
        // Other cases (blocked, suspended)
        else {
            // Remove the new process from the ready queue
            removeProcess(next_process->pid);

            // Update the state of the new process
            next_process->state = ELU;
            next_process->priority = nb_ready_active_process;

            // Update current process to the new one
            current_process = next_process;
            
            // Reset duration counter
            current_process_duration = 0;
            // CRITICAL: Unlock scheduler before context switch
            schedule_locked = false;
            
            // This is the point of no return - after this the function will not continue
            ctx_sw((process_table[old_pid].context.regs), (process_table[next_process->pid].context.regs));
        }
        // Code below this point will only execute when this process is switched back to
        // printfk("Resumed execution of process %d\n", getpid());
    } else {
        // Current process has higher priority, keep running it
        // printfk("Current process (PID=%d) has higher priority than next candidate (PID=%d)\n", 
            //    current_process->pid, next_process->pid);
        schedule_locked = false;
    }
}


/* Arret du processus - passage de l'etat elu a pret actif */
void arreter() {
    pid_t pid = getpid();
    if (process_table[pid].state == ELU) {
        // On le passe de l'etat ELU a PRET_ACTIF
        process_table[pid].state = PRET_ACTIF;
        // On ajoute le processus en cours d'execution a la liste des processus actifs
        addProcess(pid);

        // On appelle le processeur pour lui demander de choisir une nouvelle tâche
        schedule();
    }
}

/* Activation du processus - passage de l'etat suspendu a actif */
void activer(pid_t pid) {
    // printfk("Activating process with PID=%d, current state: %d\n", pid, process_table[pid].state);
    if (process_table[pid].state == PRET_SUSPENDU) {
        process_table[pid].state = PRET_ACTIF;
        addProcess(pid);
        // printfk("Process PID=%d activated and added to ready queue\n", pid);
    }
    else if (process_table[pid].state == BLOQUE_SUSPENDU) {
        process_table[pid].state = BLOQUE_ACTIF;
        // Pas besoin d'ajouter a la file des ressources car deja fait lors du blocage
        // printfk("Process PID=%d activated but still blocked on resources\n", pid);
    } else {
        // printfk("Process PID=%d already active, state=%d\n", pid, process_table[pid].state);
    }
}

/* Suspension du processus - passage de l'etat actif a suspendu */
void suspendre(pid_t pid) {
    if (process_table[pid].state == ELU) {
        process_table[pid].state = PRET_SUSPENDU;
        schedule(); // Appeler le schedule pour elire un nouveau processus
    }
    else if (process_table[pid].state == PRET_ACTIF) {
        process_table[pid].state = PRET_SUSPENDU;
        removeProcess(pid);
    }
    else if (process_table[pid].state == BLOQUE_ACTIF) {
        process_table[pid].state = BLOQUE_SUSPENDU;
        // Le processus quitte la file d'attente des ressources
        for (int i = 0; i < MAX_RESOURCES; i++) {
            if (process_table[pid].resources[i] != 0) {
                removeResource(process_table[pid].resources[i], pid);
            }
        }
    }
    else {
        return; // Processus deja suspendu
    }

}


/* Blocage du processus sur une ressource */
void bloquer(RESOURCE_ID rid) {
    // On recupere le pid du processus en cours
    pid_t pid = getpid();
    if (process_table[pid].state == ELU) {
        process_table[pid].state = BLOQUE_ACTIF;
        // On l'ajoute dans la file d'attente de la ressource bloquante
        addResource(rid, pid);

        // On appelle le schedule pour elire un nouveau processus
        schedule();
    }
}

/* Deblocage du processus d'une ressource */
void debloquer(pid_t pid, RESOURCE_ID rid) {
    if (process_table[pid].state == BLOQUE_ACTIF) {
        // On change l'etat du processus
        process_table[pid].state = PRET_ACTIF;
        // On supprime le processus de la file d'attente de la ressource
        removeResource(rid, pid);
        // On ajoute le processus a la file des processus prets
        addProcess(pid);
        // On verifie si le processus debloque est plus prioritaire que le processus en cours
        schedule();
    }
    else if (process_table[pid].state == BLOQUE_SUSPENDU) {
        // On change l'etat du processus
        process_table[pid].state = PRET_SUSPENDU;
        // On supprime le processus de la file d'attente de la ressource
        removeResource(rid, pid);
        // Le processus est suspendu, il n'est pas ajoute a la file des processus prets
    }
}

/* Terminer le processus */
void terminer(pid_t pid) {
    schedule_locked = true; // Verrouille le planificateur pour eviter les interruptions
    
    // Protection against terminating the idle process (PID 0)
    if (pid == 0) {
        // printfk("Cannot terminate the idle process (PID 0)\n");
        schedule_locked = false; // Deverrouille le planificateur
        return;
    }
    
    // On verifie si le processus existe dans la table
    if (process_table[pid].pid != 0) {
        // On verifie si le processus est en cours d'execution
        struct process_t * process_to_terminate = &process_table[pid];
        
        // Clean up resources regardless of the state
        for (int i = 0; i < MAX_RESOURCES; i++) {
            if (process_to_terminate->resources[i] != 0) {
                removeResource(process_to_terminate->resources[i], process_to_terminate->pid);
            }
        }
        
        // Handle based on current state
        if (process_to_terminate->state == PRET_ACTIF) {
            // On le retire de la file d'attente des processus prets
            removeProcess(pid);
        }
        
        // Mark as terminated (actual cleanup now happens during next context switch)
        process_to_terminate->state = TERMINE;
        
        if (process_table[pid].ppid > 0) {
            // On debloque le parent du processus termine
            activer(process_table[pid].ppid);
        }
        
        // If the terminated process is the currently running one, schedule a new one
        if (process_to_terminate->state == ELU || current_process == process_to_terminate) {
            schedule();
        }
    } else {
        // Si aucun pid n'est fourni, on termine le processus en cours
        // printfk("Erreure: pid invalide\n");
        schedule_locked = false; // Deverrouille le planificateur
        return;
    }
}


// Fix for addResource function
void addResource(RESOURCE_ID rid, pid_t pid) {
    // Ajoute le processus pid a la file d'attente de la ressource rid
    // Implementation specifique a la gestion des ressources
    
    // Resources are already allocated in static array, just add the resource
    for (int i = 0; i < MAX_RESOURCES; i++) {
        if (process_resources[pid][i] == 0) { // Trouve un emplacement vide, on considere 0 comme non utilise
            process_resources[pid][i] = rid; // Ajoute la ressource
            break;
        }
    }
}
void removeResource (RESOURCE_ID rid , pid_t pid ) {
    // Supprime le processus pid de la file d'attente de la ressource rid
    // Implementation specifique a la gestion des ressources
    if (process_table[pid].resources != NULL) {
        for (int i = 0; i < MAX_RESOURCES; i++) {
            if (process_table[pid].resources[i] == rid) { // Trouve la ressource a supprimer
                process_table[pid].resources[i] = 0; // Supprime la ressource
                break;
            }
        }
    }
}

void liberer_processus(pid_t pid) {
    // This function is now obsolete as terminated processes are automatically cleaned up
    // by the scheduler during context switching. However, it's kept for backward compatibility.
    printfk("Warning: Fonction liberer_processus() est maintenant obsolète. Les processus terminés\n");
    printfk("sont automatiquement libérés par l'ordonnanceur lors du changement de contexte.\n");
    
    // In case it's still called somewhere, perform the cleanup
    if (process_table[pid].pid != 0) {
        if (process_table[pid].state == TERMINE) {
            if (current_process != &process_table[pid]) {
                // Clear resources array
                for (int i = 0; i < MAX_RESOURCES; i++) {
                    process_resources[pid][i] = 0;
                }
                
                // Reset process table entry
                process_table[pid].pid = 0;
            }
        }
    }
}

void handle_scheduling_IT() {
    // Only attempt scheduling if scheduler isn't locked and there's a current process
    // Use a simplified condition to avoid potential issues

    // Check if it's time to schedule a new process
    // // printfk("Time slot expired for PID=%d, switching...\n", current_process->pid);
    
    // Call scheduler
    schedule();
}


/* Simplified and safer version of display_scheduler_state */
void display_scheduler_state() {
    printfk("=== SCHEDULER STATE ===\n");
    
    // Display minimal information about current process
    if (current_process != NULL) {
        printfk("Current Process: PID=%d, State=%d, Priority=%d \n", 
            current_process->pid, 
            current_process->state,
            current_process->priority);
    } else {
        printfk("Current Process: None\n");
    }
    
    // Just count existing processes without detailed printing
    int existing_count = 0;
    for (int i = 0; i < NB_PROC; i++) {
        if (process_table[i].pid != 0) {
            existing_count++;
        }
    }
    printfk("Total Existing Processes: %d\n", existing_count);

    // Count and show ready processes (safely)
    uint32_t ready_count = 0;
    printfk("\nReady Process Queue:\n");
    for (int i = 0; i < NB_PROC; i++) {
        if (ready_active_process[i] != NULL) {
            ready_count++;
            // Only print PID and priority, avoid printing names which might be corrupted
            printfk("  Position %d: PID=%d, Priority=%d\n", 
                i, ready_active_process[i]->pid, ready_active_process[i]->priority);
        }
    }
    // printfk("Ready Processes Count: %d\n", ready_count);
    
    if (ready_count != nb_ready_active_process) {
        // printfk("Counter mismatch: %d vs %d\n", ready_count, nb_ready_active_process);
        nb_ready_active_process = ready_count; // Auto-correct
    }
    
    printfk("===========================================\n");
}

void wakeup_process(pid_t pid) {
    // Wake up a suspended process
    if (process_table[pid].pid != 0) {
        process_table[pid].state = PRET_ACTIF; // Change state to ready
        process_table[pid].priority = 0; // Set priority to the highest
        addProcess(pid); // Add to the ready queue (Should be first in line)
    } else {
        printfk("Processus %d non trouve\n", pid);
    }
}

void rename_process(pid_t pid, char *name) {
    // Renomme le processus avec le PID donne
    if (process_table[pid].pid != 0) {
        process_table[pid].name = name;
    } else {
        printfk("Processus %d non trouve\n", pid);
    }
}

// Dummy process to create "empty" processes for fork
void dummy_process() {
    // This function serve as placeholder when no process is available
    while (1) {
        hlt();
    }
}

pid_t resume_child_process() {
    // Reprise normale après un fork chez le processus fils
    return 0; // Return 0 to know it's the child process
}