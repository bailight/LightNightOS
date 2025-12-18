#include "shell.h"
#include "console.h"
#include "lib.h"
#include "../lib/stdint.h"

#define MAX_CMD_LEN     128
#define MAX_CMD_NAME    32
#define MAX_ARGS        16
#define MAX_CMD_HISTORY 50
#define PROMPT          "> "

#define KEY_UP      0x01
#define KEY_DOWN    0x02
#define KEY_LEFT    0x03
#define KEY_RIGHT   0x04
#define KEY_HOME    0x05
#define KEY_END     0x06
#define KEY_DEL     0x07

static char cmd_history[MAX_CMD_HISTORY][MAX_CMD_LEN];
static int history_count = 0;
static int history_current = -1;
static int history_saved_current = -1;

static char cmd_buffer[MAX_CMD_LEN];
static int cmd_pos = 0;
static int cursor_pos = 0;
static int shell_active = 0;
static char saved_cmd[MAX_CMD_LEN];
static int saved_cmd_pos = 0;

static void cmd_echo(int argc, char **argv);
static void cmd_help(int argc, char **argv);
static void cmd_clear(int argc, char **argv);

typedef struct {
    const char *name;
    const char *description;
    void (*func)(int argc, char **argv);
} Command;

static Command commands[] = {
    {"echo",  "Print arguments", cmd_echo},
    {"help",  "Show help", cmd_help},
    {"clear", "Clear screen", cmd_clear},
    {NULL, NULL, NULL}
};

static void cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        printk("%s", argv[i]);
        if (i < argc - 1) printk(" ");
    }
    printk("\n");
}

static void cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;
    
    printk("Available commands:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        printk("  %-10s - %s\n", commands[i].name, commands[i].description);
    }
}

static void cmd_clear(int argc, char **argv) {
    (void)argc; (void)argv;
    clear_screen();
}

static void history_add(const char *cmd) {
    if (cmd == NULL || cmd[0] == '\0') {
        return;
    }
    
    if (history_count > 0) {
        if (strcmp(cmd_history[history_count - 1], cmd) == 0) {
            return;
        }
    }
    
    if (history_count >= MAX_CMD_HISTORY) {
        for (int i = 0; i < MAX_CMD_HISTORY - 1; i++) {
            str_copy(cmd_history[i], cmd_history[i + 1]);
        }
        history_count = MAX_CMD_HISTORY - 1;
    }
    
    str_copy(cmd_history[history_count], (char *)cmd);
    history_count++;
}

static void history_show(int direction) {
    if (history_count == 0) return;
    
    if (history_current == -1 && direction == 1) {
        str_copy(saved_cmd, cmd_buffer);
        saved_cmd_pos = cmd_pos;
    }
    
    int new_index = history_current;
    
    if (direction == 1) {
        if (history_current < history_count - 1) {
            new_index = history_current + 1;
        }
    } else {
        if (history_current > -1) {
            new_index = history_current - 1;
        }
    }
    
    if (new_index < -1) new_index = -1;
    if (new_index >= history_count) new_index = history_count - 1;
    
    history_current = new_index;
    
    char *show_cmd = NULL;
    if (history_current >= 0 && history_current < history_count) {
        show_cmd = cmd_history[history_current];
    } else if (history_current == -1) {
        show_cmd = saved_cmd;
    }
    

    if (show_cmd != NULL) {
        int new_len = strlen(show_cmd);
        int old_len = cmd_pos;
        
        for (int i = 0; i < old_len; i++) {
            move_cursor_left();
        }
        
        str_copy(cmd_buffer, show_cmd);
        cmd_pos = new_len;
        cursor_pos = cmd_pos;
        
        printk("%s", cmd_buffer);
        
        if (new_len < old_len) {
            int diff = old_len - new_len;
            for (int i = 0; i < diff; i++) {
                print_char(' ');
            }
            for (int i = 0; i < diff; i++) {
                move_cursor_left();
            }
        }
    }
}

static void move_cursor_to(int new_pos) {
    if (new_pos < 0) new_pos = 0;
    if (new_pos > cmd_pos) new_pos = cmd_pos;
    
    int steps = new_pos - cursor_pos;
    
    if (steps > 0) {
        for (int i = 0; i < steps; i++) {
            move_cursor_right();
        }
    } else if (steps < 0) {
        for (int i = 0; i < -steps; i++) {
            move_cursor_left();
        }
    }
    
    cursor_pos = new_pos;
}

static void redraw_from_cursor(void) {
    int chars_to_redraw = cmd_pos - cursor_pos;
    
    if (cursor_pos < cmd_pos) {
        printk("%s", cmd_buffer + cursor_pos);
    }
    
    int old_length = cmd_pos + 1 - cursor_pos;
    if (chars_to_redraw < old_length) {
        int spaces_needed = old_length - chars_to_redraw;
        for (int i = 0; i < spaces_needed; i++) {
            print_char(' ');
        }
    }
    
    int total_printed = chars_to_redraw + (old_length - chars_to_redraw);
    for (int i = 0; i < total_printed; i++) {
        move_cursor_left();
    }
}

static int parse_arguments(char *cmdline, char **argv) {
    int argc = 0;
    int in_arg = 0;
    
    while (*cmdline && argc < MAX_ARGS - 1) {
        if (*cmdline == ' ' || *cmdline == '\t') {
            if (in_arg) {
                *cmdline = '\0';
                in_arg = 0;
            }
        } else {
            if (!in_arg) {
                argv[argc++] = cmdline;
                in_arg = 1;
            }
        }
        cmdline++;
    }
    
    argv[argc] = NULL;
    return argc;
}

static void execute_command(void) {
    if (cmd_pos == 0) return;
    
    cmd_buffer[cmd_pos] = '\0';

    history_add(cmd_buffer);
    
    char *argv[MAX_ARGS];
    int argc = parse_arguments(cmd_buffer, argv);
    
    if (argc == 0) return;
    
    int found = 0;
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].func(argc, argv);
            found = 1;
            break;
        }
    }
    
    if (!found) {
        printk("Unknown command: '%s'\n", argv[0]);
        printk("Type 'help' for available commands\n");
    }
    
    printk(PROMPT);
}

void shell_process_input(char c) {
    if (!shell_active) return;
    
    if (c >= KEY_UP && c <= KEY_DEL) {
        switch (c) {
            case KEY_LEFT:
                if (cursor_pos > 0) {
                    cursor_pos--;
                    move_cursor_left();
                }
                return;
                
            case KEY_RIGHT:
                if (cursor_pos < cmd_pos) {
                    cursor_pos++;
                    move_cursor_right();
                }
                return;
                
            case KEY_UP:
                if (history_count > 0) {
                    history_show(1);
                }
                return;
                
            case KEY_DOWN:
                if (history_count > 0) {
                    history_show(-1);
                }
                return;
                
            case KEY_HOME:
                move_cursor_to(0);
                return;
                
            case KEY_END:
                move_cursor_to(cmd_pos);
                return;
                
            case KEY_DEL:
                if (cursor_pos < cmd_pos) {
                    for (int i = cursor_pos; i < cmd_pos - 1; i++) {
                        cmd_buffer[i] = cmd_buffer[i + 1];
                    }
                    cmd_pos--;
                    cmd_buffer[cmd_pos] = '\0';
                    redraw_from_cursor();
                }
                return;
        }
    }
    
    switch (c) {
        case '\n':
            if (cmd_pos > 0) {
                cmd_buffer[cmd_pos] = '\0';
                printk("\n");
                execute_command();
                cmd_pos = 0;
                cursor_pos = 0;
            } else {
                printk("\n");
                printk(PROMPT);
            }
            break;
            
        case '\b':
            if (cursor_pos > 0) {
                cursor_pos--;
                cmd_pos--;
                
                for (int i = cursor_pos; i < cmd_pos; i++) {
                    cmd_buffer[i] = cmd_buffer[i + 1];
                }
                cmd_buffer[cmd_pos] = '\0';
                
                move_cursor_left();
                redraw_from_cursor();
            }
            break;
            
        default:
            if (cmd_pos < MAX_CMD_LEN - 1 && c >= 32 && c <= 126) {
                if (cursor_pos < cmd_pos) {
                    for (int i = cmd_pos; i > cursor_pos; i--) {
                        cmd_buffer[i] = cmd_buffer[i - 1];
                    }
                }
                
                cmd_buffer[cursor_pos] = c;
                cmd_pos++;
                cmd_buffer[cmd_pos] = '\0';
                cursor_pos++;
                
                print_char(c);
                
                if (cursor_pos < cmd_pos) {
                    redraw_from_cursor();
                }
            }
            break;
    }
}

void shell_init(void) {
    clear_screen();

    printk("=== LightNightOS Shell (with cursor movement) ===\n");
    printk("Type 'help' for available commands\n");
    printk("=================================================\n\n");
    
    shell_active = 1;
    cmd_pos = 0;
    cursor_pos = 0;
    cmd_buffer[0] = '\0';
    saved_cmd[0] = '\0';
    saved_cmd_pos = 0;
    history_count = 0;
    history_current = -1;
    history_saved_current = -1;
    
    printk(PROMPT);
    update_cursor(get_cursor_row(), get_cursor_col());
}

void shell_stop(void) {

    shell_active = 0;
}

int shell_is_active(void) {
    return shell_active;
}