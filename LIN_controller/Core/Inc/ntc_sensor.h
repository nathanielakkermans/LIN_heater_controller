#ifndef NTC_SENSOR_H
#define NTC_SENSOR_H

#include "main.h"
#include <stdbool.h>

typedef struct {
    uint16_t raw_adc;       // Laatste ruwe 12-bit ADC waarde (0 - 4095)
    float    filtered_adc;  // Gevlakte ADC waarde
    float    resistance_ohm;// Berekende NTC weerstand in Ohm
    float    temp_c;        // Berekende temperatuur in graden Celsius
    bool     fault;         // true bij kabelbreuk of kortsluiting
} NTC_Sensor_t;

extern NTC_Sensor_t ntc;

void NTC_Init(ADC_HandleTypeDef *hadc);
void NTC_Update(void);

#endif /* NTC_SENSOR_H */
