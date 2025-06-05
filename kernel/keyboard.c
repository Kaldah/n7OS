#include <n7OS/keyboard.h>
#include <n7OS/cpu.h>
#include <n7OS/console.h>
#include <n7OS/printk.h>
#include <n7OS/irq.h>
#include <n7OS/time.h>
#include <inttypes.h>
#include <n7OS/mini_shell.h>

// Console color definitions
#define BLACK   0x0
#define BLUE    0x1
#define GREEN   0x2
#define CYAN    0x3
#define RED     0x4
#define MAGENTA 0x5
#define BROWN   0x6
#define GRAY    0x7
#define D_GRAY  0x8
#define L_BLUE  0x9
#define L_GREEN 0xA
#define L_CYAN  0xB
#define L_RED   0xC
#define L_MAG   0xD
#define YELLOW  0xE
#define WHITE   0xF

#define BLINK   0<<7
#define BACK    BLACK<<4
#define TEXT    L_GREEN
#define CHAR_COLOR (BLINK|BACK|TEXT)
#define NORMAL_COLOR 0x0F00  // White on black (VGA attributes)
#define USER_CURSOR_COLOR (BLINK|(GRAY<<4)|WHITE)

// External function declarations
extern void mini_shell_toggle(void);
extern int printfk(const char *format, ...);
extern void init_irq_entry(int num_irq, uint32_t handler);
extern void outb(uint8_t value, uint16_t port);
extern uint8_t inb(uint16_t port);
extern uint32_t get_time(void);

// Special key codes
#define KEY_ESCAPE 0x01
#define KEY_BACKSPACE 0x0E
#define KEY_ENTER 0x1C
#define KEY_LEFT_SHIFT 0x2A
#define KEY_RIGHT_SHIFT 0x36
#define KEY_LEFT_SHIFT_RELEASED 0xAA
#define KEY_RIGHT_SHIFT_RELEASED 0xB6

// Extended key codes (preceded by 0xE0)
#define KEY_EXTENDED 0xE0
#define KEY_UP 0x48
#define KEY_DOWN 0x50
#define KEY_LEFT 0x4B
#define KEY_RIGHT 0x4D
#define KEY_HOME 0x47
#define KEY_END 0x4F

// Circular buffer for keyboard input
#define KB_BUFFER_SIZE 32
static volatile char kb_buffer[KB_BUFFER_SIZE];
static volatile int kb_buffer_head = 0;
static volatile int kb_buffer_tail = 0;
static volatile int kb_buffer_count = 0;

// Keyboard state
static int shift_pressed = 0;
static int extended_key = 0;   // For detecting extended keys (arrows, etc.)

// External declaration for mini-shell state
extern int mini_shell_active;

// Callback mechanism
#define MAX_KEYBOARD_CALLBACKS 5

// Types of keyboard event handlers from old header
typedef enum {
    KB_NORMAL_KEY,    // Normal key (ASCII character)
    KB_SPECIAL_KEY,   // Special key (enter, escape, etc.)
    KB_ARROW_KEY      // Arrow and navigation keys
} keyboard_event_type_t;

// Callback for keyboard events
typedef void (*keyboard_callback_t)(keyboard_event_type_t type, char c, uint8_t scancode);

static keyboard_callback_t keyboard_callbacks[MAX_KEYBOARD_CALLBACKS] = {0};
static int num_callbacks = 0;

// External declarations for console functions
extern void console_putbytes(const char *s, int len);
extern uint16_t get_mem_cursor();
extern void set_mem_cursor(uint16_t pos);
extern void check_screen(uint16_t *pos);
extern uint16_t *scr_tab;

// Function to add a character to the keyboard buffer
static int kb_buffer_push(char c) {
    if (kb_buffer_count >= KB_BUFFER_SIZE) {
        return 0;  // Buffer is full
    }

    kb_buffer[kb_buffer_head] = c;
    kb_buffer_head = (kb_buffer_head + 1) % KB_BUFFER_SIZE;
    kb_buffer_count++;
    return 1;
}

// Function to get a character from the keyboard buffer
static int kb_buffer_pop() {
    if (kb_buffer_count <= 0) {
        return -1;  // Buffer is empty
    }

    char c = kb_buffer[kb_buffer_tail];
    kb_buffer_tail = (kb_buffer_tail + 1) % KB_BUFFER_SIZE;
    kb_buffer_count--;
    return c;
}

// Utility function to notify all registered callbacks
static void notify_keyboard_callbacks(keyboard_event_type_t type, char c, uint8_t scancode) {
    // Call all registered callbacks
    for (int i = 0; i < num_callbacks; i++) {
        if (keyboard_callbacks[i]) {
            keyboard_callbacks[i](type, c, scancode);
        }
    }
}

// Initialize keyboard and its interrupt
void init_keyboard(void) {
    extern void handler_IT_KEYBOARD(void);
    
    // Configure IRQ 1 (keyboard interrupt) in the IDT
    // Cast the handler address directly through a union to avoid type warnings
    union {
        void (*handler)(void);
        uint32_t addr;
    } handler_union;
    handler_union.handler = handler_IT_KEYBOARD;
    
    init_irq_entry(0x21, handler_union.addr);
    
    // Enable keyboard interrupt (IRQ 1) on the PIC
    // 0xFD = 11111101 in binary (bit 1 is set to 0)
    outb(inb(0x21) & 0xFD, 0x21);
    
    printfk("===Keyboard initialization complete===\n");
}

// Register a callback for keyboard events
void register_keyboard_callback(keyboard_callback_t callback) {
    if (num_callbacks < MAX_KEYBOARD_CALLBACKS) {
        keyboard_callbacks[num_callbacks++] = callback;
    }
}

// Unregister a callback for keyboard events
void unregister_keyboard_callback(keyboard_callback_t callback) {
    // Find and remove the callback
    for (int i = 0; i < num_callbacks; i++) {
        if (keyboard_callbacks[i] == callback) {
            // Shift all subsequent callbacks
            for (int j = i; j < num_callbacks - 1; j++) {
                keyboard_callbacks[j] = keyboard_callbacks[j + 1];
            }
            keyboard_callbacks[num_callbacks - 1] = 0;
            num_callbacks--;
            break;
        }
    }
}

// Function called by getchar() to get a character from the keyboard buffer
char kgetch(void) {
    return kb_buffer_pop();
}

// Keyboard interrupt handler function
void handler_en_C_KEYBOARD(void) {
    // Read the scancode from the keyboard
    uint8_t scancode = inb(0x60); // 0x60 is the keyboard data port
    
    // Send acknowledgment to the PIC
    outb(0x20, 0x20);
    
    // Check if a key was released (bit 7 is 1)
    if (scancode & 0x80) {
        // Key released
        scancode &= 0x7F; // Remove bit 7
        
        // Handle shift keys release
        if (scancode == KEY_LEFT_SHIFT || scancode == KEY_RIGHT_SHIFT) {
            shift_pressed = 0;
        }
    } else {
        // Key pressed
        
        // Handle shift keys press
        if (scancode == KEY_LEFT_SHIFT || scancode == KEY_RIGHT_SHIFT) {
            shift_pressed = 1;
            return;
        }
        
        // Handle special keys
        if (scancode == KEY_BACKSPACE) {
            // Send backspace event to callbacks
            notify_keyboard_callbacks(KB_SPECIAL_KEY, '\b', scancode);
            
            // Add backspace to buffer for scanf/gets
            kb_buffer_push('\b');
            
            // Default behavior if no callback handled the event
            if (!mini_shell_active) {
                char buffer[2] = {'\b', '\0'};
                console_putbytes(buffer, 1);
            }
            return;
        } else if (scancode == KEY_ENTER) {
            // Send enter event to callbacks
            notify_keyboard_callbacks(KB_SPECIAL_KEY, '\n', scancode);
            
            // Add enter to buffer for scanf/gets
            kb_buffer_push('\n');
            
            // Default behavior if no callback handled the event
            if (!mini_shell_active) {
                char buffer[2] = {'\n', '\0'};
                console_putbytes(buffer, 1);
            }
            return;
        } else if (scancode == KEY_EXTENDED) {
            // Mark that an extended key was detected
            extended_key = 1;
            return;
        } else if (scancode == KEY_ESCAPE) {
            // Send escape event to callbacks
            notify_keyboard_callbacks(KB_SPECIAL_KEY, 27, scancode);
            
            // Default behavior if no callback handled the event
            mini_shell_toggle();
            return;
        }
        
        // If it's an extended key sequence (preceded by 0xE0)
        if (extended_key) {
            extended_key = 0; // Reset the flag
            
            // Send arrow key event to callbacks
            notify_keyboard_callbacks(KB_ARROW_KEY, 0, scancode);
            
            // If in user mode, don't process here as the callback already handles it
            if (!mini_shell_active) {
                // Otherwise, use normal handling for console mode
                // Get current cursor position
                uint16_t cursor_pos = get_mem_cursor();
                
                // Process arrow keys
                switch (scancode) {
                    case KEY_LEFT:
                        // Left arrow: move cursor left
                        if (cursor_pos > 0) {
                            // If at beginning of a line (except first line)
                            if (cursor_pos % 80 == 0 && cursor_pos >= 80) {
                                cursor_pos--; // Go to end of previous line
                            } else {
                                cursor_pos--; // Just go back one character
                            }
                        }
                        break;
                    case KEY_RIGHT:
                        // Right arrow: move cursor right
                        // If at end of a line (except last position)
                        if ((cursor_pos + 1) % 80 == 0 && cursor_pos < 80*25-1) {
                            cursor_pos++; // Go to beginning of next line
                        } else if (cursor_pos < 80*25-1) {
                            cursor_pos++; // Just advance one character
                        }
                        break;
                    case KEY_UP:
                        // Up arrow: move cursor up
                        if (cursor_pos >= 80) { // If not already on first line
                            // Keep same column when moving vertically
                            uint16_t column = cursor_pos % 80;
                            cursor_pos -= 80;
                            // Ensure we're at the same column
                            cursor_pos = (cursor_pos / 80) * 80 + column;
                        }
                        break;
                    case KEY_DOWN:
                        // Down arrow: move cursor down
                        if (cursor_pos < 80*24) { // If not already on last line
                            // Keep same column when moving vertically
                            uint16_t column = cursor_pos % 80;
                            cursor_pos += 80;
                            // Ensure we're at the same column
                            cursor_pos = (cursor_pos / 80) * 80 + column;
                        }
                        break;
                    case KEY_HOME:
                        // Home key: move cursor to beginning of current line
                        cursor_pos = (cursor_pos / 80) * 80;
                        break;
                    case KEY_END:
                        // End key: move cursor to end of current line
                        cursor_pos = (cursor_pos / 80) * 80 + 79;
                        break;
                }
                
                // Make sure cursor stays within screen boundaries
                check_screen(&cursor_pos);
                
                // Update cursor position
                set_mem_cursor(cursor_pos);
            }
            return;
        }
        
        // Get the character corresponding to the scancode
        char c = 0;
        static uint8_t last_scancode = 0;
        static uint32_t last_keypress_time = 0;
        
        // Get current time
        uint32_t current_time = get_time();
        
        // Basic key repeat rate limiting
        if (scancode == last_scancode && (current_time - last_keypress_time) < 50) {
            // Ignore repeated keypresses that are too close together
            return;
        }
        
        // Update last key press info
        last_scancode = scancode;
        last_keypress_time = current_time;
        
        // Local copy of scancode maps to ensure they're available
        extern const uint16_t scancode_map[];
        extern const uint16_t scancode_map_shift[];
        
        // Use the scancode_map and scancode_map_shift arrays
        if (scancode < 128 && scancode > 0) {
            uint16_t key;
            if (shift_pressed) {
                key = scancode_map_shift[scancode];
            } else {
                key = scancode_map[scancode];
            }
            
            // Convert the key value to a character if it's in the printable ASCII range or a valid control character
            if (key < 128 && ((key >= 32 && key < 127) || key == '\n' || key == '\r' || key == '\t' || key == '\b')) {
                c = (char)key;
            }
        }
        
        // If it's a valid character, process it
        if (c != 0) {
            // Add the character to the keyboard buffer for scanf/gets
            kb_buffer_push(c);
            
            // Let callbacks handle the character first
            notify_keyboard_callbacks(KB_NORMAL_KEY, c, scancode);
            
            // Display character if no callback consumed it
            if (c >= 32 || c == '\n' || c == '\r' || c == '\b') {
                // If in mini-shell mode, let mini-shell handle display
                if (!mini_shell_active) {
                    // Only do direct screen writing in normal mode
                    char buffer[2] = {c, '\0'};
                    console_putbytes(buffer, 1);
                } else {
                    // For mini-shell, don't need to do anything here as the callback will handle it
                    // But we need to ensure the hardware cursor is at the right position
                    // This tells the shell to update its cursor
                    notify_keyboard_callbacks(KB_NORMAL_KEY, 0, 0xFF);
                }
            }
        }
    }
}

// Function that can be called from timer interrupt to check for keyboard input
void check_keyboard_buffer(void) {
    // This function can be used to periodically check the keyboard buffer
    // without relying solely on keyboard interrupts
    // Currently empty as we use interrupt-driven keyboard input
}