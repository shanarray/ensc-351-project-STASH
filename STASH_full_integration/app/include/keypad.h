#ifndef KEYPAD_H
#define KEYPAD_H
int keypad_init(void);
void keypad_cleanup(void);
char keypad_scan(void);
char keypad_get_key(void);
int keypad_enter_minutes(const char *prompt);
#endif
