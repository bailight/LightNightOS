#ifndef SHELL_H
#define SHELL_H

#define SHELL_KEY_UP    0x4800
#define SHELL_KEY_DOWN  0x5000
#define SHELL_KEY_LEFT  0x4B00
#define SHELL_KEY_RIGHT 0x4D00

#define SHELL_KEY_HOME   0x4700
#define SHELL_KEY_END    0x4F00
#define SHELL_KEY_DEL    0x5300

void shell_init(void);
void shell_stop(void);
int shell_is_active(void);

void shell_process_input(char c);

#endif