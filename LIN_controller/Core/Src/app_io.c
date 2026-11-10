#include "app_io.h"

App_IO_t io = {0};
LED_Handle_t status_led = {StatusLED_GPIO_Port, StatusLED_Pin, LED_MODE_OFF, 0, false};
LED_Handle_t err_led    = {LED_ERR_GPIO_Port,   LED_ERR_Pin,   LED_MODE_OFF, 0, false};

void IO_Init(void) {
    io.pump_do = false;
    HAL_GPIO_WritePin(PumpDO_GPIO_Port, PumpDO_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(StatusLED_GPIO_Port, StatusLED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, GPIO_PIN_RESET);
}

/* Generieke non-blocking functie voor elke LED */
void LED_UpdateSingle(LED_Handle_t *led) {
    uint32_t now = HAL_GetTick();

    switch (led->mode) {
        case LED_MODE_OFF:
            led->state = false;
            break;

        case LED_MODE_ON:
            led->state = true;
            break;

        case LED_MODE_BLINK_SLOW: // 500ms op, 500ms neer
            if ((now - led->last_tick) >= 500) {
                led->last_tick = now;
                led->state = !led->state;
            }
            break;

        case LED_MODE_BLINK_FAST: // 100ms op, 100ms neer
            if ((now - led->last_tick) >= 100) {
                led->last_tick = now;
                led->state = !led->state;
            }
            break;

        case LED_MODE_HEARTBEAT: // 100ms aan, 900ms uit
            if (led->state && (now - led->last_tick) >= 100) {
                led->last_tick = now;
                led->state = false;
            } else if (!led->state && (now - led->last_tick) >= 900) {
                led->last_tick = now;
                led->state = true;
            }
            break;

        default:
            led->state = false;
            break;
    }

    HAL_GPIO_WritePin(led->port, led->pin, led->state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void IO_Update(void) {
    static uint32_t last_io_tick = 0;
    uint32_t now = HAL_GetTick();

    // Lees elke 10 ms de digitale ingangen en schrijf de pomp-uitgang
    if ((now - last_io_tick) >= 10) {
        last_io_tick = now;

        // Actief hoog inlezen
        io.in1 = (HAL_GPIO_ReadPin(IN1_GPIO_Port, IN1_Pin) == GPIO_PIN_SET);
        io.in2 = (HAL_GPIO_ReadPin(IN2_GPIO_Port, IN2_Pin) == GPIO_PIN_SET);
        io.in3 = (HAL_GPIO_ReadPin(IN3_GPIO_Port, IN3_Pin) == GPIO_PIN_SET);
        io.in4 = (HAL_GPIO_ReadPin(IN4_GPIO_Port, IN4_Pin) == GPIO_PIN_SET);
        io.in5 = (HAL_GPIO_ReadPin(IN5_GPIO_Port, IN5_Pin) == GPIO_PIN_SET);
        io.in6 = (HAL_GPIO_ReadPin(IN6_GPIO_Port, IN6_Pin) == GPIO_PIN_SET);

        // Stuur Pomp uitgang aan
        HAL_GPIO_WritePin(PumpDO_GPIO_Port, PumpDO_Pin, io.pump_do ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    // Update beide LEDs via dezelfde generieke functie
    LED_UpdateSingle(&status_led);
    LED_UpdateSingle(&err_led);
}
