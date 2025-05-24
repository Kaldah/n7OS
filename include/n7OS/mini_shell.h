#ifndef _MINI_SHELL_H_
#define _MINI_SHELL_H_

#include <inttypes.h>
#include <n7OS/keyboard.h>

/**
 * Initialise le mini-shell
 */
void mini_shell_init(void);

/**
 * Processus principal du mini-shell
 */
void mini_shell_process(void);

/**
 * Traite une touche en mode shell
 * @param c Le caractère à traiter
 */
void mini_shell_process_key(char c);

/**
 * Traite les touches fléchées
 * @param scancode Le code de la touche fléchée
 */
void mini_shell_process_arrow(uint8_t scancode);

/**
 * Active ou désactive le mini-shell
 */
void mini_shell_toggle(void);

#endif /* _MINI_SHELL_H_ */
