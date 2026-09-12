#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
RES="$ROOT/Resources"

# Los exports aprobados de Figma se guardan en el proyecto. Los enlaces de
# exportación de Figma son temporales y no deben formar parte del rebuild.
# En particular, los meters originales están protegidos y nunca se descargan.
declare -a REQUIRED=(
  "chassis.png:1535:1023"
  "header.png:1125:84"
  "display.png:745:242"
  "input_meter.png:216:727"
  "output_meter.png:216:727"
  "delay_core.png:563:237"
  "tone_stereo.png:553:237"
  "mod_character.png:1112:223"
  "utility_bar.png:1488:96"
  "preset_bar.png:1488:86"
  "knob_body.png:118:118"
)

declare -a VECTOR_ASSETS=(
  knob_ticks.svg knob_shadow.svg knob_bezel.svg knob_pointer_glow.svg
  knob_pointer.svg knob_pointer_highlight.svg freeze_icon.svg bypass_icon.svg
)

fail=0
echo "Validando assets aprobados de AURORA D80..."

for entry in "${REQUIRED[@]}"; do
  IFS=: read -r name expected_width expected_height <<< "$entry"
  path="$RES/$name"
  if [ ! -f "$path" ]; then
    echo "  ERROR: falta Resources/$name"
    fail=1
    continue
  fi

  width="$(/usr/bin/sips -g pixelWidth "$path" 2>/dev/null | /usr/bin/awk '/pixelWidth:/{print $2}')"
  height="$(/usr/bin/sips -g pixelHeight "$path" 2>/dev/null | /usr/bin/awk '/pixelHeight:/{print $2}')"
  if [ "$width" != "$expected_width" ] || [ "$height" != "$expected_height" ]; then
    echo "  ERROR: $name mide ${width}x${height}; se esperaba ${expected_width}x${expected_height}"
    fail=1
  else
    echo "  OK: $name (${width}x${height})"
  fi
done

for name in "${VECTOR_ASSETS[@]}"; do
  if [ ! -s "$RES/$name" ]; then
    echo "  ERROR: falta Resources/$name o está vacío"
    fail=1
  else
    echo "  OK: $name"
  fi
done

check_meter() {
  local name="$1"
  local expected_hash="$2"
  local actual_hash
  actual_hash="$(/usr/bin/shasum -a 256 "$RES/$name" | /usr/bin/awk '{print $1}')"
  if [ "$actual_hash" != "$expected_hash" ]; then
    echo "  ERROR: $name cambió. El rebuild se detiene para proteger los meters."
    fail=1
  fi
}

check_meter "input_meter.png" "0299927b33df96e50c44563c758511d539db1433fcc472f0471f7c203c461f52"
check_meter "output_meter.png" "e73d7855ccfe42abe09d8c9412f6c7e492d759a8c898800879454f3c81d4fe91"

if [ "$fail" -ne 0 ]; then
  echo "Validación fallida. No se descargó ni reemplazó ningún archivo."
  exit 1
fi

echo "Assets verificados. Los meters permanecen intactos."
