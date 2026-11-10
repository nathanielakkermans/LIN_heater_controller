#include "ntc_sensor.h"
#include <math.h>

#define NTC_PULLUP_R    10000.0f // 10k weerstand aan 3.3V
#define NTC_NOMINAL_R   10000.0f // 10k NTC bij 25 C
#define NTC_NOMINAL_T   298.15f  // 25.0 C in Kelvin (25 + 273.15)
#define NTC_BETA        3950.0f  // Beta-coëfficiënt

NTC_Sensor_t ntc = {0};
static ADC_HandleTypeDef *ntc_hadc;
static bool filter_initialized = false;

void NTC_Init(ADC_HandleTypeDef *hadc) {
    ntc_hadc = hadc;
    // Verplichte zelf-kalibratie voor de STM32F103 ADC!
    HAL_ADCEx_Calibration_Start(ntc_hadc);
}

void NTC_Update(void) {
    static uint32_t last_ntc_tick = 0;
    uint32_t now = HAL_GetTick();

    // Bemonster elke 50 ms (20 Hz)
    if ((now - last_ntc_tick) >= 50) {
        last_ntc_tick = now;

        HAL_ADC_Start(ntc_hadc);
        if (HAL_ADC_PollForConversion(ntc_hadc, 5) == HAL_OK) {
            ntc.raw_adc = (uint16_t)HAL_ADC_GetValue(ntc_hadc);

            // Detecteer kortsluiting naar GND (< 20) of losse sensor (> 4075)
            if (ntc.raw_adc < 20 || ntc.raw_adc > 4075) {
                ntc.fault = true;
                return;
            }
            ntc.fault = false;

            // Exponential Moving Average (EMA) filter voor rustige uitlezing
            if (!filter_initialized) {
                ntc.filtered_adc = (float)ntc.raw_adc;
                filter_initialized = true;
            } else {
                ntc.filtered_adc = (0.15f * (float)ntc.raw_adc) + (0.85f * ntc.filtered_adc);
            }

            // Spanningsdeler: 3.3V -> 10k -> ADC -> NTC -> GND
            ntc.resistance_ohm = NTC_PULLUP_R * (ntc.filtered_adc / (4095.0f - ntc.filtered_adc));

            // Steinhart-Hart Beta formule: 1/T = 1/T0 + (1/B) * ln(R/R0)
            float inv_t = (1.0f / NTC_NOMINAL_T) + (logf(ntc.resistance_ohm / NTC_NOMINAL_R) / NTC_BETA);
            ntc.temp_c = (1.0f / inv_t) - 273.15f;
        }
    }
}
