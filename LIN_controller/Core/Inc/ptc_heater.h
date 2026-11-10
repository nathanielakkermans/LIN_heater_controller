#ifndef PTC_HEATER_H
#define PTC_HEATER_H

#include "main.h"
#include <stdbool.h>

typedef enum {
    LIN_STATE_WAIT_SYNC = 0,
    LIN_STATE_WAIT_PID,
    LIN_STATE_READ_DATA
} LIN_RxState_t;

typedef struct {
    // --- Verzenden (ID 0x04) ---
    bool     enable;
    uint8_t  power_pct;

    // --- Ontvangen (ID 0x25) ---
    int8_t   temp_c;
    float    current_a;
    float    hv_voltage;

    // --- Diagnose & Buffers ---
    uint8_t  rx_buffer[9];   // We slaan nu alleen de 8 databytes + 1 checksum byte op
    bool     data_valid;
    bool     comm_error;
    uint16_t error_count;
    uint32_t last_rx_tick;
} PTC_Heater_t;

extern PTC_Heater_t ptc;

void PTC_Init(UART_HandleTypeDef *huart);
void PTC_Update(void);
void PTC_ProcessByte(uint8_t byte);

#endif /* PTC_HEATER_H */
