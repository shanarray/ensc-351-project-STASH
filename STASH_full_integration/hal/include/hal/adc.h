
#ifndef HAL_ADC_H
#define HAL_ADC_H

/* MCP3208: hardware channels CH0 through CH7. */
int Adc_init(void);
void Adc_cleanup(void);
int Adc_readChannel(int channel);

#endif


