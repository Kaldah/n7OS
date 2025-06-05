#include <n7OS/console.h>
#include <n7OS/keyboard.h>
#include <n7OS/printk.h>
#include <n7OS/processus.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

// Function declarations for keyboard driver
extern char kgetch(void);

// Forward declarations for additional kernel-related utilities
#include <n7OS/mem.h>
#include <n7OS/paging.h>
#include <n7OS/time.h>
#include <n7OS/kheap.h>

// External function declarations
extern void print_mem(uint32_t nb_pages);
// Assuming printfk has a similar signature to printf
extern int printfk(const char *format, ...);
extern int write(const char *buffer, int size);

// Variables globales pour le mini-shell
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

// Forward declarations
static void shell_display_prompt(void);
static void shell_execute_command(void);
static void shell_update_cursor(void);
static void shell_clear_line(void);
static void shell_add_to_history(void);
static void shell_load_from_history(int direction);
void mini_shell_process_key(char c);
void mini_shell_process_arrow(uint8_t scancode);

// Affiche le prompt du shell
static void shell_display_prompt(void) {
    uint16_t cursor = get_mem_cursor();
    shell_ctx.prompt_start = cursor;
    console_putbytes(SHELL_PROMPT, SHELL_PROMPT_LEN);
    shell_ctx.position = get_mem_cursor();
    shell_update_cursor();
}

// Met à jour le curseur utilisateur (surlignement)
static void shell_update_cursor(void) {
    uint16_t old_cursor = get_mem_cursor();
    set_mem_cursor(shell_ctx.position);
    
    // Mettre à jour l'apparence du curseur (caractère surligné)
    uint16_t ch = scr_tab[shell_ctx.position];
    scr_tab[shell_ctx.position] = (ch & 0x00FF) | CURSOR_COLOR; // Garder le caractère, changer l'attribut
    
    // Restaurer la position du curseur pour l'affichage
    set_mem_cursor(old_cursor);
}

// Efface la ligne courante depuis le prompt
static void shell_clear_line(void) {
    uint16_t current_pos = shell_ctx.prompt_start + SHELL_PROMPT_LEN;
    uint16_t old_cursor = get_mem_cursor();
    
    // Effacer les caractères
    set_mem_cursor(current_pos);
    for (int i = 0; i < shell_ctx.buffer_len; i++) {
        scr_tab[current_pos + i] = NORMAL_COLOR | ' ';
    }
    
    // Restaurer la position du curseur
    set_mem_cursor(old_cursor);
}

// Ajoute la commande courante à l'historique
static void shell_add_to_history(void) {
    if (shell_ctx.buffer_len <= 0) return;
    
    // Vérifier si la commande est déjà la dernière de l'historique
    if (shell_ctx.history_count > 0 && strcmp(shell_ctx.buffer, shell_ctx.history[shell_ctx.history_count - 1]) == 0) {
        return;
    }
    
    // Si l'historique est plein, décaler tout
    if (shell_ctx.history_count >= MAX_COMMAND_HISTORY) {
        for (int i = 0; i < MAX_COMMAND_HISTORY - 1; i++) {
            strcpy(shell_ctx.history[i], shell_ctx.history[i + 1]);
        }
        shell_ctx.history_count = MAX_COMMAND_HISTORY - 1;
    }
    
    // Ajouter la nouvelle commande
    strcpy(shell_ctx.history[shell_ctx.history_count], shell_ctx.buffer);
    shell_ctx.history_count++;
    shell_ctx.history_index = shell_ctx.history_count;
}

// Charge une commande depuis l'historique
static void shell_load_from_history(int direction) {
    if (shell_ctx.history_count == 0) return;
    
    // Calculer le nouvel index
    if (direction < 0) { // Flèche haut - historique précédent
        if (shell_ctx.history_index > 0) {
            shell_ctx.history_index--;
        }
    } else { // Flèche bas - historique suivant
        if (shell_ctx.history_index < shell_ctx.history_count - 1) {
            shell_ctx.history_index++;
        } else if (shell_ctx.history_index == shell_ctx.history_count - 1) {
            // Si on est à la dernière commande, on efface la ligne
            shell_clear_line();
            memset(shell_ctx.buffer, 0, SHELL_BUFFER_SIZE);
            shell_ctx.buffer_len = 0;
            shell_ctx.buffer_pos = 0;
            shell_ctx.history_index = shell_ctx.history_count;
            shell_ctx.position = shell_ctx.prompt_start + SHELL_PROMPT_LEN;
            set_mem_cursor(shell_ctx.position);
            return;
        }
    }
    
    // Charger la commande
    shell_clear_line();
    strcpy(shell_ctx.buffer, shell_ctx.history[shell_ctx.history_index]);
    shell_ctx.buffer_len = strlen(shell_ctx.buffer);
    shell_ctx.buffer_pos = shell_ctx.buffer_len;
    
    // Afficher la commande
    set_mem_cursor(shell_ctx.prompt_start + SHELL_PROMPT_LEN);
    console_putbytes(shell_ctx.buffer, shell_ctx.buffer_len);
    shell_ctx.position = shell_ctx.prompt_start + SHELL_PROMPT_LEN + shell_ctx.buffer_pos;
    set_mem_cursor(shell_ctx.position);
}

// Exécute une commande simple
static void handle_simple_command(const char *cmd) {
    if (strcmp(cmd, "help") == 0) {
        printfk("Commandes disponibles:\n");
        printfk("  help         - Affiche cette aide\n");
        printfk("  clear        - Efface l'écran\n");
        printfk("  echo [texte] - Affiche du texte\n");
        printfk("  exit         - Quitte le mini-shell\n");
    } else if (strcmp(cmd, "clear") == 0) {
        // Effacer l'écran
        for (int i = 0; i < 80*25; i++) {
            scr_tab[i] = NORMAL_COLOR | ' ';
        }
        set_mem_cursor(0);
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        // Afficher le texte qui suit echo
        printfk("%s\n", cmd + 5);
    } else if (strcmp(cmd, "exit") == 0) {
        mini_shell_active = 0;
    } else if (strlen(cmd) > 0) {
        printfk("Commande inconnue: %s\n", cmd);
        printfk("Tapez 'help' pour afficher la liste des commandes\n");
    }
}

// Exécute la commande saisie
static void shell_execute_command(void) {
    // Ajouter le caractère nul à la fin du buffer
    shell_ctx.buffer[shell_ctx.buffer_len] = '\0';
    
    // Exécuter la commande
    handle_simple_command(shell_ctx.buffer);
}

// Traite une touche en mode shell
void mini_shell_process_key(char c) {
    if (c == '\n' || c == '\r') {
        // Touche Entrée - Exécuter la commande
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
                
                // Effacer le caractère à l'écran avec la bonne couleur
                set_mem_cursor(shell_ctx.position);
                scr_tab[shell_ctx.position] = NORMAL_COLOR | ' '; // Utiliser la couleur normale pour effacer
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

// Process arrow keys (simplified version that doesn't rely on scan codes)
void mini_shell_process_arrow(uint8_t scancode) {
    // This function is kept for compatibility but will not be called
    // in the current polling-based implementation
}

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
    
    // Afficher le prompt initial
    shell_display_prompt();
    
    // Marquer comme initialisé
    shell_initialized = 1;
}

// Entrée principale du mini-shell, appelée comme un processus
void mini_shell_process(void) {    // Initialisation
    printfk("Démarrage du mini-shell (polling-based keyboard)...\n");
    
    // Activer le mini-shell
    mini_shell_active = 1;
    mini_shell_init();
    
    // Affichage du message de bienvenue
    printfk("\n****************************************\n");
    printfk("* Mini-Shell n7OS v1.0                *\n");
    printfk("* Tapez 'help' pour la liste des commandes *\n");
    printfk("****************************************\n\n");      // Boucle principale de traitement des commandes
    char c;    // Shell is now ready to receive input
      while (mini_shell_active) {
        // Attente d'une touche du clavier
        c = kgetch();
        
        // Traiter la touche si elle est valide
        if (c != '\0') {
            mini_shell_process_key(c);
        } else {
            // If no key was pressed, we can use a CPU halt instruction
            // to save power and be more efficient until the next interrupt
            __asm__ volatile("hlt"); // Wait for next interrupt
        }
    }
}

// Fonction pour activer/désactiver le mini-shell
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
        // Affiche un message pour indiquer que le mode shell est désactivé
        printfk("\n****************************************\n");
        printfk("* Mode utilisateur désactivé           *\n");
        printfk("****************************************\n");
    }
} 
