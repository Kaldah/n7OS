/**
 * @file processus.h
 * @brief Gestion des processus dans le noyau
 * @author Corentin COUSTY
 */

#ifndef _PROCESSUS_H
#define _PROCESSUS_H

#include <inttypes.h>
#include <stddef.h>

#define NB_PROC 255 // Nombre maximum de processus
#define STACK_SIZE 1024 // Taille de la pile d'un processus
#define MAX_RESOURCES 10 // Nombre maximum de ressources par processus

typedef enum {ELU, PRET_ACTIF, PRET_SUSPENDU, BLOQUE_ACTIF,
    BLOQUE_SUSPENDU} PROCESS_STATE;

typedef uint32_t pid_t; // Type pour l'identifiant de processus

typedef uint32_t RESOURCE_ID; // Type pour l'identifiant de ressource

typedef RESOURCE_ID * RESOURCES_LIST; // Type pour la liste des ressources d'un processus

/**
 * @brief Structure pour stocker les registres de la pile d'un processus
 */
typedef union {
    struct {
        uint32_t ebx; // Base index
        uint32_t esp; // Stack pointer
        uint32_t ebp; // Base pointer
        uint32_t esi; // Source index
        uint32_t edi; // Destination index
    } s;
    uint32_t regs[5]; // Array access to the registers
} stack_context_t;

/**
 * @brief Structure de données pour un processus
 * 
 */
struct process_t {
    char *name; // Nom du processus
    pid_t pid; // Identifiant du processus
    pid_t ppid; // Identifiant du processus parent
    PROCESS_STATE state; // État du processus
    uint32_t priority; // Priorité du processus
    void* stack;  // Pointeur vers la pile du processus
    stack_context_t context; // Contexte d'exécution du processus
    RESOURCES_LIST resources; // Liste des ressources utilisées par le processus
};

/* Gestion de la création et de la destruction des processus : */
pid_t create(void * program, char *name); /* Crée un nouveau processus */

void activer(pid_t pid); /* Active un processus suspendu */

void suspendre(pid_t pid); /* Suspend un processus actif */

void arreter(); /* Arrête le processus en cours d'exécution */

void bloquer(RESOURCE_ID rid); /* Bloque le processus en cours sur une ressource */

void debloquer(pid_t pid, RESOURCE_ID rid); /* Débloque un processus d'une ressource */

/* Gestion de la file d’attente des processus prêts : */
void addProcess(pid_t pid ); /* Add the process pid in the queue */

void removeProcess (pid_t pid ) ; /* Remove the process from the queue */

/* Gestion de la file d’attente des ressources :*/
/* Add the process pid to the queue of resource rid */
void addResource (RESOURCE_ID rid , pid_t pid );

/* remove the process pid from the queue of resource rid */
void removeResource (RESOURCE_ID rid , pid_t pid ) ;

/* Gestion de l’unité centrale :*/
void scheduler();

/* Stops process execution and returns it s stack */
void * stop_process_uc();

/* pid of the running process */
pid_t getpid();

/* pid of the parent process */
pid_t getppid();
#endif