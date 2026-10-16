#!/bin/sh
set -eu

test_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
source_dir="$test_dir/../esphome/components/navien"
test_case=all

usage() {
  echo "Usage: $0 [--source COMPONENT_DIR] [--case all|lengths|packets|failures]"
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --source|--case)
      if [ "$#" -lt 2 ]; then
        usage >&2
        exit 2
      fi
      case "$1" in
        --source) source_dir=$2 ;;
        --case) test_case=$2 ;;
      esac
      shift 2
      ;;
    -h|--help) usage; exit 0 ;;
    *) usage >&2; exit 2 ;;
  esac
done

case "$test_case" in
  all|lengths|packets|failures) ;;
  *) usage >&2; exit 2 ;;
esac

build_dir=$(mktemp -d "${TMPDIR:-/tmp}/navien-test.XXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

mkdir -p "$build_dir/esphome/core" "$build_dir/esphome/components/sensor" \
  "$build_dir/esphome/components/uart"
for header in esphome.h esphome/core/component.h \
  esphome/components/sensor/sensor.h esphome/components/uart/uart.h; do
  printf '#pragma once\n' > "$build_dir/$header"
done
cat > "$build_dir/esphome/core/helpers.h" <<'EOF'
#pragma once
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <sys/types.h>
EOF
cat > "$build_dir/esphome/core/log.h" <<'EOF'
#pragma once
#define ESP_LOGE(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGD(...) ((void)0)
#define ESP_LOGV(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
EOF

# Isolate the parser from ESPHome entities; keep the production method bodies intact.
if [ "$(grep -c '^#include "navien.h"$' "$source_dir/navien_link.cpp")" != 1 ]; then
  echo 'Expected exactly one navien.h include in navien_link.cpp' >&2
  exit 1
fi
sed 's/^#include "navien.h"$/#include "navien_link.h"/' \
  "$source_dir/navien_link.cpp" > "$build_dir/navien_link.cpp"

"${CXX:-c++}" -std=c++17 -g \
  -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
  -I"$build_dir" -I"$source_dir" \
  "$build_dir/navien_link.cpp" "$test_dir/regression.cpp" -o "$build_dir/regression"
"$build_dir/regression" "$test_case"
