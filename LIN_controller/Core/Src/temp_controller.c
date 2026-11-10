#include "temp_controller.h"
#include "app_io.h"
#include "ntc_sensor.h"
#include "ptc_heater.h"

#define PUMP_POST_RUN_MS   30000 // 30 seconden nalooptijd pomp
#define MIN_EFFECTIVE_DUTY 15    // Onder 15% gaan de IGBT's in de kachel uit
#define OVERTEMP_LIMIT_C   92.0f // Harde veiligheidslimiet

Temp_Controller_t ctrl = {0};
static uint32_t heater_off_tick = 0;
static bool was_active = false;

void TempCtrl_Init(void) {
    ctrl.kp = 10.0f; // 10% vermogen per graad afwijking (dus < 10°C onder setpoint begint terugregelen)
    ctrl.setpoint_c = 0.0f;
    ctrl.max_duty_limit = 25;
}

void TempCtrl_Update(void) {
    static uint32_t last_ctrl_tick = 0;
    uint32_t now = HAL_GetTick();

    if ((now - last_ctrl_tick) < 100) {
        return; // Draai de regellus op 10 Hz (elke 100 ms)
    }
    last_ctrl_tick = now;

    // 1. Bepaal Setpoint o.b.v. IN1..IN3 (Hoogste krijgt voorrang)
    if (io.in3) {
        ctrl.setpoint_c = 80.0f;
        ctrl.system_active = true;
    } else if (io.in2) {
        ctrl.setpoint_c = 60.0f;
        ctrl.system_active = true;
    } else if (io.in1) {
        ctrl.setpoint_c = 40.0f;
        ctrl.system_active = true;
    } else {
        ctrl.setpoint_c = 0.0f;
        ctrl.system_active = false;
    }

    // 2. Bepaal Maximaal Vermogen o.b.v. IN4 & IN5
    if (!io.in4 && !io.in5)      ctrl.max_duty_limit = 25;
    else if (io.in4 && !io.in5)  ctrl.max_duty_limit = 50;
    else if (!io.in4 && io.in5)  ctrl.max_duty_limit = 75;
    else                         ctrl.max_duty_limit = 100;

    // 3. Controleer op Overtemperatuur of Sensor/LIN fouten
    if (ntc.temp_c > OVERTEMP_LIMIT_C ){//|| (ptc.data_valid && ptc.temp_c > 92)) {
        ctrl.overtemp_fault = true;
    } else if (ntc.temp_c < (OVERTEMP_LIMIT_C - 5.0f) && ptc.temp_c < 85) {
        ctrl.overtemp_fault = false;
    }

    bool any_fault = (ntc.fault  || ctrl.overtemp_fault );//|| ptc.comm_error);

    // 4. Pomp Aansturing (inclusief 30 seconden nalooptijd)
    if (ctrl.system_active) {
        io.pump_do = true;
        ctrl.pump_post_run = false;
        was_active = true;
    } else {
        if (was_active) {
            heater_off_tick = now;
            was_active = false;
            ctrl.pump_post_run = true;
        }
        if (ctrl.pump_post_run) {
            if ((now - heater_off_tick) < PUMP_POST_RUN_MS) {
                io.pump_do = true;
            } else {
                io.pump_do = false;
                ctrl.pump_post_run = false;
            }
        } else {
            io.pump_do = false;
        }
    }

    // 5. Zuivere P-Regelaar Berekening (op basis van de NTC temperatuur)
    // Opstart-veiligheid: wacht de eerste 2 seconden met vermogen vragen zodat de PTC netjes in Idle ontwaakt
    if (!ctrl.system_active || any_fault || now < 2000) {
        ctrl.calculated_duty = 0;
        ptc.enable = false;
        ptc.power_pct = 0;
    } else {
        float error = ctrl.setpoint_c - ntc.temp_c;

        if (error <= 0.0f) {
            ctrl.calculated_duty = 0;
        } else {
            float raw_p = error * ctrl.kp;
            if (raw_p > (float)ctrl.max_duty_limit) {
                raw_p = (float)ctrl.max_duty_limit;
            }

            // Kleine hysterese op de ondergrens zodat hij niet klappert rond 15%
            if (ptc.enable && raw_p < (float)MIN_EFFECTIVE_DUTY) {
                ctrl.calculated_duty = 0;
            } else if (!ptc.enable && raw_p < (float)(MIN_EFFECTIVE_DUTY + 5)) {
                ctrl.calculated_duty = 0;
            } else {
                ctrl.calculated_duty = (uint8_t)raw_p;
            }
        }

        if (ctrl.calculated_duty > 0) {
            ptc.enable = true;
            ptc.power_pct = ctrl.calculated_duty;
        } else {
            ptc.enable = false;
            ptc.power_pct = 0;
        }
    }

    // 6. Koppel Status en Fouten aan de LEDs
    // Error LED: Snel knipperen bij NTC/Overtemp fout, Langzaam bij LIN timeout
    if (ntc.fault || ctrl.overtemp_fault) {
        err_led.mode = LED_MODE_BLINK_FAST;
    } else if (ptc.comm_error) {
        err_led.mode = LED_MODE_BLINK_SLOW;
    } else {
        err_led.mode = LED_MODE_OFF;
    }

    // Status LED: Uit = Systeem uit, Hartslag = Pomp naloop, Langzaam = Verwarmen, Continu Aan = Op temperatuur
    if (!ctrl.system_active) {
        status_led.mode = ctrl.pump_post_run ? LED_MODE_HEARTBEAT : LED_MODE_OFF;
    } else if (ptc.enable && ptc.power_pct > 0) {
        status_led.mode = LED_MODE_BLINK_SLOW;
    } else {
        status_led.mode = LED_MODE_ON; // Setpoint bereikt!
    }
}
