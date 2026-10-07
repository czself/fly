#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
task_test_dir=$(mktemp -d)
trap 'rm -rf "$task_test_dir"' EXIT
cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -g \
    -Isrc/flight tests/test_task_input.c src/flight/task_input.c src/flight/radio.c \
    src/flight/hover.c src/flight/pid.c -lm -o "$task_test_dir/task_input"
"$task_test_dir/task_input"
python3 - <<'PY'
import csv
with open('vofa.csv', newline='') as handle:
    rows=list(csv.reader(handle))
assert len(rows)==2 and len(rows[0])==len(rows[1])==32
assert all(float(value)==0 for value in rows[1])
print('Competition CSV: 32 columns, all initial values zero')
PY
python3 scripts/check_docs.py
pio run -e contest_hover
if arm-none-eabi-nm .pio/build/contest_hover/firmware.elf | \
   grep -Eq 'Board_MotorPwm|HAL_TIM_PWM_Start'; then
    echo 'contest_hover unexpectedly links physical motor PWM code' >&2
    exit 1
fi
cmp .pio/build/contest_hover/firmware.bin firmware/contest_hover.bin
python3 - <<'PY'
import hashlib
import json
from pathlib import Path

root = Path('.')
manifest = json.loads((root / 'docs/firmware_sha256.json').read_text())
for relative, expected in manifest.items():
    actual = hashlib.sha256((root / relative).read_bytes()).hexdigest()
    assert actual == expected, f'{relative}: SHA256 mismatch'
print('Competition firmware isolation, build snapshot and manifest passed')
PY
