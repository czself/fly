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
