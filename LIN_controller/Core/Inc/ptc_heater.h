#ifndef PTC_HEATER_H
#define PTC_HEATER_H

#include "main.h"
#include <stdbool.h>

typedef struct {
    // --- Verzenden (ID 0x04) ---
    bool     enable;         // false = Standby (0x80), true = Vermogen vragen
    uint8_t  power_pct;      // 0 t/m 100 %

    // --- Ontvangen (ID 0x25) ---
    int8_t   temp_c;         // Interne koelwatertemperatuur van de kachel (°C)
    float    current_a;      // Stroom in Ampère (0.1 A per bit)
    float    hv_voltage;     // Hoogspanning in Volt

    // --- Diagnose & Buffers ---
    uint8_t  rx_buffer_raw[11];
    uint8_t  rx_buffer[9];
    bool     data_valid;     // Wordt true zodra er 1 geldig pakket is ontvangen
    bool     comm_error;     // true als de kachel > 1 seconde niet antwoordt
    uint16_t error_count;    // Checksum foutenteller
    uint32_t last_rx_tick;   // Tijdstip van laatst geslaagde LIN-ontvangst
} PTC_Heater_t;

extern PTC_Heater_t ptc;

void PTC_Init(UART_HandleTypeDef *huart);
void PTC_Update(void);
void PTC_RxCallback(UART_HandleTypeDef *huart);

#endif /* PTC_HEATER_H */
