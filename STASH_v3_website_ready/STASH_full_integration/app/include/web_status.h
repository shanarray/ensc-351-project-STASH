#ifndef STASH_WEB_STATUS_H
#define STASH_WEB_STATUS_H
#include "timer.h"

void WebStatus_publish(TimerState state, int hours, int minutes, int seconds);
void WebStatus_log_session(const char *mode, int duration_seconds);

#endif
