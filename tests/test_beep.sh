#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
cat > /tmp/stash_beep.c <<'EOF'
#define _POSIX_C_SOURCE 200809L
#include "hal/buzzer_gpio.h"
#include <time.h>
#include <stdio.h>
int main(void) {
    if (Buzzer_init() != 0) return 1;
    Buzzer_write(true);
    struct timespec delay = {0, 250000000};
    nanosleep(&delay, NULL);
    Buzzer_write(false);
    Buzzer_cleanup();
    puts("Beep test finished");
    return 0;
}
EOF
gcc -std=c11 -Wall -Wextra -Werror -Wpedantic -Ihal/include \
  hal/src/buzzer_gpio.c /tmp/stash_beep.c -lgpiod -o /tmp/stash_beep
/tmp/stash_beep
