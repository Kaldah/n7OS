#include <n7OS/console.h>
#include <n7OS/cpu.h>
#include <string.h>
#include <n7OS/time.h>
#include <n7OS/printk.h> // Add for printfk function

#define BLINK   0<<7
#define BACK    BLACK<<4
#define TEXT    L_GREEN
#define CHAR_COLOR (BLINK|BACK|TEXT)

#define PORT_CMD 0x3D4
#define PORT_DATA 0x3D5

#define LSB_CMD 0xF
#define MSB_CMD 0xE


// Position de l'heure
#define TIME_START_POSITION (VGA_WIDTH - 11) // 11 caractères pour " | 00:00:00"

uint16_t *scr_tab;

uint16_t history[HISTORY_SIZE]; // tableau pour l'historique des caractères
uint16_t history_last_line_index = 0; // index de la dernière ligne enregistrée dans l'historique
uint16_t history_current_index = 0; // index pour parcourir l'historique

void init_console() {
    scr_tab= (uint16_t *) SCREEN_ADDR;
    // On clear l'écran
    printfk("\f");
    // On affiche le temps 0
    console_puts_time("00:00:00");
}

// get the cursor position from memory
uint16_t get_mem_cursor() {

    outb(MSB_CMD, PORT_CMD);
    uint8_t msb_cursor = inb(PORT_DATA);
    outb(LSB_CMD, PORT_CMD);
    uint8_t lsb_cursor = inb(PORT_DATA);

    return (msb_cursor << 8) | lsb_cursor;
}

void scroll_up() {
    // we save the history using the history_end_index
    // Save the whole line that is about to be scrolled off
    // Pour éviter des bugs, on s'assure que l'historique est assez grand
    if (HISTORY_SIZE >= VGA_WIDTH) {
        int save_index = history_last_line_index * VGA_WIDTH;
        for (int i = 0; i < VGA_WIDTH; i++) {
            // Normalement l'historique contient un nombre de lignes donc
            // est un multiple de VGA_WIDTH mais par prudence on met un modulo
            history[(save_index + i) % HISTORY_SIZE] = scr_tab[i];
        }
        history_last_line_index = (history_last_line_index + 1) % NUMBER_OF_HISTORY_LINES;
        // Mise à jour de history_current_index pour qu'il reste au même niveau que history_end_index
        // si on n'est pas en train de parcourir l'historique
        if (history_current_index == ((history_last_line_index + NUMBER_OF_HISTORY_LINES - 1) % NUMBER_OF_HISTORY_LINES)) {
            history_current_index = history_last_line_index;
        }
    }
    // we move the screen up by one line
    memmove(scr_tab, scr_tab + VGA_WIDTH, (VGA_SIZE - VGA_WIDTH) * sizeof(uint16_t));
}

void scroll_down() {
    // On vérifie si on peut scroller vers le bas (si on n'est pas déjà en bas de l'historique)
    if (history_current_index != history_last_line_index) {
        // D'abord, nous déplaçons l'écran vers le bas pour faire de la place pour la nouvelle ligne
        memmove(scr_tab + VGA_WIDTH, scr_tab, (VGA_SIZE - VGA_WIDTH) * sizeof(uint16_t));
        
        // Ensuite, on calcule l'index précédent pour restaurer la ligne de l'historique
        // Pour obtenir la ligne précédente dans l'historique, on recule d'une position
        uint16_t prev_index = (history_current_index == 0) ? 
                            NUMBER_OF_HISTORY_LINES - 1 : 
                            history_current_index - 1;
        
        // On calcule l'index dans le tableau history[]
        int restore_index = prev_index * VGA_WIDTH;
        
        // On restaure la ligne depuis l'historique à la première ligne de l'écran
        for (int i = 0; i < VGA_WIDTH; i++) {
            scr_tab[i] = history[(restore_index + i) % HISTORY_SIZE];
        }
        
        // On met à jour l'index de l'historique
        history_current_index = prev_index;
    }
}
// check if the cursor is out of the screen
void check_screen(uint16_t * cursor_pos) {

    // if the cursor is out of the screen, we move the screen up by one line
    if (*cursor_pos >= VGA_SIZE) {
        // we move the screen up by one line
        scroll_up();
        // we clear the last line
        for (int i = VGA_SIZE - VGA_WIDTH; i < VGA_SIZE; i++) {
            scr_tab[i] = CHAR_COLOR << 8 | ' ';
        }
        // we move the cursor back to the start of the line
        *cursor_pos = VGA_SIZE - VGA_WIDTH;
    }
}

// set the cursor position in memory

void set_mem_cursor(uint16_t cursor_pos) {
    
    outb(LSB_CMD, PORT_CMD);
    // 0x00FF is used to keep only the 8 LSB
    outb(cursor_pos & 0x00FF, PORT_DATA);
    outb(MSB_CMD, PORT_CMD);
    outb(cursor_pos >> 8, PORT_DATA);
}

void console_putchar(const char c) {
    uint16_t cursor_pos = get_mem_cursor();

    // Vérification des emplacements interdits (" | 00:00:00")
    if (cursor_pos >= TIME_START_POSITION && cursor_pos < VGA_WIDTH) {
        cursor_pos = VGA_WIDTH; // Passer à la ligne suivante
    }

    if (c >= 32 && c <= 126) {
        scr_tab[cursor_pos] = CHAR_COLOR << 8 | c;
        cursor_pos++;

    } else if (c == '\n') {
        // Nouvelle ligne
        cursor_pos = cursor_pos + VGA_WIDTH - cursor_pos % VGA_WIDTH;

    } else if (c == '\f') {
        // Effacer l'écran
        for (int i = 0; i < VGA_SIZE; i++) {
            scr_tab[i] = CHAR_COLOR << 8 | ' ';
        }
        cursor_pos = 0;

    } else if (c == '\b') {
        // Retour arrière
        if (cursor_pos > 0) {
            cursor_pos--; // Déplace le curseur en arrière
            scr_tab[cursor_pos] = CHAR_COLOR << 8 | ' '; // Efface le caractère
        }

    } else if (c == '\t') {
        cursor_pos = cursor_pos + 4;

    } else if (c == '\r') {
        cursor_pos = cursor_pos - (cursor_pos % VGA_WIDTH);
    }
    // Vérifier si le curseur est hors de l'écran
    check_screen(&cursor_pos);

    // Mettre à jour la position du curseur en mémoire
    set_mem_cursor(cursor_pos);
}

void console_putbytes(const char *s, int len) {

    uint16_t cursor_pos = get_mem_cursor();

    // On retire la surbrillance du charactere du curseur
    scr_tab[cursor_pos]= CHAR_COLOR << 8 | (scr_tab[cursor_pos] & 0x00FF);

    for (int i= 0; i<len; i++) {
        console_putchar(s[i]);
    }

    // la position du curseur a changé, on la remet à jour
    cursor_pos = get_mem_cursor();

    // On met en surbrillance le charactère du curseur
    scr_tab[cursor_pos]= (1 << 7|BACK|TEXT) << 8 | (scr_tab[cursor_pos] & 0x00FF);
    
    set_mem_cursor(cursor_pos);
    // Properly use get_time_string() which returns a char* pointer
    char *time_str = get_time_string();
    console_puts_time(time_str);
}

void console_puts_time(const char *s) {

    // Ajout de " | " devant le temps s
    char formatted_time[12] = " | ";
    strncat(formatted_time, s, 9); // Concaténer le temps après " | "

    // Écriture directe dans la zone réservée pour l'heure
    for (int i = 0; i < 11; i++) {
        scr_tab[TIME_START_POSITION + i] = CHAR_COLOR << 8 | formatted_time[i];
    }
}