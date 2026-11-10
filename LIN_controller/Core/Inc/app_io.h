#ifndef APP_IO_H
#define APP_IO_H

#include "main.h"
#include <stdbool.h>

/* LED Knipper Modes */
typedef enum {
    LED_MODE_OFF       = 0, // Continu uit
    LED_MODE_BLINK_SLOW= 1, // 1 Hz (500ms aan, 500ms uit)
    LED_MODE_BLINK_FAST= 2, // 5 Hz (100ms aan, 100ms uit)
    LED_MODE_ON        = 3, // Continu aan
    LED_MODE_HEARTBEAT = 4  // Kort flitsen (100ms aan, 900ms uit)
} LED_Mode_t;

/* Generieke LED Struct (herbruikbaar voor elke LED) */
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint8_t       mode;      // In te stellen via LED_Mode_t (0, 1, 2, 3, 4)
    uint32_t      last_tick;
    bool          state;
} LED_Handle_t;

/* Globale I/O Status Struct */
typedef struct {
    bool in1; // PA1 - Setpoint 40C
    bool in2; // PA2 - Setpoint 60C
    bool in3; // PA3 - Setpoint 80C
    bool in4; // PA4 - Max Power bit 0
    bool in5; // PA5 - Max Power bit 1
    bool in6; // PA6 - Reserve

    bool pump_do; // PB12 - Pomp uitgang
} App_IO_t;

extern App_IO_t     io;
extern LED_Handle_t status_led;
extern LED_Handle_t err_led;

void IO_Init(void);
void IO_Update(void);
void LED_UpdateSingle(LED_Handle_t *led);

#endif /* APP_IO_H */
