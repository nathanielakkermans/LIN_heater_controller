#include "ptc_heater.h"

PTC_Heater_t ptc = {0};
static UART_HandleTypeDef *ptc_huart;

// State Machine variabelen
static LIN_RxState_t rx_state = LIN_STATE_WAIT_SYNC;
static uint8_t       rx_index = 0;
static uint8_t       expected_pid_rx = 0;

static uint8_t LIN_CalcChecksum(uint8_t pid, uint8_t *data, uint8_t length) {
    uint16_t sum = pid;
    for (uint8_t i = 0; i < length; i++) {
        sum += data[i];
        if (sum > 0xFF) sum -= 0xFF;
    }
    return (uint8_t)(~sum);
}

static uint8_t LIN_CalcPID(uint8_t id) {
    uint8_t p0 = ((id >> 0) ^ (id >> 1) ^ (id >> 2) ^ (id >> 4)) & 0x01;
    uint8_t p1 = ~((id >> 1) ^ (id >> 3) ^ (id >> 4) ^ (id >> 5)) & 0x01;
    return (id & 0x3F) | (p0 << 6) | (p1 << 7);
}

void PTC_Init(UART_HandleTypeDef *huart) {
    ptc_huart = huart;
    ptc.enable = false;
    ptc.power_pct = 0;
    ptc.comm_error = false;
    ptc.last_rx_tick = HAL_GetTick();

    expected_pid_rx = LIN_CalcPID(0x25);

    HAL_GPIO_WritePin(LIN_NSLP_GPIO_Port, LIN_NSLP_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LIN_WAKE_GPIO_Port, LIN_WAKE_Pin, GPIO_PIN_SET);
}

/* De Fundamentele Oplossing: Byte-voor-Byte State Machine */
void PTC_ProcessByte(uint8_t byte) {
    switch (rx_state) {
        case LIN_STATE_WAIT_SYNC:
            if (byte == 0x55) {
                rx_state = LIN_STATE_WAIT_PID;
            }
            break;

        case LIN_STATE_WAIT_PID:
            if (byte == expected_pid_rx) {
                rx_index = 0;
                rx_state = LIN_STATE_READ_DATA;
            } else if (byte == 0x55) {
                // Kan gebeuren als er twee syncs snel achter elkaar zijn, blijf in PID wacht
                rx_state = LIN_STATE_WAIT_PID;
            } else {
                // Foutieve ID of echo van ons eigen verzend-commando, wacht weer op 0x55
                rx_state = LIN_STATE_WAIT_SYNC;
            }
            break;

        case LIN_STATE_READ_DATA:
            ptc.rx_buffer[rx_index++] = byte;

            // We hebben 8 databytes + 1 checksum = 9 bytes nodig
            if (rx_index >= 9) {
                // Controleer checksum
                uint8_t calc_chk = LIN_CalcChecksum(expected_pid_rx, ptc.rx_buffer, 8);
                if (calc_chk == ptc.rx_buffer[8]) {
                    // Data is 100% succesvol en gevalideerd!
                    ptc.temp_c     = (int8_t)(ptc.rx_buffer[0] - 40);
                    ptc.current_a  = (float)ptc.rx_buffer[1] * 0.1f;
                    uint8_t hv_raw = ptc.rx_buffer[6] & 0x7F;
                    ptc.hv_voltage = (5.14f * (float)hv_raw) - 47.34f;

                    ptc.data_valid   = true;
                    ptc.comm_error   = false;
                    ptc.last_rx_tick = HAL_GetTick();
                } else {
                    ptc.error_count++;
                }

                // Reset state machine voor de volgende cyclus
                rx_state = LIN_STATE_WAIT_SYNC;
            }
            break;
    }
}

static void PTC_SendControl(void) {
    uint8_t pid = LIN_CalcPID(0x04);
    uint8_t data[2];

    if (ptc.enable && ptc.power_pct > 0) {
        data[0] = (ptc.power_pct > 100) ? 100 : ptc.power_pct;
    } else {
        data[0] = 0x80;
    }
    data[1] = 0xFE;

    uint8_t checksum = LIN_CalcChecksum(pid, data, 2);
    static uint8_t frame[5];
    frame[0] = 0x55;
    frame[1] = pid;
    frame[2] = data[0];
    frame[3] = data[1];
    frame[4] = checksum;

    // Reset de parser-status, want de echo van ons commando komt nu langs
    rx_state = LIN_STATE_WAIT_SYNC;

    HAL_LIN_SendBreak(ptc_huart);
    HAL_UART_Transmit(ptc_huart, frame, 5, 25);
}

static void PTC_RequestData(void) {
    uint8_t pid = LIN_CalcPID(0x25);
    static uint8_t header[2];
    header[0] = 0x55;
    header[1] = pid;

    // Zorg dat de parser klaar staat voor het inkomende antwoord (eigen echo + heater)
    rx_state = LIN_STATE_WAIT_SYNC;

    HAL_LIN_SendBreak(ptc_huart);
    HAL_UART_Transmit(ptc_huart, header, 2, 25);
}

void PTC_Update(void) {
    static uint32_t last_tick = 0;
    static uint8_t ptc_state = 0;
    uint32_t now = HAL_GetTick();

    if ((now - last_tick) >= 50) {
        last_tick = now;

        if (ptc_state == 0) {
            PTC_SendControl();
            ptc_state = 1;
        } else {
            PTC_RequestData();
            ptc_state = 0;
        }
    }

    if ((now - ptc.last_rx_tick) > 1000) {
        ptc.comm_error = true;
    }
}
