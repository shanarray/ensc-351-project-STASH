// hal/include/hal/adc.h
#ifndef ADC_H
#define ADC_H

#include <stdbool.h>

void Adc_init(void);
void Adc_cleanup(void);
int Adc_readChannel(int channel);

#endif