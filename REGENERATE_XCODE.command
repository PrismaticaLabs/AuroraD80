#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
PROJUCER="/Applications/JUCE/Projucer.app/Contents/MacOS/Projucer"

echo "AURORA D80 — Figma Production Rebuild"
"$ROOT/FETCH_FIGMA_ASSETS.command"

if [ ! -x "$PROJUCER" ]; then
  echo "No encontré Projucer en $PROJUCER"
  echo "Abre AuroraD80.jucer manualmente con Projucer y pulsa Save Project."
  exit 1
fi

echo "Regenerando Xcode..."
"$PROJUCER" --resave "$ROOT/AuroraD80.jucer"
PROJECT="$ROOT/Builds/MacOSX/AuroraD80.xcodeproj"
if [ -d "$PROJECT" ]; then
  open "$PROJECT"
else
  echo "No se encontró $PROJECT"
  exit 1
fi
