#!/bin/sh
# Build NovaRemote on macOS. Quits a running NovaRemote first.
#
#   ./buildMAC.sh             Release build -> build/NovaRemote.app
#   ./buildMAC.sh run         Release build, then open it
#   ./buildMAC.sh debug run   Debug build -> build-debug/NovaRemote.app, then open it
#
# The first build downloads wxWidgets 3.2.12 and compiles it into the app
# (5 to 10 minutes). After that only NovaRemote recompiles.
set -e
cd "$(dirname "$0")"

CONFIG=Release
RUN=0
for a in "$@"; do
    case "$a" in
        debug) CONFIG=Debug ;;
        run)   RUN=1 ;;
    esac
done

BUILD=build
[ "$CONFIG" = Debug ] && BUILD=build-debug

pkill -x NovaRemote 2>/dev/null || true

cmake -S wxui -B "$BUILD" -DCMAKE_BUILD_TYPE=$CONFIG -DNOVAREMOTE_FETCH_WX=ON
cmake --build "$BUILD" -j "$(sysctl -n hw.ncpu)"

echo
echo "Built $BUILD/NovaRemote.app"
[ $RUN = 1 ] && open "$BUILD/NovaRemote.app"
exit 0
