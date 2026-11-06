#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
gcc -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -Wpedantic \
  -Itests/fixtures -Ihal/include -Itests/fixtures/hal \
  tests/fixtures/feedback.c tests/fixtures/feedback_system.c \
  hal/src/buzzer_gpio.c tests/fixtures/mock_led.c \
  tests/fixtures/timer.c tests/fixtures/main.c \
  -lgpiod -o /tmp/stash_buzzer_demo
.//tmp/stash_buzzer_demo
