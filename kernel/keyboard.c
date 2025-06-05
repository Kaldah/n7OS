#include <n7OS/keyboard.h>
#include <n7OS/cpu.h>
#include <n7OS/console.h>
#include <n7OS/printk.h>
#include <stdio.h>
#include <n7OS/irq.h>
#include <inttypes.h>
#include <n7OS/mini_shell.h>   // Pour le mini-shell

// Ports d'E/S du clavier
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

// Définition de touches spéciales
#define KEY_ESCAPE 0x01
#define KEY_BACKSPACE 0x0E
#define KEY_ENTER 0x1C
#define KEY_LEFT_SHIFT 0x2A
#define KEY_RIGHT_SHIFT 0x36
#define KEY_LEFT_SHIFT_RELEASED 0xAA
#define KEY_RIGHT_SHIFT_RELEASED 0xB6

// Codes pour les flèches (codes étendus, précédés d'un code 0xE0)
#define KEY_EXTENDED 0xE0
#define KEY_UP 0x48
#define KEY_DOWN 0x50
#define KEY_LEFT 0x4B
#define KEY_RIGHT 0x4D
#define KEY_HOME 0x47   // Touche Début de ligne
#define KEY_END 0x4F    // Touche Fin de ligne

// État du clavier
static int shift_pressed = 0;
static int extended_key = 0; // Pour détecter les touches étendues (flèches, etc.)

// Déclaration externe pour l'état du mini-shell
extern int mini_shell_active;

// Mécanisme de callbacks
#define MAX_KEYBOARD_CALLBACKS 5
static keyboard_callback_t keyboard_callbacks[MAX_KEYBOARD_CALLBACKS] = {0};
static int num_callbacks = 0;

// Fonction utilitaire pour notifier tous les callbacks enregistrés (déclaration)
static void notify_keyboard_callbacks(keyboard_event_type_t type, char c, uint8_t scancode);

// Déclarations externes des fonctions de console
extern void console_putbytes(const char *s, int len);
extern uint16_t get_mem_cursor();
extern void set_mem_cursor(uint16_t pos);
extern void check_screen(uint16_t *pos);
extern uint16_t *scr_tab;

// Table de correspondance scancode -> ASCII (pour un clavier AZERTY français)
// Position = scancode, valeur = caractère ASCII (caractères accentués simplifiés)
static char keyboard_map[128] = {
    0, 27, '&', '2', '"', '\'', '(', '-', '_', '_', 'c', 'a', ')', '=', '\b',
    '\t', 'a', 'z', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '^', '$', '\n',
    0, 'q', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 'u', '*', 0,
    '<', 'w', 'x', 'c', 'v', 'b', 'n', ',', ';', ':', '!', 0, '*', 0, ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Table de correspondance avec shift appuyé (majuscules et symboles spéciaux)
static char keyboard_map_shifted[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '_', '+', '\b',
    '\t', 'A', 'Z', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'Q', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'M', '%', '~', 0,
    '>', 'W', 'X', 'C', 'V', 'B', 'N', '?', '.', '/', '#', 0, '*', 0, ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Fonction utilitaire pour notifier tous les callbacks enregistrés
static void notify_keyboard_callbacks(keyboard_event_type_t type, char c, uint8_t scancode) {
    // Appel de tous les callbacks enregistrés
    for (int i = 0; i < num_callbacks; i++) {
        if (keyboard_callbacks[i]) {
            keyboard_callbacks[i](type, c, scancode);
        }
    }
}

// Initialisation du clavier et de son interruption
void init_keyboard() {
    extern void handler_IT_KEYBOARD();
    
    // Configurer l'entrée IRQ 1 (interruption clavier) dans l'IDT
    init_irq_entry(0x21, (uint32_t) handler_IT_KEYBOARD);
    
    // Activer l'interruption du clavier (IRQ 1) sur le PIC
    // 0xFD = 11111101 en binaire (le bit 1 est mis à 0)
    outb(inb(0x21) & 0xFD, 0x21);
    
    printfk("===Initialisation du clavier===\n");
}

// Enregistrer un callback pour les événements clavier
void register_keyboard_callback(keyboard_callback_t callback) {
    if (num_callbacks < MAX_KEYBOARD_CALLBACKS) {
        keyboard_callbacks[num_callbacks++] = callback;
    }
}

// Supprimer un callback pour les événements clavier
void unregister_keyboard_callback(keyboard_callback_t callback) {
    // Trouver et supprimer le callback
    for (int i = 0; i < num_callbacks; i++) {
        if (keyboard_callbacks[i] == callback) {
            // Décaler tous les callbacks suivants
            for (int j = i; j < num_callbacks - 1; j++) {
                keyboard_callbacks[j] = keyboard_callbacks[j + 1];
            }
            keyboard_callbacks[num_callbacks - 1] = 0;
            num_callbacks--;
            break;
        }
    }
}

// Fonction de traitement de l'interruption clavier
void handler_en_C_KEYBOARD() {
    // Lecture du scancode du clavier
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    // Envoyer l'acquittement au PIC
    outb(0x20, 0x20);
    
    // Vérifie si une touche a été relâchée (bit 7 à 1)
    if (scancode & 0x80) {
        // Touche relâchée
        scancode &= 0x7F; // Enlève le bit 7
        
        // Gestion des touches shift
        if (scancode == KEY_LEFT_SHIFT || scancode == KEY_RIGHT_SHIFT) {
            shift_pressed = 0;
        }
    } else {
        // Touche appuyée
        
        // Gestion des touches shift
        if (scancode == KEY_LEFT_SHIFT || scancode == KEY_RIGHT_SHIFT) {
            shift_pressed = 1;
            return;
        }
        
        // Gestion des touches spéciales
        if (scancode == KEY_BACKSPACE) {
            // Envoyer l'événement backspace aux callbacks
            notify_keyboard_callbacks(KB_SPECIAL_KEY, '\b', scancode);
            
            // Comportement par défaut si aucun callback n'a géré l'événement
            if (!mini_shell_active) {
                char buffer[2] = {'\b', '\0'};
                console_putbytes(buffer, 1);
            }
            return;
        } else if (scancode == KEY_ENTER) {
            // Envoyer l'événement entrée aux callbacks
            notify_keyboard_callbacks(KB_SPECIAL_KEY, '\n', scancode);
            
            // Comportement par défaut si aucun callback n'a géré l'événement
            if (!mini_shell_active) {
                char buffer[2] = {'\n', '\0'};
                console_putbytes(buffer, 1);
            }
            return;
        } else if (scancode == KEY_EXTENDED) {
            // Marquer qu'une touche étendue a été détectée
            extended_key = 1;
            return;
        } else if (scancode == KEY_ESCAPE) {
            // Envoyer l'événement échap aux callbacks
            notify_keyboard_callbacks(KB_SPECIAL_KEY, 27, scancode);
            
            // Comportement par défaut si aucun callback n'a géré l'événement
            mini_shell_toggle();
            return;
        }
        
        // Si c'est une séquence de touche étendue (précédée par 0xE0)
        if (extended_key) {
            extended_key = 0; // Réinitialiser le flag
            
            // Envoyer l'événement touche fléchée aux callbacks
            notify_keyboard_callbacks(KB_ARROW_KEY, 0, scancode);
            
            // Si en mode utilisateur, ne pas traiter ici car le callback s'en occupe déjà
            if (!mini_shell_active) {
                // Sinon, utiliser la gestion normale pour le mode console
                // Récupérer la position actuelle du curseur
                uint16_t cursor_pos = get_mem_cursor();
                
                // Traiter les touches flèches
                switch (scancode) {
                    case KEY_LEFT:
                        // Flèche gauche: déplace le curseur vers la gauche
                        if (cursor_pos > 0) {
                            // Si on est au début d'une ligne (sauf la première ligne)
                            if (cursor_pos % 80 == 0 && cursor_pos >= 80) {
                                cursor_pos--; // Aller à la fin de la ligne précédente
                            } else {
                                cursor_pos--; // Juste reculer d'un caractère
                            }
                        }
                        break;
                    case KEY_RIGHT:
                        // Flèche droite: déplace le curseur vers la droite
                        // Si on est à la fin d'une ligne (sauf la dernière position)
                        if ((cursor_pos + 1) % 80 == 0 && cursor_pos < 80*25-1) {
                            cursor_pos++; // Aller au début de la ligne suivante
                        } else if (cursor_pos < 80*25-1) {
                            cursor_pos++; // Juste avancer d'un caractère
                        }
                        break;
                    case KEY_UP:
                        // Flèche haut: déplace le curseur vers le haut
                        if (cursor_pos >= 80) { // Si on n'est pas déjà sur la première ligne
                            // Conserver la même colonne lors du déplacement vertical
                            uint16_t column = cursor_pos % 80;
                            cursor_pos -= 80;
                            // Assurer que nous sommes à la même colonne
                            cursor_pos = (cursor_pos / 80) * 80 + column;
                        }
                        break;
                    case KEY_DOWN:
                        // Flèche bas: déplace le curseur vers le bas
                        if (cursor_pos < 80*24) { // Si on n'est pas déjà sur la dernière ligne
                            // Conserver la même colonne lors du déplacement vertical
                            uint16_t column = cursor_pos % 80;
                            cursor_pos += 80;
                            // Assurer que nous sommes à la même colonne
                            cursor_pos = (cursor_pos / 80) * 80 + column;
                        }
                        break;
                    case KEY_HOME:
                        // Touche Home: déplace le curseur au début de la ligne courante
                        cursor_pos = (cursor_pos / 80) * 80;
                        break;
                    case KEY_END:
                        // Touche End: déplace le curseur à la fin de la ligne courante
                        cursor_pos = (cursor_pos / 80) * 80 + 79;
                        break;
                }
                
                // Vérifier que le curseur reste dans les limites de l'écran
                check_screen(&cursor_pos);
                
                // Mettre à jour la position du curseur
                set_mem_cursor(cursor_pos);
            }
            return;
        }
        
        // Récupère le caractère correspondant au scancode
        char c = shift_pressed ? keyboard_map_shifted[scancode] : keyboard_map[scancode];
        
        // Si c'est un caractère valide, le traiter
        if (c != 0) {
            // Notifier les callbacks pour cette touche
            notify_keyboard_callbacks(KB_NORMAL_KEY, c, scancode);
            
            // Si le mini-shell est actif, ne pas traiter la touche ici car le callback s'en occupe déjà
            if (!mini_shell_active) {
                // En mode normal, afficher directement le caractère
                char buffer[2] = {c, '\0'};
                console_putbytes(buffer, 1);
            }
        }
    }
}
