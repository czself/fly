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
pio run -e demo01_vofa -e demo02_remote -e demo02_remote_framed -e demo03_motor -e demo03_motor_bench -e demo04_hover_logic -e demo05_tof -e demo06_flow -e demo07_integration

python3 scripts/check_teaching_assets.py
