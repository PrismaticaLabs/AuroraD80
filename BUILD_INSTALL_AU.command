#!/bin/bash
set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"
PROJECT="$ROOT/Builds/MacOSX/AuroraD80.xcodeproj"
DEST="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$DEST"
for f in chassis.png header.png display.png input_meter.png output_meter.png delay_core.png tone_stereo.png mod_character.png utility_bar.png preset_bar.png knob_body.png knob_ticks.svg knob_shadow.svg knob_bezel.svg knob_pointer_glow.svg knob_pointer.svg knob_pointer_highlight.svg freeze_icon.svg bypass_icon.svg; do
  if [ ! -f "$ROOT/Resources/$f" ]; then
    echo "Falta Resources/$f — ejecuta primero REGENERATE_XCODE.command"
    exit 1
  fi
done
if [ ! -d "$PROJECT" ]; then echo "Primero ejecuta REGENERATE_XCODE.command"; exit 1; fi
SCHEME=$(xcodebuild -project "$PROJECT" -list 2>/dev/null | awk '/Schemes:/{f=1;next} f && /Standalone/{next} f && /AU/{gsub(/^ +| +$/,"",$0); print; exit}')
[ -z "$SCHEME" ] && SCHEME="AuroraD80 - AU"
echo "Compilando AU con scheme: $SCHEME"
xcodebuild -project "$PROJECT" -scheme "$SCHEME" -configuration Debug -destination 'platform=macOS' build
COMPONENT=$(find "$ROOT/Builds/MacOSX/build" "$HOME/Library/Developer/Xcode/DerivedData" -name 'AuroraD80.component' -type d 2>/dev/null | tail -1)
if [ -z "$COMPONENT" ]; then echo "Build terminó pero no encontré AuroraD80.component"; exit 2; fi
rm -rf "$DEST/AuroraD80.component"
cp -R "$COMPONENT" "$DEST/AuroraD80.component"
echo "Instalado: $DEST/AuroraD80.component"
killall -9 AudioComponentRegistrar 2>/dev/null || true
auval -a 2>/dev/null | grep -i AuroraD80 || true
open -R "$DEST/AuroraD80.component"
