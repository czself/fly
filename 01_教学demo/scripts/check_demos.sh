#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
task_dir=$(mktemp -d /tmp/f22-lessons.XXXXXX)
trap 'rm -rf "$task_dir"' EXIT
cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined \
   -fno-omit-frame-pointer -Isrc/flight \
   tests/test_flight.c src/flight/remote.c src/flight/radio.c src/flight/motor_guard.c \
   src/flight/pid.c src/flight/hover.c src/flight/hover_sim.c -lm -o "$task_dir/test_flight"
"$task_dir/test_flight"
cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined \
   -fno-omit-frame-pointer -Isrc/flight -Isrc/sensors \
   tests/test_sensors.c src/sensors/tof.c src/sensors/flow.c src/flight/observation.c \
   -lm -o "$task_dir/test_sensors"
"$task_dir/test_sensors"
pio run -e demo00_template -e demo01_gpio_led -e demo02_key_led -e demo05_i2c_scan \
  -e demo_selftest -e demo_vofa -e demo_uart_probe -e demo01_vofa \
  -e demo02_remote -e demo02_remote_framed \
  -e demo03_motor -e demo03_motor_bench -e demo04_hover_logic \
  -e demo05_tof -e demo06_flow -e demo07_integration
if arm-none-eabi-nm .pio/build/demo_selftest/firmware.elf | \
   grep -Eq 'Board_MotorPwm|HAL_TIM_PWM_Start'; then
    echo 'safe demo_selftest unexpectedly links motor PWM code' >&2
    exit 1
fi

python3 scripts/check_teaching_assets.py
