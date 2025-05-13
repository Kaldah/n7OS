#include <n7OS/processus.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <n7OS/sys.h>


struct process_t process_table[NB_PROC]; // Table des processus
struct process_t *ready_active_process[NB_PROC]; // File d’attente des processus prêts

// Cette file devrait être une liste de pointeurs vers les files d'attente de chaque ressource
// mais pour simplifier, on va la considérer comme un tableau de pointeurs
// vers les processus bloqués sur une ressource pour le moment
struct process_t *blocked_process[NB_PROC]; // File d’attente des processus bloqués
struct process_t *current_process; // Pointeur vers le processus en cours d'exécution

/* Fonctions utilitaires */
static pid_t Allouer_Pid() {
    // Recherche d'un PID disponible
    for (pid_t i = 0; i < NB_PROC; i++) {
        if (process_table[i].pid == 0) { // Considère 0 comme PID invalide/non utilisé car c'est celui de l'ID du noyau
            return i;
        }
    }
    return -1; // Aucun PID disponible
}

// Fix for init_stack function - allocate memory dynamically instead of returning local variable
void* init_stack() {
    void* stack = malloc(STACK_SIZE); // Allocate memory for the stack
    if (stack == NULL) {
        return NULL; // Return NULL if allocation failed
    }
    return stack; // Return pointer to the allocated stack
}

// Fix for init_resources function
RESOURCES_LIST init_resources() {
    return NULL; // Return NULL for an empty list
}

/* pid of the running process */
pid_t getpid() {
    // Renvoie le PID du processus en cours d'exécution
    // Implémentation spécifique à la gestion des processus
    return current_process->pid; // Récupère le PID du processus en cours
}

/* pid of the parent process */
pid_t getppid(pid_t pid) {
    // Renvoie le PID du processus parent
    // Implémentation spécifique à la gestion des processus
    if (pid > 0) {
        return process_table[pid].ppid; // Renvoie le PID du parent
    }
    if (pid == 0) {
        return 0; // Renvoie le PID du processus lui-même car 0 est le PID du noyau
    }

    return -1; // Aucun parent trouvé
}

void addProcess(pid_t pid) {
    // Ajoute le processus pid à la file d'attente des processus prêts
    // en maintenant le tableau trié par priorité (priorité croissante)
    int i;
    int insert_pos = -1;
    
    // Priorité du processus à ajouter
    uint32_t new_priority = process_table[pid].priority;

    /* 
    J'ai choisit d'utiliser un tri par insertion pour maintenir la liste triée
    par priorité.
    La complexité de cette méthode est O(n) dans le meilleur des cas et O(n^2)
    dans le pire des cas.
    Cependant, la liste des processus prêts est généralement assez courte (NB_PROC)
    et la complexité est donc acceptable.
    De plus, de cette manière la liste est toujours triée et on peut facilement
    ajouter un processus à la bonne position.
    On pourrait aussi utiliser un tri par sélection ou un tri par fusion, mais
    cela nécessiterait plus de mémoire et de temps de calcul.

    Une autre solution serait d'utiliser une liste chaînée, mais cela
    nécessiterait de gérer la mémoire dynamique et de faire des allocations
    et désallocations de mémoire, ce qui n'est pas souhaitable dans un noyau.
    
    On pourrait aussi réaliser le tri par insertion dans la fonction scheduler()
    mais cela nécessiterait de trier la liste à chaque fois qu'un processus
    est ajouté ou supprimé, ce qui n'est pas optimal car lorsqu'on supprime un
    processus, la liste reste triée.
    */
    
    // 1. Trouver la position d'insertion en fonction de la priorité
    for (i = 0; i < NB_PROC; i++) {
        if (ready_active_process[i] == NULL) {
            // Si on atteint la fin de la liste, on insère ici
            insert_pos = i;
            break;
        } 
        else if (ready_active_process[i]->priority > new_priority) {
            // On a trouvé un processus de priorité inférieure
            // (valeur numérique plus grande = priorité plus basse)
            insert_pos = i;
            break;
        }
    }
    
    // 2. Si on n'a pas trouvé de position, c'est que la liste est pleine
    if (insert_pos == -1) {
        return; // Liste pleine, impossible d'ajouter
    }
    
    // 3. Décaler tous les processus après la position d'insertion
    for (i = NB_PROC - 1; i > insert_pos; i--) {
        if (i > 0) {
            ready_active_process[i] = ready_active_process[i-1];
        }
    }
    
    // 4. Insérer le processus à la position trouvée
    ready_active_process[insert_pos] = &process_table[pid];
}

void removeProcess(pid_t pid) {
    int removed_index = -1;
    
    // Find and mark the process
    for (int i = 0; i < NB_PROC; i++) {
        if (ready_active_process[i] != NULL && ready_active_process[i]->pid == pid) {
            removed_index = i;
            ready_active_process[i] = NULL;
            break;
        }
    }
    
    // Compact the array by shifting everything down
    /* Cette étape est indispensable pour éviter des cas limites 
    et des erreurs avec l'ajout de nouveaux processus par insertion */
    if (removed_index != -1) {
        for (int i = removed_index; i < NB_PROC - 1; i++) {
            ready_active_process[i] = ready_active_process[i + 1];
        }
        ready_active_process[NB_PROC - 1] = NULL;
    }
}


// Fix for addResource function
void addResource (RESOURCE_ID rid , pid_t pid ) {
    // Ajoute le processus pid à la file d'attente de la ressource rid
    // Implémentation spécifique à la gestion des ressources

    // On vérifie s'il y a déjà des ressources allouées
    if (process_table[pid].resources == NULL) {
        // Allocate memory for resources
        RESOURCES_LIST resources = malloc(sizeof(RESOURCE_ID) * MAX_RESOURCES);
        resources[0] = rid;
        
        // Proper assignment without type mismatch
        process_table[pid].resources = resources;
    }

    // On ajoute la ressource à la liste des ressources
    for (int i = 0; i < MAX_RESOURCES; i++) {
        if (process_table[pid].resources[i] == 0) { // Trouve un emplacement vide, on considère 0 comme non utilisé
            process_table[pid].resources[i] = rid; // Ajoute la ressource
            break;
        }
    }
    
}
void removeResource (RESOURCE_ID rid , pid_t pid ) {
    // Supprime le processus pid de la file d'attente de la ressource rid
    // Implémentation spécifique à la gestion des ressources
    if (process_table[pid].resources != NULL) {
        for (int i = 0; i < MAX_RESOURCES; i++) {
            if (process_table[pid].resources[i] == rid) { // Trouve la ressource à supprimer
                process_table[pid].resources[i] = 0; // Supprime la ressource
                break;
            }
        }
    }
}

/* Gestion de l’unité centrale :*/
void scheduler() {
    // Implémentation du planificateur de processus
    // Choisit le prochain processus à exécuter
    // Met à jour l'état des processus et la file d'attente

    /*
    Une fonction SCHEDULER assure la gestion de l’unité centrale : elle doit s’assurer que
    la priorité du processus en cours d’exécution sur l’unité centrale, dans l’état ELU, est
    supérieure ou égale à celle de tout processus se trouvant dans l’état PRET_ACTIF.
    Elle est appelée à chaque fois qu’un processus quitte l’état ELU ou passe dans l’état
    PRET_ACTIF. Prototype : void scheduler();
    */

    // On vérifie si la file d'attente des processus prêts est vide
    if (ready_active_process[0] == NULL) {
        // Si la file est vide, on ne fait rien
        return;
    }
    // On choisit le processus avec la plus haute priorité (valeur numérique la plus basse)
    struct process_t *next_process = ready_active_process[0];

    // On vérifie si le processus en cours d'exécution a une priorité inférieure
    if (current_process != NULL && current_process->priority > next_process->priority) {
        // Si le processus en cours a une priorité inférieure, on l'arrête
        // Il faudrait aussi vérifier s'il utilise des ressources, les libérer
        // et le mettre dans l'état PRET_ACTIF ou BLOQUE_ACTIF si d'autres processus
        // veulent utiliser ces ressources
        arreter(current_process->pid); // Le processus revient dans l'état PRET_ACTIF
        next_process->state = ELU; // On le passe à l'état ELU
        next_process->priority = 10; // On lui donne une priorité de 7 qui sera la plus basse
        current_process = next_process; // On met à jour le processus en cours  
        removeProcess(next_process->pid); // On le retire de la file d'attente

        // On augmente la priorité de tous les autres processus
        for (int i = 0; i < NB_PROC; i++) {
            if (ready_active_process[i] != NULL) {
                ready_active_process[i]->priority--; // On diminue la priorité de tous les autres processus
                // On vérifie si la priorité est devenue négative
                if (ready_active_process[i]->priority < 0) {
                    ready_active_process[i]->priority = 0; // On remet la priorité à 0
                }
            }
        }
        // La liste est toujours triée par priorité
    }
}

/* Stops process execution and returns it s stack */
void * stop_process_uc() {
    // Arrête l'exécution du processus et renvoie sa pile
    // Implémentation spécifique à la gestion des processus
    return current_process->stack; // Renvoie la pile du processus en cours
}


/* Arrêt du processus - passage de l'état élu à prêt actif */
void arreter() {
    pid_t pid = getpid();
    if (process_table[pid].state == ELU) {
        // On le passe de l'état ELU à PRET_ACTIF
        process_table[pid].state = PRET_ACTIF;
        // On ajoute le processus en cours d'exécution à la liste des processus actifs
        addProcess(pid);
        // On arrête le processus en cours d'exécution et on met à jour sa stack
        process_table[pid].stack = stop_process_uc();
        // On appelle le processeur pour lui demander de choisir une nouvelle tâche
        scheduler();
    }
}


// Fix for create function
pid_t create() {
    pid_t Pid;
    
    if ((Pid = Allouer_Pid()) == (pid_t)-1) {
        printf("Cannot create new process\n");
        shutdown(1); // Shutdown if no PID available
    }
    process_table[Pid].pid = Pid;
    process_table[Pid].stack = init_stack();
    process_table[Pid].state = PRET_SUSPENDU;
    process_table[Pid].resources = init_resources();
    
    return Pid;
}

/* Activation du processus - passage de l'état suspendu à actif */
void activer(pid_t pid) {
    if (process_table[pid].state == PRET_SUSPENDU) {
        process_table[pid].state = PRET_ACTIF;
        addProcess(pid);
        scheduler();
    }
    else if (process_table[pid].state == BLOQUE_SUSPENDU) {
        process_table[pid].state = BLOQUE_ACTIF;
        // Pas besoin d'ajouter à la file des ressources car déjà fait lors du blocage
    }
}

/* Suspension du processus - passage de l'état actif à suspendu */
void suspendre(pid_t pid) {
    if (process_table[pid].state == ELU) {
        process_table[pid].state = PRET_SUSPENDU;
        scheduler(); // Appeler le scheduler pour élire un nouveau processus
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
}


/* Blocage du processus sur une ressource */
void bloquer(RESOURCE_ID rid) {
    // On récupère le pid du processus en cours
    pid_t pid = getpid();
    if (process_table[pid].state == ELU) {
        process_table[pid].state = BLOQUE_ACTIF;
        // On l'ajoute dans la file d'attente de la ressource bloquante
        addResource(rid, pid);
        // On arrête le processus en cours d'exécution et on met à jour sa stack
        process_table[pid].stack = stop_process_uc();
        // On appelle le scheduler pour élire un nouveau processus
        scheduler();
    }
}

/* Déblocage du processus d'une ressource */
void debloquer(pid_t pid, RESOURCE_ID rid) {
    if (process_table[pid].state == BLOQUE_ACTIF) {
        // On change l'état du processus
        process_table[pid].state = PRET_ACTIF;
        // On supprime le processus de la file d'attente de la ressource
        removeResource(rid, pid);
        // On ajoute le processus à la file des processus prêts
        addProcess(pid);
        // On vérifie si le processus débloqué est plus prioritaire que le processus en cours
        scheduler();
    }
    else if (process_table[pid].state == BLOQUE_SUSPENDU) {
        // On change l'état du processus
        process_table[pid].state = PRET_SUSPENDU;
        // On supprime le processus de la file d'attente de la ressource
        removeResource(rid, pid);
        // Le processus est suspendu, il n'est pas ajouté à la file des processus prêts
    }
}
