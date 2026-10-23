#!/bin/sh
set -eu
TASK_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
mkdir -p "$TASK_ROOT/optimizer/build"
CXX=${CXX:-clang++}
"$CXX" -O3 -std=c++17 "$TASK_ROOT/optimizer/beam.cpp" -o "$TASK_ROOT/optimizer/build/beam"
"$CXX" -O3 -std=c++17 "$TASK_ROOT/optimizer/static.cpp" -o "$TASK_ROOT/optimizer/build/static"
exec python3 "$TASK_ROOT/optimizer/run.py" --binary "$TASK_ROOT/optimizer/build/beam" "$@"
