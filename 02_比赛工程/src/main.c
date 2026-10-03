/* Competition development entry: real sensor + wireless task preview.
 * NO physical motor output exists here. See README for pending flight acceptance. */
#include "common.h"
#include "gpio.h"
#include "system_clock.h"
#include "usart.h"
#include "i2c.h"
#include "sensors.h"
#include "f22_sensor_bus.h"
#include "tof.h"
#include "flow.h"
#include "observation.h"
#include "hover.h"
#include "task_input.h"
#include "config.h"
#include <math.h>
#include <string.h>

#define RX_QUEUE_SIZE 256U
static volatile uint8_t rx_queue[RX_QUEUE_SIZE], rx_head, rx_tail, rx_error;
static volatile uint32_t rx_times[RX_QUEUE_SIZE];
static uint8_t rx_byte;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart != &huart1) return;
    uint8_t next = (uint8_t)(rx_head + 1U);
    if (next == rx_tail) rx_error = 1U;
    else { rx_queue[rx_head] = rx_byte; rx_times[rx_head] = HAL_GetTick(); rx_head = next; }
    if (HAL_UART_Receive_IT(&huart1, &rx_byte, 1U) != HAL_OK) rx_error = 1U;
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{ if (uart == &huart1) rx_error = 1U; }
static void ReceiveReset(TaskInput *input)
{
    uint32_t mask = __get_PRIMASK(); __disable_irq();
    (void)HAL_UART_AbortReceive(&huart1);
    rx_head = rx_tail = rx_error = 0U; TaskInput_Init(input);
    if (HAL_UART_Receive_IT(&huart1, &rx_byte, 1U) != HAL_OK) rx_error = 1U;
    __set_PRIMASK(mask);
}
static uint8_t LocalStop(void)
{
    return HAL_GPIO_ReadPin(BOARD_KEY1_GPIO_Port, BOARD_KEY1_Pin) == GPIO_PIN_RESET ||
           HAL_GPIO_ReadPin(BOARD_KEY2_GPIO_Port, BOARD_KEY2_Pin) == GPIO_PIN_RESET;
}
static void Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    SCB->VTOR = FLASH_BASE; __DSB(); __ISB(); __enable_irq();
    HAL_Init(); SystemClock_Config(); MX_GPIO_Init();
    MX_USART1_UART_Init(); MX_USART4_UART_Init(); MX_I2C2_Init();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7; gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW; HAL_GPIO_Init(GPIOA, &gpio);
    F22_ModuleBusInit();
    HAL_NVIC_SetPriority(USART1_IRQn, 2U, 0U); HAL_NVIC_EnableIRQ(USART1_IRQn);
}
static void SendFloats(const float values[32])
{
    uint8_t frame[132]; const uint8_t tail[4] = {0,0,0x80,0x7f};
    _Static_assert(sizeof(float) == 4U, "JustFloat float32 required");
    memcpy(frame, values, 128U); memcpy(frame + 128U, tail, 4U);
    (void)HAL_UART_Transmit(&huart4, frame, sizeof(frame), 20U);
}
static unsigned Calibrate(float bias[3])
{
    ImuSample sample; float sum[3] = {0}; uint8_t id = 0;
    if (Sensors_ImuReadId(&id) != HAL_OK || id != 0x68U || Sensors_ImuInit() != HAL_OK) return 0;
    for (unsigned n = 0; n < 100U; n++) {
        if (Sensors_ImuRead(&sample) != HAL_OK) return 0;
        float norm = 0;
        for (unsigned k = 0; k < 3; k++) {
            float gyro = sample.gyro[k] / 131.0f, acc = sample.accel[k] / 16384.0f;
            if (fabsf(gyro) > 2.0f) return 0;
            sum[k] += gyro; norm += acc * acc;
        }
        if (norm < .81f || norm > 1.21f) return 0;
        HAL_Delay(5U);
    }
    for (unsigned k = 0; k < 3; k++) bias[k] = sum[k] / 100.0f;
    return 1;
}
static void Rotate(const float in[3], float out[3])
{
    for (unsigned i = 0; i < 3; i++)
        out[i] = imu_mount[3*i]*in[0] + imu_mount[3*i+1]*in[1] + imu_mount[3*i+2]*in[2];
}
int main(void)
{
    TofSensor tof; FlowSensor flow; Attitude attitude; HeightEstimate height;
    FlowEstimate position; HoverController controller; HoverOutput output; TaskInput input;
    float bias[3] = {0}, gyro[3] = {0}; unsigned errors = 0;
    Init(); unsigned imu_ready = Calibrate(bias);
    Attitude_Init(&attitude); Height_Reset(&height); FlowEstimate_Reset(&position);
    Hover_Init(&controller); TaskInput_Init(&input);
    (void)Tof_Init(&tof, F22_TofIo()); (void)Flow_Init(&flow, F22_FlowIo());
    ReceiveReset(&input);
    uint32_t previous = HAL_GetTick(), retry = previous;
    while (1) {
        uint32_t now = HAL_GetTick(); float dt = (now - previous) * .001f; previous = now;
        if (dt == 0) dt = .02f;
        if (rx_error) { ReceiveReset(&input); ++errors; }
        while (rx_tail != rx_head) {
            uint8_t byte = rx_queue[rx_tail]; uint32_t arrived = rx_times[rx_tail];
            rx_tail = (uint8_t)(rx_tail + 1U); TaskInput_Feed(&input, byte, arrived);
        }
        /* The ISR may receive a byte after the loop's initial now was sampled. */
        uint32_t command_now = HAL_GetTick();
        HoverCommand command = TaskInput_Take(&input, command_now);
        if (LocalStop()) command = HOVER_STOP;
        /* Blocking calibration/reconnect are permitted only while not active.
         * Flush queued bytes afterward: an old START cannot become newly fresh. */
        unsigned recalibrate = TaskInput_TakeCalibration(&input, controller.state, command_now);
        if (recalibrate && command == HOVER_NONE) {
            imu_ready = Calibrate(bias); Attitude_Init(&attitude);
            Height_Reset(&height); FlowEstimate_Reset(&position); ReceiveReset(&input);
            previous = HAL_GetTick(); continue;
        }
        if ((controller.state == HOVER_IDLE || controller.state == HOVER_FAULT) &&
            command == HOVER_NONE && (!imu_ready || !tof.present || !flow.present) &&
            (uint32_t)(now - retry) >= 2000U) {
            if (!imu_ready) { imu_ready = Calibrate(bias); Attitude_Init(&attitude); }
            if (!tof.present) (void)Tof_Init(&tof, F22_TofIo());
            if (!flow.present) (void)Flow_Init(&flow, F22_FlowIo());
            retry = HAL_GetTick(); previous = retry; ReceiveReset(&input); continue;
        }
        ImuSample imu; float acc_sensor[3], gyro_sensor[3], acc[3];
        if (imu_ready && Sensors_ImuRead(&imu) == HAL_OK) {
            for (unsigned k = 0; k < 3; k++) {
                acc_sensor[k] = imu.accel[k] / 16384.0f;
                gyro_sensor[k] = imu.gyro[k] / 131.0f - bias[k];
            }
            Rotate(acc_sensor, acc); Rotate(gyro_sensor, gyro);
            (void)Attitude_Update(&attitude, acc, gyro, dt);
        } else { imu_ready = 0; attitude.valid = 0; }
        int fresh = Tof_Poll(&tof); (void)Flow_Poll(&flow);
        if (fresh == 1 && tof.valid && attitude.valid)
            (void)Height_Update(&height, tof.distance_mm*.001f, attitude.roll, attitude.pitch, tof.last_sample_ms);
        if (!tof.valid || !attitude.valid) Height_Reset(&height);
        position.valid = 0;
        if (flow.valid && height.valid && attitude.valid)
            (void)FlowEstimate_Update(&position, &flow_calibration, flow.dx, flow.dy,
                                      height.height, gyro, attitude.yaw, dt);
        unsigned link = Radio_LinkValid(&input.radio, HAL_GetTick());
        HoverSample sample = {.roll_deg=attitude.roll, .pitch_deg=attitude.pitch, .yaw_deg=attitude.yaw,
            .gyro_dps={gyro[0],gyro[1],gyro[2]}, .height_m=height.height, .vertical_speed_mps=height.vz,
            .x_m=position.x, .y_m=position.y, .vx_mps=position.vx, .vy_mps=position.vy,
            .imu_valid=attitude.valid && IMU_MOUNT_VERIFIED, .height_valid=height.valid,
            .position_valid=position.valid, .command_link_valid=link};
        Hover_Step(&controller, &sample, command, dt, &output);
        float ch[32] = {output.state, attitude.valid?attitude.roll:NAN, attitude.valid?attitude.pitch:NAN,
            attitude.valid?attitude.yaw:NAN, height.valid?height.height:NAN, height.valid?height.vz:NAN,
            position.valid?position.x:NAN, position.valid?position.y:NAN,
            position.valid?position.vx:NAN, position.valid?position.vy:NAN, flow.dx, flow.dy, flow.quality,
            imu_ready, tof.valid, flow.valid, IMU_MOUNT_VERIFIED, FLOW_CALIBRATED,
            output.height_target_m, output.collective, output.virtual_motor[0], output.virtual_motor[1],
            output.virtual_motor[2], output.virtual_motor[3], 0, controller.stable_seconds,
            controller.stable_seconds>5.0f, link, input.radio.command, input.radio.accepted, errors, 0};
        SendFloats(ch);
        HAL_GPIO_WritePin(BOARD_LED1_GPIO_Port, BOARD_LED1_Pin,
                          link?BOARD_LED1_ON_LEVEL:BOARD_LED1_OFF_LEVEL);
        uint32_t elapsed = HAL_GetTick() - now; if (elapsed < 20U) HAL_Delay(20U - elapsed);
    }
}
