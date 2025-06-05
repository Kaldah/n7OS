#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <inttypes.h>

// Codes pour les touches spéciales
#define KEY_ESCAPE 0x01
#define KEY_BACKSPACE 0x0E
#define KEY_ENTER 0x1C
#define KEY_LEFT_SHIFT 0x2A
#define KEY_RIGHT_SHIFT 0x36

// Codes pour les flèches (codes étendus, précédés d'un code 0xE0)
#define KEY_EXTENDED 0xE0
#define KEY_UP 0x48
#define KEY_DOWN 0x50
#define KEY_LEFT 0x4B
#define KEY_RIGHT 0x4D
#define KEY_HOME 0x47
#define KEY_END 0x4F

// Types de gestionnaires d'événements clavier
typedef enum {
    KB_NORMAL_KEY,    // Touche normale (caractère ASCII)
    KB_SPECIAL_KEY,   // Touche spéciale (entrée, échap, etc.)
    KB_ARROW_KEY      // Touches fléchées et de navigation
} keyboard_event_type_t;

// Callback pour les événements clavier
typedef void (*keyboard_callback_t)(keyboard_event_type_t type, char c, uint8_t scancode);

// Initialise l'interruption clavier et la gestion du clavier
void init_keyboard();

// Fonction de traitement de l'interruption clavier
void handler_en_C_KEYBOARD();

// Enregistrer un callback pour les événements clavier
void register_keyboard_callback(keyboard_callback_t callback);

// Supprimer un callback pour les événements clavier
void unregister_keyboard_callback(keyboard_callback_t callback);

// Fonctions pour la compatibilité avec l'ancien code
void user_mode_toggle();
void user_mode_init();
void user_mode_execute_command();
const char* user_mode_get_buffer();

#endif
