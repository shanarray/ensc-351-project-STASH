#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
gcc -std=c11 -Wall -Wextra -Werror -Wpedantic \
    -Itests/include -Iapp/include -Ihal/include -Ihal/include/hal \
    app/src/feedback.c tests/mock_led.c tests/test_feedback.c \
    -o /tmp/stash_feedback_test
/tmp/stash_feedback_test
