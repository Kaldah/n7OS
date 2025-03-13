#include  <n7OS/console.h>
#include  <n7OS/cpu.h>

#define BLINK   0<<7
#define BACK    BLACK<<4
#define TEXT    L_GREEN
#define CHAR_COLOR (BLINK|BACK|TEXT)

#define PORT_CMD 0x3D4
#define PORT_DATA 0x3D5

#define LSB_CMD 0xF
#define MSB_CMD 0xE

uint16_t *scr_tab;

void init_console() {
    scr_tab= (uint16_t *) SCREEN_ADDR;
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

    

    if (c >= 32 && c <= 126) {
        scr_tab[cursor_pos]= CHAR_COLOR<<8|c;
        cursor_pos++;

    } else if (c == '\n') {
        // New line
        cursor_pos = cursor_pos + VGA_WIDTH - cursor_pos % VGA_WIDTH;

    } else if (c == '\f') {
        // Clear the screen
        for (int i = 0; i < VGA_SIZE; i++) {
            scr_tab[i] = CHAR_COLOR << 8 | ' ';
        }
        cursor_pos = 0;

    } else if (c == '\b') {
        // Backspace
        cursor_pos--;

    } else if (c == '\t') {
        cursor_pos = cursor_pos + 4;

    } else if (c == '\r') {
        cursor_pos = cursor_pos - (cursor_pos % VGA_WIDTH);
    }
    // Check if the cursor will be out of the screen
    check_screen(&cursor_pos);

    // Set the cursor to the new position after the character has been printed
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
}