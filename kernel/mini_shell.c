#include <n7OS/console.h>
#include <n7OS/keyboard.h>
#include <n7OS/printk.h>
#include <n7OS/processus.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

// Forward declarations for additional kernel-related utilities
#include <n7OS/mem.h>       // Memory management functions
#include <n7OS/paging.h>    // Paging info
#include <n7OS/time.h>      // Timing information
#include <n7OS/kheap.h>     // Heap information

// External function declarations
extern void print_mem(uint32_t nb_pages);

// Mise à jour d'une variable globale pour le mode utilisateur, utilisée par user_mode.h
int mini_shell_active = 0;

// Configuration du mini-shell
#define SHELL_BUFFER_SIZE 256
#define SHELL_PROMPT "n7OS$ "
#define SHELL_PROMPT_LEN 6
#define MAX_COMMAND_HISTORY 10

// Structure pour suivre le curseur utilisateur
typedef struct {
    uint16_t position;       // Position actuelle dans la mémoire d'écran
    uint16_t prompt_start;   // Position de début du prompt
    uint16_t buffer_pos;     // Position actuelle dans le buffer
    uint16_t buffer_len;     // Longueur actuelle du buffer
    char buffer[SHELL_BUFFER_SIZE];  // Buffer pour stocker la commande
    char history[MAX_COMMAND_HISTORY][SHELL_BUFFER_SIZE];  // Historique des commandes
    int history_count;       // Nombre de commandes dans l'historique
    int history_index;       // Index actuel dans l'historique
} shell_context_t;

// Variables globales pour le mini-shell
static shell_context_t shell_ctx;
static int shell_initialized = 0;

// Accès à la mémoire écran et fonctions de console
extern uint16_t *scr_tab;
extern void console_putbytes(const char *s, int len);
extern uint16_t get_mem_cursor();
extern void set_mem_cursor(uint16_t pos);
extern void check_screen(uint16_t *pos);

// Couleurs pour l'affichage
#define NORMAL_COLOR 0x0F00  // Blanc sur fond noir (attributs VGA)
#define CURSOR_COLOR 0x7000  // Noir sur fond gris (curseur utilisateur)

// Prototypes de fonctions locales
static void shell_display_prompt(void);
static void shell_execute_command(void);
static void shell_update_cursor(void);
static void shell_clear_line(void);
static void shell_add_to_history(void);
static void shell_load_from_history(int direction);
static void mini_shell_keyboard_handler(keyboard_event_type_t type, char c, uint8_t scancode);

// Initialisation du mini-shell
void mini_shell_init(void) {
    if (shell_initialized) return;
    
    // Initialiser le contexte du shell
    memset(&shell_ctx, 0, sizeof(shell_context_t));
    shell_ctx.position = get_mem_cursor();
    shell_ctx.prompt_start = shell_ctx.position;
    shell_ctx.buffer_pos = 0;
    shell_ctx.buffer_len = 0;
    shell_ctx.history_count = 0;
    shell_ctx.history_index = -1;
    
    // Enregistrer le gestionnaire d'événements clavier du mini-shell
    register_keyboard_callback(mini_shell_keyboard_handler);
    
    // Afficher le prompt initial
    shell_display_prompt();
    
    // Marquer comme initialisé
    shell_initialized = 1;
}

// Entrée principale du mini-shell, appelée comme un processus
void mini_shell_process(void) {
    // Initialisation
    printfk("Démarrage du mini-shell...\n");
    
    // Activer le mini-shell
    mini_shell_active = 1;
    mini_shell_init();
    
    // Affichage du message de bienvenue
    printfk("\n****************************************\n");
    printfk("* Mini-Shell n7OS v1.0                *\n");
    printfk("* Tapez 'help' pour la liste des commandes *\n");
    printfk("****************************************\n");
    printfk("Commandes disponibles: help, clear, version, ps, meminfo, uptime, kinfo, exit\n\n");
    
    // Boucle principale - dans un vrai système, on attendrait des événements clavier
    // Dans notre cas, les entrées seront traitées par les gestionnaires d'interruption clavier
    while (1) {
        // Attente passive - dans un vrai système, on attendrait des événements
        // On pourrait mettre un yield() ici pour céder le processeur
        // Ou utiliser un mécanisme de sémaphore pour bloquer jusqu'à une entrée
    }
}

// Affiche le prompt du shell
static void shell_display_prompt(void) {
    uint16_t cursor = get_mem_cursor();
    shell_ctx.prompt_start = cursor;
    console_putbytes(SHELL_PROMPT, SHELL_PROMPT_LEN);
    shell_ctx.position = get_mem_cursor();
    shell_update_cursor();
}

// Traite une touche en mode shell
void mini_shell_process_key(char c) {
    if (c == '\n') {
        // Touche Entrée - Exécuter la commande
        shell_ctx.buffer[shell_ctx.buffer_len] = '\0';
        console_putbytes("\n", 1);
        shell_execute_command();
        shell_add_to_history();
        shell_ctx.buffer_len = 0;
        shell_ctx.buffer_pos = 0;
        shell_display_prompt();
    } else if (c == '\b') {
        // Touche Retour arrière
        if (shell_ctx.buffer_pos > 0) {
            // Supprimer le caractère du buffer
            if (shell_ctx.buffer_pos == shell_ctx.buffer_len) {
                // Si on est à la fin du buffer
                shell_ctx.buffer_len--;
                shell_ctx.buffer_pos--;
                shell_ctx.position--;
                
                // Effacer le caractère à l'écran
                set_mem_cursor(shell_ctx.position);
                console_putbytes(" ", 1);
                set_mem_cursor(shell_ctx.position);
            } else {
                // Si on est au milieu du buffer, décaler tous les caractères
                for (int i = shell_ctx.buffer_pos - 1; i < shell_ctx.buffer_len - 1; i++) {
                    shell_ctx.buffer[i] = shell_ctx.buffer[i + 1];
                }
                shell_ctx.buffer_len--;
                shell_ctx.buffer_pos--;
                
                // Réafficher la ligne
                shell_clear_line();
                set_mem_cursor(shell_ctx.prompt_start);
                console_putbytes(SHELL_PROMPT, SHELL_PROMPT_LEN);
                console_putbytes(shell_ctx.buffer, shell_ctx.buffer_len);
                shell_ctx.position = shell_ctx.prompt_start + SHELL_PROMPT_LEN + shell_ctx.buffer_pos;
                set_mem_cursor(shell_ctx.position);
            }
        }
        shell_update_cursor();
    } else {
        // Caractère normal
        if (shell_ctx.buffer_len < SHELL_BUFFER_SIZE - 1) {
            if (shell_ctx.buffer_pos == shell_ctx.buffer_len) {
                // Ajouter à la fin
                shell_ctx.buffer[shell_ctx.buffer_pos] = c;
                shell_ctx.buffer_pos++;
                shell_ctx.buffer_len++;
                
                // Afficher le caractère
                char buf[2] = {c, '\0'};
                console_putbytes(buf, 1);
                shell_ctx.position++;
            } else {
                // Insérer au milieu
                for (int i = shell_ctx.buffer_len; i > shell_ctx.buffer_pos; i--) {
                    shell_ctx.buffer[i] = shell_ctx.buffer[i - 1];
                }
                shell_ctx.buffer[shell_ctx.buffer_pos] = c;
                shell_ctx.buffer_pos++;
                shell_ctx.buffer_len++;
                
                // Réafficher toute la ligne
                shell_clear_line();
                set_mem_cursor(shell_ctx.prompt_start);
                console_putbytes(SHELL_PROMPT, SHELL_PROMPT_LEN);
                console_putbytes(shell_ctx.buffer, shell_ctx.buffer_len);
                shell_ctx.position = shell_ctx.prompt_start + SHELL_PROMPT_LEN + shell_ctx.buffer_pos;
                set_mem_cursor(shell_ctx.position);
            }
        }
        shell_update_cursor();
    }
}

// Traite les touches fléchées
void mini_shell_process_arrow(uint8_t scancode) {
    switch (scancode) {
        case 0x4B: // Flèche gauche
            if (shell_ctx.buffer_pos > 0) {
                shell_ctx.buffer_pos--;
                shell_ctx.position--;
                set_mem_cursor(shell_ctx.position);
            }
            break;
            
        case 0x4D: // Flèche droite
            if (shell_ctx.buffer_pos < shell_ctx.buffer_len) {
                shell_ctx.buffer_pos++;
                shell_ctx.position++;
                set_mem_cursor(shell_ctx.position);
            }
            break;
            
        case 0x48: // Flèche haut (historique précédent)
            shell_load_from_history(-1);
            break;
            
        case 0x50: // Flèche bas (historique suivant)
            shell_load_from_history(1);
            break;
            
        case 0x47: // Home
            shell_ctx.buffer_pos = 0;
            shell_ctx.position = shell_ctx.prompt_start + SHELL_PROMPT_LEN;
            set_mem_cursor(shell_ctx.position);
            break;
            
        case 0x4F: // End
            shell_ctx.buffer_pos = shell_ctx.buffer_len;
            shell_ctx.position = shell_ctx.prompt_start + SHELL_PROMPT_LEN + shell_ctx.buffer_len;
            set_mem_cursor(shell_ctx.position);
            break;
    }
    
    shell_update_cursor();
}

// Met à jour le curseur utilisateur (surlignement)
static void shell_update_cursor(void) {
    // Sauvegarder le caractère et ses attributs
    uint16_t current_char = scr_tab[shell_ctx.position];
    char c = current_char & 0xFF;
    
    // Appliquer le surlignement et restaurer le caractère
    scr_tab[shell_ctx.position] = (CURSOR_COLOR | c);
    
    // Mettre à jour la position du curseur hardware
    set_mem_cursor(shell_ctx.position);
}

// Efface la ligne courante depuis le prompt
static void shell_clear_line(void) {
    uint16_t current_pos = shell_ctx.prompt_start;
    set_mem_cursor(current_pos);
    
    // Calculer le nombre de caractères à effacer
    int chars_to_clear = SHELL_PROMPT_LEN + shell_ctx.buffer_len;
    
    // Effacer les caractères
    for (int i = 0; i < chars_to_clear; i++) {
        console_putbytes(" ", 1);
    }
    
    // Repositionner au début
    set_mem_cursor(shell_ctx.prompt_start);
}

// Ajoute la commande courante à l'historique
static void shell_add_to_history(void) {
    if (shell_ctx.buffer_len == 0) return;
    
    // Vérifier si la commande est identique à la dernière
    if (shell_ctx.history_count > 0 && strcmp(shell_ctx.buffer, 
                                             shell_ctx.history[shell_ctx.history_count - 1]) == 0) {
        return;
    }
    
    // Décaler l'historique si nécessaire
    if (shell_ctx.history_count == MAX_COMMAND_HISTORY) {
        for (int i = 0; i < MAX_COMMAND_HISTORY - 1; i++) {
            strcpy(shell_ctx.history[i], shell_ctx.history[i + 1]);
        }
        shell_ctx.history_count--;
    }
    
    // Ajouter la nouvelle commande
    strcpy(shell_ctx.history[shell_ctx.history_count], shell_ctx.buffer);
    shell_ctx.history_count++;
    shell_ctx.history_index = shell_ctx.history_count;
}

// Charge une commande depuis l'historique
static void shell_load_from_history(int direction) {
    if (shell_ctx.history_count == 0) return;
    
    if (direction < 0) {
        // Flèche haut: commande précédente
        if (shell_ctx.history_index > 0) {
            shell_ctx.history_index--;
        } else {
            return; // Déjà à la première commande
        }
    } else {
        // Flèche bas: commande suivante
        if (shell_ctx.history_index < shell_ctx.history_count - 1) {
            shell_ctx.history_index++;
        } else if (shell_ctx.history_index == shell_ctx.history_count - 1) {
            shell_ctx.history_index = shell_ctx.history_count;
            // Effacer la ligne et remettre un buffer vide
            shell_clear_line();
            set_mem_cursor(shell_ctx.prompt_start);
            console_putbytes(SHELL_PROMPT, SHELL_PROMPT_LEN);
            memset(shell_ctx.buffer, 0, SHELL_BUFFER_SIZE);
            shell_ctx.buffer_len = 0;
            shell_ctx.buffer_pos = 0;
            shell_ctx.position = shell_ctx.prompt_start + SHELL_PROMPT_LEN;
            set_mem_cursor(shell_ctx.position);
            shell_update_cursor();
            return;
        }
    }
    
    // Charger la commande correspondante
    strcpy(shell_ctx.buffer, shell_ctx.history[shell_ctx.history_index]);
    shell_ctx.buffer_len = strlen(shell_ctx.buffer);
    shell_ctx.buffer_pos = shell_ctx.buffer_len;
    
    // Afficher la ligne mise à jour
    shell_clear_line();
    set_mem_cursor(shell_ctx.prompt_start);
    console_putbytes(SHELL_PROMPT, SHELL_PROMPT_LEN);
    console_putbytes(shell_ctx.buffer, shell_ctx.buffer_len);
    shell_ctx.position = shell_ctx.prompt_start + SHELL_PROMPT_LEN + shell_ctx.buffer_pos;
    set_mem_cursor(shell_ctx.position);
    shell_update_cursor();
}

// Helper functions for processing shell commands

// Parse command arguments (simple space-separated parsing)
static void parse_args(char *cmd, char *args[], int *argc) {
    char *token;
    *argc = 0;
    
    // Skip leading spaces
    while (*cmd == ' ') cmd++;
    
    // Get the first token
    token = cmd;
    args[(*argc)++] = token;
    
    // Get remaining tokens
    while (*cmd) {
        if (*cmd == ' ') {
            *cmd = '\0';  // Terminate the current token
            cmd++;        // Move to next character
            
            // Skip consecutive spaces
            while (*cmd == ' ') cmd++;
            
            // If we're not at the end, start a new token
            if (*cmd) {
                args[(*argc)++] = cmd;
                if (*argc >= 10) break;  // Max 10 arguments
            }
        } else {
            cmd++;
        }
    }
}

// Function to display process list in a Linux-like format
static void shell_cmd_ps(void) {
    extern struct process_t *process_table[NB_PROC];
    extern struct process_t *current_process;
    extern struct process_t *ready_active_process[NB_PROC];
    extern uint32_t time; // System uptime in milliseconds
    int total_procs = 0;
    
    // Print header similar to Linux ps command with additional info
    printfk("\n%-5s %-5s %-8s %-6s %-5s %s\n", 
           "PID", "PPID", "STATE", "PRIO", "ADDR", "NAME");
    printfk("------ ------ --------- ------ ------ ----------------\n");
    
    // Iterate through process table
    for (int i = 0; i < NB_PROC; i++) {
        if (process_table[i] != NULL) {
            total_procs++;
            const char *state_str;
            char state_char = '?';
            
            // State character (similar to Linux ps)
            switch (process_table[i]->state) {
                case ELU:             state_str = "running"; state_char = 'R'; break;
                case PRET_ACTIF:      state_str = "ready";   state_char = 'S'; break;
                case PRET_SUSPENDU:   state_str = "sleep";   state_char = 'S'; break;
                case BLOQUE_ACTIF:    state_str = "blocked"; state_char = 'D'; break;
                case BLOQUE_SUSPENDU: state_str = "bsusp";   state_char = 'T'; break;
                case TERMINE:         state_str = "zombie";  state_char = 'Z'; break;
                default:              state_str = "???";     state_char = '?'; break;
            }
            
            // Check if this process is the current one
            char current_mark = (current_process && process_table[i]->pid == current_process->pid) ? '*' : ' ';
            
            // Calculate stack address (for info)
            uint32_t stack_addr = (uint32_t)process_table[i]->stack;
            
            // Print process info (Linux-style format)
            printfk("%4d%c %-5d %-8c %-6d 0x%04x %s\n",
                   process_table[i]->pid,
                   current_mark,
                   process_table[i]->ppid,
                   state_char,
                   process_table[i]->priority,
                   stack_addr & 0xFFFF, // Show only last 4 hex digits
                   process_table[i]->name ? process_table[i]->name : "<unknown>");
        }
    }
    
    // Print summary
    printfk("\nTotal: %d processus", total_procs);
    if (current_process) {
        printfk(", PID courant: %d (%s)\n", 
               current_process->pid, 
               current_process->name ? current_process->name : "<unknown>");
    } else {
        printfk("\n");
    }
}

// Function to display memory information
static void shell_cmd_meminfo(int nb_pages) {
    extern uint32_t *free_page_bitmap_table;
    printfk("=== État de la mémoire physique ===\n");
    printfk("Taille d'une page: %d octets\n", PAGE_SIZE);
    printfk("Nombre total de pages: %d\n", PAGE_NUMBER);
    
    // Calculate statistics
    int total_pages = 0;
    int free_pages = 0;
    int allocated_pages = 0;
    
    if (free_page_bitmap_table != NULL) {
        // Calculate allocated pages from bitmap
        for (int i = 0; i < BIT_MAP_SIZE; i++) {
            uint32_t bitmap_entry = free_page_bitmap_table[i];
            for (int j = 0; j < 32; j++) {
                total_pages++;
                if (bitmap_entry & (1 << j)) {
                    allocated_pages++;
                } else {
                    free_pages++;
                }
                
                if (total_pages >= PAGE_NUMBER) break;
            }
            if (total_pages >= PAGE_NUMBER) break;
        }
        
        // Print memory usage summary
        printfk("Pages allouées: %d (%d KB)\n", allocated_pages, allocated_pages * PAGE_SIZE / 1024);
        printfk("Pages libres: %d (%d KB)\n", free_pages, free_pages * PAGE_SIZE / 1024);
        printfk("Utilisation mémoire: %d%%\n", (allocated_pages * 100) / total_pages);
        
        // Print visual memory map if requested
        if (nb_pages > 0) {
            printfk("\nCarte mémoire (1=alloué, 0=libre):\n");
            print_mem(nb_pages);
        }
    } else {
        printfk("Gestionnaire de mémoire non initialisé\n");
    }
    
    // Display heap information (if available)
#ifdef KHEAP_H
    extern uint32_t kheap_start;
    extern uint32_t kheap_end;
    extern uint32_t kheap_max;
    
    printfk("\n=== État du tas noyau ===\n");
    printfk("Début du tas: 0x%08x\n", kheap_start);
    printfk("Position actuelle: 0x%08x\n", kheap_end);
    printfk("Taille maximale: 0x%08x\n", kheap_max);
    printfk("Taille utilisée: %d Ko (%d octets)\n", 
           (kheap_end - kheap_start) / 1024, kheap_end - kheap_start);
    printfk("Taille disponible: %d Ko (%d octets)\n", 
           (kheap_max - kheap_end) / 1024, kheap_max - kheap_end);
    printfk("Utilisation du tas: %d%%\n", 
           (int)((kheap_end - kheap_start) * 100 / (kheap_max - kheap_start)));
#endif
}

// Function to display uptime
static void shell_cmd_uptime(void) {
    extern uint32_t time;  // From time.c
    
    uint32_t ticks = time;
    uint32_t seconds = ticks / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    
    printfk("Temps écoulé depuis le démarrage: %dh %dm %ds (%d ticks)\n", 
            hours, minutes, seconds, ticks);
}

// Function to display kernel information
static void shell_cmd_kinfo(void) {
    extern struct process_t *process_table[NB_PROC];
    extern uint32_t nb_ready_active_process;
    
    printfk("=== Informations sur le noyau n7OS ===\n");
    
    // Count processes
    int total_processes = 0;
    int running_processes = 0;
    int blocked_processes = 0;
    int zombie_processes = 0;
    
    for (int i = 0; i < NB_PROC; i++) {
        if (process_table[i] != NULL) {
            total_processes++;
            
            switch (process_table[i]->state) {
                case ELU:
                case PRET_ACTIF:
                    running_processes++;
                    break;
                case BLOQUE_ACTIF:
                case BLOQUE_SUSPENDU:
                    blocked_processes++;
                    break;
                case TERMINE:
                    zombie_processes++;
                    break;
            }
        }
    }
    
    printfk("Configuration système:\n");
    printfk("- Nombre max de processus: %d\n", NB_PROC);
    printfk("- Taille de pile par processus: %d octets\n", STACK_SIZE * sizeof(uint32_t));
    printfk("- Quantum de temps: %d ms\n", TIME_SLOT);
    
    printfk("\nStatistiques des processus:\n");
    printfk("- Processus total: %d\n", total_processes);
    printfk("- Processus actifs: %d\n", running_processes);
    printfk("- Processus bloqués: %d\n", blocked_processes);
    printfk("- Processus terminés (zombies): %d\n", zombie_processes);
    printfk("- Processus prêts: %d\n", nb_ready_active_process);
    
#ifdef TIME_H
    shell_cmd_uptime();
#endif
}

// Exécute la commande saisie
static void shell_execute_command(void) {
    char *args[10];  // Max 10 arguments
    int argc = 0;
    
    // Parse command and arguments
    parse_args(shell_ctx.buffer, args, &argc);
    
    if (argc == 0) return;  // Empty command
    
    // Pour le moment, afficher la commande
    // printfk("Exécution de la commande: %s\n", args[0]);
    
    // Commandes de base
    if (strcmp(args[0], "help") == 0) {
        printfk("Commandes disponibles:\n");
        printfk("  help           - Affiche cette aide\n");
        printfk("  clear          - Efface l'écran\n");
        printfk("  version        - Affiche la version du système\n");
        printfk("  ps             - Liste les processus en cours (style Linux)\n");
        printfk("  meminfo [n]    - Affiche l'état de la mémoire (n pages, défaut=128)\n");
        printfk("  uptime         - Affiche le temps écoulé depuis le démarrage\n");
        printfk("  kinfo          - Affiche des informations sur le noyau\n");
        printfk("  exit           - Quitte le mini-shell\n");
    } else if (strcmp(args[0], "clear") == 0) {
        // Effacer l'écran
        for (int i = 0; i < 25 * 80; i++) {
            scr_tab[i] = NORMAL_COLOR | ' ';
        }
        set_mem_cursor(0);
        shell_ctx.prompt_start = 0;
        shell_ctx.position = SHELL_PROMPT_LEN;
    } else if (strcmp(args[0], "version") == 0) {
        printfk("n7OS version 1.0 - Projet ASR 2SN\n");
    } else if (strcmp(args[0], "ps") == 0) {
        shell_cmd_ps();
    } else if (strcmp(args[0], "meminfo") == 0) {
        int nb_pages = 128;  // Default value
        if (argc > 1) {
            // Convert string to int
            nb_pages = 0;
            for (int i = 0; args[1][i] != '\0'; i++) {
                if (args[1][i] >= '0' && args[1][i] <= '9') {
                    nb_pages = nb_pages * 10 + (args[1][i] - '0');
                }
            }
        }
        shell_cmd_meminfo(nb_pages);
    } else if (strcmp(args[0], "uptime") == 0) {
        shell_cmd_uptime();
    } else if (strcmp(args[0], "kinfo") == 0) {
        shell_cmd_kinfo();
    } else if (strcmp(args[0], "exit") == 0) {
        printfk("Sortie du mini-shell. Appuyez sur ESC pour relancer.\n");
        // Désactiver le mini-shell
        mini_shell_active = 0;
        unregister_keyboard_callback(mini_shell_keyboard_handler);
    } else if (shell_ctx.buffer_len > 0) {
        printfk("Commande inconnue: %s\n", args[0]);
    }
}

// Implémentation du gestionnaire d'événements clavier pour le mini-shell
static void mini_shell_keyboard_handler(keyboard_event_type_t type, char c, uint8_t scancode) {
    // Ne traiter les événements que si le mini-shell est actif
    if (!mini_shell_active) return;
    
    switch (type) {
        case KB_NORMAL_KEY:
            // Touches normales (caractères ASCII)
            mini_shell_process_key(c);
            break;
        
        case KB_SPECIAL_KEY:
            // Touches spéciales (entrée, échap, etc.)
            if (c == '\n' || c == '\b' || c == 27) { // Entrée, backspace ou échap
                mini_shell_process_key(c);
            }
            break;
        
        case KB_ARROW_KEY:
            // Touches fléchées et de navigation
            mini_shell_process_arrow(scancode);
            break;
    }
}

// Fonction pour activer/désactiver le mini-shell (appelée quand l'utilisateur appuie sur ESC)
void mini_shell_toggle(void) {
    mini_shell_active = !mini_shell_active;
    
    if (mini_shell_active) {
        // Affiche un message pour indiquer que le mode shell est activé
        printfk("\n****************************************\n");
        printfk("* Mode utilisateur activé (mini-shell) *\n");
        printfk("****************************************\n");
        
        // Initialiser le mini-shell
        mini_shell_init();
    } else {
        // Désactiver le mini-shell et retirer son gestionnaire d'événements
        if (shell_initialized) {
            unregister_keyboard_callback(mini_shell_keyboard_handler);
        }
        
        // Affiche un message pour indiquer que le mode shell est désactivé
        printfk("\n****************************************\n");
        printfk("* Mode utilisateur désactivé           *\n");
        printfk("****************************************\n");
    }
}
