#!/bin/sh
set -eu

cd "$(dirname "$0")/../.."

gcc -std=c11 -D_POSIX_C_SOURCE=200809L \
    -Wall -Wextra -Werror -Wpedantic \
    -Iapp/include -Ihal/include -Ihal/include/hal \
    app/src/feedback.c \
    hal/src/timer.c \
    tests/timer_led_demo/led_hardware_adapter.c \
    tests/timer_led_demo/timer_led_demo.c \
    -lgpiod -o tests/timer_led_demo/timer_led_demo

./tests/timer_led_demo/timer_led_demo
