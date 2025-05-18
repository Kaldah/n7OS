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

void init_console() {
    scr_tab= (uint16_t *) SCREEN_ADDR;
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
    // we move the screen up by one line
    memmove(scr_tab, scr_tab + VGA_WIDTH, (VGA_SIZE - VGA_WIDTH) * sizeof(uint16_t));
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
        cursor_pos--;

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