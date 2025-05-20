#include <n7OS/processus.h>
#include <stdio.h>
#include <stdbool.h>
#include <n7OS/printk.h> // Add for // printfk function
#include <unistd.h>

// Define static arrays for all process components
struct process_t process_entities[NB_PROC];  // Static array of process structures
uint32_t process_stacks[NB_PROC][STACK_SIZE]; // Static array of process stacks
RESOURCE_ID process_resources[NB_PROC][MAX_RESOURCES]; // Static array for process resources
bool process_used[NB_PROC] = {false}; // Track which process slots are used

extern void ctx_sw(void *ctx_old, void *ctx_new);
struct process_t *process_table[NB_PROC]; // Table des processus (pointers to process_entities)
struct process_t *ready_active_process[NB_PROC]; // File d'attente des processus prets
uint32_t nb_ready_active_process; // Nombre de processus prets

// Cette file devrait etre une liste de pointeurs vers les files d'attente de chaque ressource
// mais pour simplifier, on va la considerer comme un tableau de pointeurs
// vers les processus bloques sur une ressource pour le moment
struct process_t *blocked_process[NB_PROC]; // File d'attente des processus bloques
struct process_t *current_process = NULL; // Pointeur vers le processus en cours d'execution

uint32_t current_process_duration = 0; // Durée d'execution du processus en cours

bool schedule_locked = false; // Variable pour verifier si le planificateur est verrouille


/* Fonctions utilitaires */
static pid_t Allouer_Pid() {
    // Recherche d'un PID disponible
    for (pid_t i = 0; i < NB_PROC; i++) {
        if (process_table[i] == NULL) { // Considere 0 comme PID invalide/non utilise car c'est celui de l'ID du noyau
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
        return process_table[pid]->ppid; // Renvoie le PID du parent
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
    return (void*)process_stacks[pid]; // Return pointer to this process's stack
}


/* Creation d'un processus */
pid_t create(void * program, char * name) {
    pid_t pid;
    
    if ((pid = Allouer_Pid()) == (pid_t)-1) {
        // printfk("Cannot create new process\n");
        shutdown(1); // Shutdown if no PID available
    }
    
    // Mark this process slot as used
    process_used[pid] = true;
    
    // Use the pre-allocated process structure
    struct process_t *new_process = &process_entities[pid];
    
    // Initialize the process
    new_process->name = name;
    new_process->priority = nb_ready_active_process;
    new_process->pid = pid;
    new_process->ppid = current_process ? current_process->pid : 0;
    new_process->stack = init_stack(pid);
    new_process->state = PRET_SUSPENDU;
    new_process->resources = init_resources(pid);
    
    // Calculate stack top (end of the array)
    void *stack_top = (void*)((uint32_t)new_process->stack + STACK_SIZE * sizeof(uint32_t) - sizeof(uint32_t));
    
    // Put the program pointer at the top of the stack
    *(uint32_t*)stack_top = (uint32_t)program;
    
    // Initialize context directly in the process structure (no temporary context needed)
    new_process->context.s.ebx = 0;
    new_process->context.s.esp = (uint32_t)stack_top;
    // new_process->context.s.ebp = (uint32_t)new_process->stack;
    new_process->context.s.ebp = 0;
    new_process->context.s.esi = 0;
    new_process->context.s.edi = 0;
    
    // Set the process in the process table
    process_table[pid] = new_process;
    
    return pid;
}

void addProcess(pid_t pid) {
    // Ajoute le processus pid a la file d'attente des processus prets
    // en maintenant le tableau trie par priorite (priorite croissante)
    int i;
    int insert_pos = -1;
    
    // Priorite du processus a ajouter
    uint32_t new_priority = process_table[pid]->priority;

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
    ready_active_process[insert_pos] = process_table[pid];
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
        
        // Skip adding terminated processes back to the ready queue
        if (current_process->state == ELU) {
            // We stop the current process
            current_process->state = PRET_ACTIF; // Set the state to ready
            current_process->priority = nb_ready_active_process; // Set the priority to the lowest
            // Add the current process back to the ready queue
            addProcess(old_pid);
        }

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
        
        // printfk("Switching from PID=%d to PID=%d\n", old_pid, next_process->pid);
        
        // Reset duration counter
        current_process_duration = 0;
        // CRITICAL: Unlock scheduler before context switch
        schedule_locked = false;
                
        // This is the point of no return - after this the function will not continue
        ctx_sw((process_table[old_pid]->context.regs), (process_table[next_process->pid]->context.regs));
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
    if (process_table[pid]->state == ELU) {
        // On le passe de l'etat ELU a PRET_ACTIF
        process_table[pid]->state = PRET_ACTIF;
        // On ajoute le processus en cours d'execution a la liste des processus actifs
        addProcess(pid);

        // On appelle le processeur pour lui demander de choisir une nouvelle tâche
        schedule();
    }
}

/* Activation du processus - passage de l'etat suspendu a actif */
void activer(pid_t pid) {
    // printfk("Activating process with PID=%d, current state: %d\n", pid, process_table[pid]->state);
    if (process_table[pid]->state == PRET_SUSPENDU) {
        process_table[pid]->state = PRET_ACTIF;
        addProcess(pid);
        // printfk("Process PID=%d activated and added to ready queue\n", pid);
    }
    else if (process_table[pid]->state == BLOQUE_SUSPENDU) {
        process_table[pid]->state = BLOQUE_ACTIF;
        // Pas besoin d'ajouter a la file des ressources car deja fait lors du blocage
        // printfk("Process PID=%d activated but still blocked on resources\n", pid);
    } else {
        // printfk("Process PID=%d already active, state=%d\n", pid, process_table[pid]->state);
    }
}

/* Suspension du processus - passage de l'etat actif a suspendu */
void suspendre(pid_t pid) {
    if (process_table[pid]->state == ELU) {
        process_table[pid]->state = PRET_SUSPENDU;
        schedule(); // Appeler le schedule pour elire un nouveau processus
    }
    else if (process_table[pid]->state == PRET_ACTIF) {
        process_table[pid]->state = PRET_SUSPENDU;
        removeProcess(pid);
    }
    else if (process_table[pid]->state == BLOQUE_ACTIF) {
        process_table[pid]->state = BLOQUE_SUSPENDU;
        // Le processus quitte la file d'attente des ressources
        for (int i = 0; i < MAX_RESOURCES; i++) {
            if (process_table[pid]->resources[i] != 0) {
                removeResource(process_table[pid]->resources[i], pid);
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
    if (process_table[pid]->state == ELU) {
        process_table[pid]->state = BLOQUE_ACTIF;
        // On l'ajoute dans la file d'attente de la ressource bloquante
        addResource(rid, pid);

        // On appelle le schedule pour elire un nouveau processus
        schedule();
    }
}

/* Deblocage du processus d'une ressource */
void debloquer(pid_t pid, RESOURCE_ID rid) {
    if (process_table[pid]->state == BLOQUE_ACTIF) {
        // On change l'etat du processus
        process_table[pid]->state = PRET_ACTIF;
        // On supprime le processus de la file d'attente de la ressource
        removeResource(rid, pid);
        // On ajoute le processus a la file des processus prets
        addProcess(pid);
        // On verifie si le processus debloque est plus prioritaire que le processus en cours
        schedule();
    }
    else if (process_table[pid]->state == BLOQUE_SUSPENDU) {
        // On change l'etat du processus
        process_table[pid]->state = PRET_SUSPENDU;
        // On supprime le processus de la file d'attente de la ressource
        removeResource(rid, pid);
        // Le processus est suspendu, il n'est pas ajoute a la file des processus prets
    }
}

/* Terminer le processus */
void terminer(pid_t pid) {
    schedule_locked = true; // Verrouille le planificateur pour eviter les interruptions
    
    // On verifie si le processus existe dans la table
    if (process_table[pid] != NULL) {
        // On verifie si le processus est en cours d'execution
        struct process_t * process_to_terminate = process_table[pid];
        if (process_to_terminate->state == ELU) {
            // On le passe a l'etat TERMINE
            process_to_terminate->state = TERMINE;

             // On debloque toutes les ressources utilisees par le processus
            for (int i = 0; i < MAX_RESOURCES; i++) {
                if (process_to_terminate->resources[i] != 0) {
                    removeResource(process_to_terminate->resources[i], process_to_terminate->pid);
                }
            }
            // printfk("Processus %d termine\n", pid);
            // On appelle le schedule pour elire un nouveau processus
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
    if (process_table[pid]->resources != NULL) {
        for (int i = 0; i < MAX_RESOURCES; i++) {
            if (process_table[pid]->resources[i] == rid) { // Trouve la ressource a supprimer
                process_table[pid]->resources[i] = 0; // Supprime la ressource
                break;
            }
        }
    }
}

void liberer_processus(pid_t pid) {
    // No need to free memory with static arrays, just mark the process as unused
    if (process_table[pid] != NULL) {
        // Make sure the process is actually terminated
        if (process_table[pid]->state == TERMINE) {
            // Make sure we're not trying to free the current process
            if (current_process != process_table[pid]) {
                // Mark process slot as unused
                process_used[pid] = false;
                
                // Clear resources array (not strictly necessary but good practice)
                for (int i = 0; i < MAX_RESOURCES; i++) {
                    process_resources[pid][i] = 0;
                }
                
                // Remove from process table
                process_table[pid] = NULL;
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
        printfk("Current Process: PID=%d, State=%d\n", 
               current_process->pid, 
               current_process->state);
    } else {
        printfk("Current Process: None\n");
    }
    
    // Just count active processes without detailed printing
    int active_count = 0;
    for (int i = 0; i < NB_PROC; i++) {
        if (process_table[i] != NULL) {
            active_count++;
        }
    }
    printfk("Total Active Processes: %d\n", active_count);
    
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
    if (process_table[pid] != NULL) {
        process_table[pid]->state = PRET_ACTIF; // Change state to ready
        process_table[pid]->priority = 0; // Set priority to the highest
        addProcess(pid); // Add to the ready queue (Should be first in line)
    } else {
        printfk("Processus %d non trouve\n", pid);
    }
}

void rename_process(pid_t pid, char *name) {
    // Renomme le processus avec le PID donne
    if (process_table[pid] != NULL) {
        process_table[pid]->name = name;
    } else {
        printfk("Processus %d non trouve\n", pid);
    }
}

// Dummy process to create "empty" processes for fork
void dummy_process() {
    while (1) {
        hlt();
    }
}