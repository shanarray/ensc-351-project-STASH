#include "buttons.h"
#include "adc.h"
int buttons_read(ButtonSample *sample) {
    if (!sample) return -1;
    int group = Adc_readChannel(2);
    int power = Adc_readChannel(3);
    if (group < 0 || power < 0) return -1;
    Button b = BTN_NONE;
    if (group >= 0 && group < 500) b = BTN_STUDY;
    else if (group >= 1700 && group <= 2400) b = BTN_BREAK;
    else if (group >= 2500 && group <= 3000) b = BTN_PAUSE;
    sample->group = b;
    sample->power = power < 1000;
    return 0;
}
