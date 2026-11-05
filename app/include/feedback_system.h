#ifndef FEEDBACK_SYSTEM_H
#define FEEDBACK_SYSTEM_H

#include "feedback.h"

// Call after the button GPIO has been initialized.
void FeedbackSystem_init(FeedbackBuzzerWrite buzzer_write);

// Call after input_handler_update() on every loop.
void FeedbackSystem_update(void);

void FeedbackSystem_cleanup(void);

#endif
