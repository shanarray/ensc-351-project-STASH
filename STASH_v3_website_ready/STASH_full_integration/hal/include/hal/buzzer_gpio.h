#ifndef BUZZER_GPIO_H
#define BUZZER_GPIO_H

#include <stdbool.h>

int Buzzer_init(void);
void Buzzer_write(bool on);
void Buzzer_motion_alert_start(void);
void Buzzer_update(void);
void Buzzer_cleanup(void);

#endif
