#ifndef BUZZER_GPIO_H
#define BUZZER_GPIO_H
#include <stdbool.h>
int Buzzer_init(void);
void Buzzer_write(bool on);

// added for buzzer beep
void Buzzer_beep(int frequency, int duration_ms);

void Buzzer_cleanup(void);

#endif