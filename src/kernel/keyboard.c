#include "keyboard.h"
#include "console.h"
#include "kernel.h"
#include "../lib/stdint.h"

#define PS2_DATA    0x60
#define PS2_STATUS  0x64
#define PIC_MASTER_CMD  0x20

static char scancode_to_char(uint8_t scancode) {
    static char map[] = {
        0,  27,'1','2','3','4','5','6','7','8','9','0','-','=', '\b',
        '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
        'a','s','d','f','g','h','j','k','l',';','\'','`',   0,'\\',
        'z','x','c','v','b','n','m',',','.','/',           0,   0,   0,' '
    };
    if (scancode < sizeof(map)) {
        return map[scancode];
    }
    return 0;
}

static void kbd_flush(void) {
    for (int i = 0; i < 256; ++i) {
        uint8_t status = inb(PS2_STATUS);
        if (status & 0x01) {
            (void)inb(PS2_DATA);
        } else {
            break;
        }
    }
}

void keyboard_init(void) {
    kbd_flush();
}

void keyboard_handler_c(void) {
    while (inb(PS2_STATUS) & 0x01) {
        uint8_t scancode = inb(PS2_DATA);

        if (scancode & 0x80) {
            continue;
        }

        // Process function keys F1~F4
        if (scancode >= 0x3B && scancode <= 0x3E) {
            vt_switch(scancode - 0x3B);
            continue;
        }

        // Handle the backspace key
        if (scancode == 0x0E) {
            uint8_t col = get_cursor_col();
            if (col > 0) {
                set_cursor_col(col - 1);
                print_char(' ');
                set_cursor_col(col - 1);
            }
        } else {
            // Scan code to character and print
            char c = scancode_to_char(scancode);
            if (c != 0) {
                print_char(c);
            }
        }
    }

    // Update cursor position
    update_cursor(get_cursor_row(), get_cursor_col());
    outb(PIC_MASTER_CMD, 0x20);
}