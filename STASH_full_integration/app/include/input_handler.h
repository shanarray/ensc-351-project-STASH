#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

/* input_handler.h - debounces the 4 buttons and turns presses into timer
 * state changes.
 *
 *   STUDY  button -> timer_study()   (starts/restarts a study session)
 *   BREAK  button -> timer_break()   (starts/restarts a break)
 *   PAUSE  button -> timer_pause() if running, timer_resume() if paused
 *   RESET  button -> timer_reset()
 *
 * Call input_handler_update() often (every few ms) from the main loop. */

#define STUDY_MINUTES    1
#define BREAK_MINUTES    1
#define DEBOUNCE_MS     30   // a level must be steady this long to count 

void input_handler_update(void);
void input_handler_update_at(long long now_ms);

#endif
