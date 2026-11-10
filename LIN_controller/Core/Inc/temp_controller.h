#ifndef TEMP_CONTROLLER_H
#define TEMP_CONTROLLER_H

#include "main.h"
#include <stdbool.h>

typedef struct {
    bool     system_active;    // true als IN1, IN2 of IN3 hoog is
    float    setpoint_c;       // Gewenste temperatuur (40, 60 of 80 °C)
    uint8_t  max_duty_limit;   // 25%, 50%, 75% of 100% (o.b.v. IN4 & IN5)
    float    kp;               // Proportionele versterking (standaard 10.0 %/°C)
    uint8_t  calculated_duty;  // Berekende vermogensvraag naar PTC
    bool     overtemp_fault;   // true als PTC of NTC boven 90 °C komt
    bool     pump_post_run;    // true tijdens de 30s nalooptijd van de pomp
} Temp_Controller_t;

extern Temp_Controller_t ctrl;

void TempCtrl_Init(void);
void TempCtrl_Update(void);

#endif /* TEMP_CONTROLLER_H */
