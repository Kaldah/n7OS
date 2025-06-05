#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <inttypes.h>

// Keyboard ports
#define KEYB_ENCODER        0x60 
#define KEYB_CONTROLLER     0x64
#define KEYB_ENC_CMD_REG    0x60
#define KEYB_CTRL_CMD_REG   0x64

// Keyboard controller commands
#define KEYB_CTRL_OUT_BUF   0x1
#define KEYB_CTRL_IN_BUF    0x2
#define KEYB_CTRL_SYSTEM    0x4
#define KEYB_CTRL_CMD_DATA  0x8
#define KEYB_CTRL_LOCKED    0x10
#define KEYB_CTRL_AUX_BUF   0x20
#define KEYB_CTRL_TIMEOUT   0x40
#define KEYB_CTRL_PARITY    0x80

// Keyboard commands
#define KEYB_ENC_SET_LED    0xED
#define KEYB_ECHO           0xEE
#define KEYB_SET_ALT_SCODE  0xF0
#define KEYB_SET_AUTOREPEAT 0xF3
#define KEYB_ENABLE         0xF4

// LEDs 
#define KEY_LOCK_OFF    0x0
#define NUM_LOCK        0x2
#define CAPS_LOCK       0x1
#define SCROLL_LOCK     0x4

// Keyboard IRQ number and PIC IRQ line
#define KEYB_IRQ 		0x21
#define KEYB_PIC_IRQ 	0x1

// Special keys pressed and released
#define SHIFT_PRESSED   0x2A
#define SHIFT_RELEASED  0xAA
#define CTRL_PRESSED    0x1D
#define CTRL_RELEASED   0x9D
#define ALT_PRESSED     0x38
#define ALT_RELEASED    0xB8

#define IS_KEY_RELEASED(key) (key & 0x80)

enum KEYCODE {
// Alphanumeric keys ////////////////
	KEY_SPACE             = ' ',
	KEY_0                 = '0',
	KEY_1                 = '1',
	KEY_2                 = '2',
	KEY_3                 = '3',
	KEY_4                 = '4',
	KEY_5                 = '5',
	KEY_6                 = '6',
	KEY_7                 = '7',
	KEY_8                 = '8',
	KEY_9                 = '9',
	KEY_a                 = 'a',
	KEY_b                 = 'b',
	KEY_c                 = 'c',
	KEY_d                 = 'd',
	KEY_e                 = 'e',
	KEY_f                 = 'f',
	KEY_g                 = 'g',
	KEY_h                 = 'h',
	KEY_i                 = 'i',
	KEY_j                 = 'j',
	KEY_k                 = 'k',
	KEY_l                 = 'l',
	KEY_m                 = 'm',
	KEY_n                 = 'n',
	KEY_o                 = 'o',
	KEY_p                 = 'p',
	KEY_q                 = 'q',
	KEY_r                 = 'r',
	KEY_s                 = 's',
	KEY_t                 = 't',
	KEY_u                 = 'u',
	KEY_v                 = 'v',
	KEY_w                 = 'w',
	KEY_x                 = 'x',
	KEY_y                 = 'y',
	KEY_z                 = 'z',
	KEY_A                 = 'A',
	KEY_B                 = 'B',
	KEY_C                 = 'C',
	KEY_D                 = 'D',
	KEY_E                 = 'E',
	KEY_F                 = 'F',
	KEY_G                 = 'G',
	KEY_H                 = 'H',
	KEY_I                 = 'I',
	KEY_J                 = 'J',
	KEY_K                 = 'K',
	KEY_L                 = 'L',
	KEY_M                 = 'M',
	KEY_N                 = 'N',
	KEY_O                 = 'O',
	KEY_P                 = 'P',
	KEY_Q                 = 'Q',
	KEY_R                 = 'R',
	KEY_S                 = 'S',
	KEY_T                 = 'T',
	KEY_U                 = 'U',
	KEY_V                 = 'V',
	KEY_W                 = 'W',
	KEY_X                 = 'X',
	KEY_Y                 = 'Y',
	KEY_Z                 = 'Z',

	KEY_RETURN            = '\r',
	KEY_ESCAPE            = 0x1001,
	KEY_BACKSPACE         = '\b',

// Arrow keys ////////////////////////

	KEY_UP                = 0x1100,
	KEY_DOWN              = 0x1101,
	KEY_LEFT              = 0x1102,
	KEY_RIGHT             = 0x1103,

// Function keys /////////////////////

	KEY_F1                = 0x1201,
	KEY_F2                = 0x1202,
	KEY_F3                = 0x1203,
	KEY_F4                = 0x1204,
	KEY_F5                = 0x1205,
	KEY_F6                = 0x1206,
	KEY_F7                = 0x1207,
	KEY_F8                = 0x1208,
	KEY_F9                = 0x1209,
	KEY_F10               = 0x120a,
	KEY_F11               = 0x120b,
	KEY_F12               = 0x120b,
	KEY_F13               = 0x120c,
	KEY_F14               = 0x120d,
	KEY_F15               = 0x120e,

	KEY_DOT               = '.',
	KEY_COMMA             = ',',
	KEY_COLON             = ':',
	KEY_SEMICOLON         = ';',
	KEY_SLASH             = '/',
	KEY_BACKSLASH         = '\\',
	KEY_PLUS              = '+',
	KEY_MINUS             = '-',
	KEY_ASTERISK          = '*',
	KEY_EXCLAMATION       = '!',
	KEY_QUESTION          = '?',
	KEY_QUOTEDOUBLE       = '\"',
	KEY_QUOTE             = '\'',
	KEY_EQUAL             = '=',
	KEY_HASH              = '#',
	KEY_PERCENT           = '%',
	KEY_AMPERSAND         = '&',
	KEY_UNDERSCORE        = '_',
	KEY_LEFTPARENTHESIS   = '(',
	KEY_RIGHTPARENTHESIS  = ')',
	KEY_LEFTBRACKET       = '[',
	KEY_RIGHTBRACKET      = ']',
	KEY_LEFTCURL          = '{',
	KEY_RIGHTCURL         = '}',
	KEY_DOLLAR            = '$',
	KEY_POUND             = '£',
	KEY_EURO              = '$',
	KEY_LESS              = '<',
	KEY_GREATER           = '>',
	KEY_BAR               = '|',
	KEY_GRAVE             = '`',
	KEY_TILDE             = '~',
	KEY_AT                = '@',
	KEY_CARRET            = '^',

// Numeric keypad //////////////////////

	KEY_KP_0              = '0',
	KEY_KP_1              = '1',
	KEY_KP_2              = '2',
	KEY_KP_3              = '3',
	KEY_KP_4              = '4',
	KEY_KP_5              = '5',
	KEY_KP_6              = '6',
	KEY_KP_7              = '7',
	KEY_KP_8              = '8',
	KEY_KP_9              = '9',
	KEY_KP_PLUS           = '+',
	KEY_KP_MINUS          = '-',
	KEY_KP_DECIMAL        = '.',
	KEY_KP_DIVIDE         = '/',
	KEY_KP_ASTERISK       = '*',
	KEY_KP_NUMLOCK        = 0x300f,
	KEY_KP_ENTER          = 0x3010,

	KEY_TAB               = 0x4000,
	KEY_CAPSLOCK          = 0x4001,

// Modify keys ////////////////////////////

	KEY_LSHIFT            = 0x4002,
	KEY_LCTRL             = 0x4003,
	KEY_LALT              = 0x4004,
	KEY_LWIN              = 0x4005,
	KEY_RSHIFT            = 0x4006,
	KEY_RCTRL             = 0x4007,
	KEY_RALT              = 0x4008,
	KEY_RWIN              = 0x4009,

	KEY_INSERT            = 0x400a,
	KEY_DELETE            = 0x400b,
	KEY_HOME              = 0x400c,
	KEY_END               = 0x400d,
	KEY_PAGEUP            = 0x400e,
	KEY_PAGEDOWN          = 0x400f,
	KEY_SCROLLLOCK        = 0x4010,
	KEY_PAUSE             = 0x4011,

	KEY_UNKNOWN,
	KEY_NUMKEYCODES
};

// Keep for reference but mark as unused to prevent warnings
__attribute__((unused)) static const uint16_t scancode_map[] = {
	0, KEY_ESCAPE, KEY_AMPERSAND, KEY_e, KEY_QUOTEDOUBLE, KEY_QUOTE, KEY_LEFTPARENTHESIS, KEY_MINUS, KEY_e, KEY_UNDERSCORE, KEY_c, KEY_a, KEY_RIGHTPARENTHESIS, KEY_EQUAL, KEY_BACKSPACE, // first line
	KEY_TAB, KEY_a, KEY_z, KEY_e, KEY_r, KEY_t, KEY_y, KEY_u, KEY_i, KEY_o, KEY_p, KEY_CARRET, KEY_DOLLAR, KEY_RETURN, // second line
	0, // left control, 
	KEY_q, KEY_s, KEY_d, KEY_f, KEY_g, KEY_h, KEY_j, KEY_k, KEY_l, KEY_m, KEY_u, KEY_2, // third line
	0, // left shift
	KEY_ASTERISK, KEY_w, KEY_x, KEY_c, KEY_v, KEY_b, KEY_n, KEY_COMMA, KEY_SEMICOLON, KEY_COLON, KEY_EXCLAMATION, 0, // fourth line
	KEY_ASTERISK, 0, // Alt
	KEY_SPACE, 0, // Maj
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1 à F10
	0, // Numlock
	0, // Scrolllock
	0, // Home
	0, // Up
	0, // Page up
	KEY_MINUS,
	0, // Left
	0, 
	0, // Right
	KEY_PLUS,
	0, // End
	0, // Down
	0, // Page down
	0, // Insert
	0, // Delete
	0, // Snapshot
	0,  
	KEY_LESS,
	0, 0, // F11, F12
}; 

// scancode_map_shift is the same as scancode_map but with shift pressed
// Keep for reference but mark as unused to prevent warnings
__attribute__((unused)) static const uint16_t scancode_map_shift[] = {
	0, KEY_ESCAPE, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, 0, // degre
	KEY_PLUS, KEY_BACKSPACE, // first line
	KEY_TAB, KEY_A, KEY_Z, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_COMMA, KEY_POUND, KEY_RETURN, // second line
	0, // left control, 
	KEY_Q, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_M, KEY_PERCENT, 0, // third line
	0, // left shift
	KEY_u, KEY_W, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_QUESTION, KEY_DOT, KEY_SLASH, KEY_EXCLAMATION, 0, // fourth line
	KEY_ASTERISK, 0, // Alt
	KEY_SPACE, 0, // Maj
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1 à F10
	0, // Numlock
	0, // Scrolllock
	0, // Home
	0, // Up
	0, // Page up
	KEY_MINUS,
	0, // Left
	0, 
	0, // Right
	KEY_PLUS,
	0, // End
	0, // Down
	0, // Page down
	0, // Insert
	0, // Delete
	0, // Snapshot
	0,  
	KEY_LESS,
	0, 0, // F11, F12
}; 

// Function prototypes
void init_keyboard(void);
char kgetch(void);
void check_keyboard_buffer(void);  // New function that can be called from the timer interrupt

// Keyboard interrupt handler function
void handler_en_C_KEYBOARD(void);

#endif