#!/bin/sh
# Build Dvlce Duet on macOS: Xcode (or its command line tools) + CMake.  Universal binary (arm64 + x86_64).
#   ./build-mac.sh          core + tests + plugin (AU, VST3, Standalone)
#   ./build-mac.sh core     core + tests only (no JUCE)
# Environment (all optional): BUILD_DIR, JUCE_DIR (a local JUCE 8.0.13 checkout), JOBS.
set -e
cd "$(dirname "$0")"
BUILD_DIR=${BUILD_DIR:-build}
PLUGIN=ON
if [ "$1" = "core" ]; then BUILD_DIR="$BUILD_DIR-core"; PLUGIN=OFF; fi
JUCEARG=""
if [ -n "$JUCE_DIR" ]; then JUCEARG="-DFETCHCONTENT_SOURCE_DIR_JUCE=$JUCE_DIR"; fi
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DDVLCE_DUET_BUILD_PLUGIN=$PLUGIN $JUCEARG
cmake --build "$BUILD_DIR" -j "${JOBS:-2}"
"$BUILD_DIR/dvlce_duet_core_tests"
if [ "$PLUGIN" = "ON" ]; then "$BUILD_DIR/plugin/DvlceDuetSidechainTest"; "$BUILD_DIR/plugin/DvlceDuetSplitTest"; fi
