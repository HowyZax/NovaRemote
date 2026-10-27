# NovaRemote on macOS: where things stand and what to do next

Written 2026-10-27, after the first green macOS CI build.

## What was done

- The project moved from wxWidgets 3.1.3 (a 2019 development snapshot) to wxWidgets 3.2.12, the current stable release. Windows links the slim copy in `Reference/WX_Release_3.2.12`. The Mac compiles wxWidgets from source into the app.
- `wxui/CMakeLists.txt` was added. It is the Mac (and Linux) build. The Windows build is still `wxui/NovaRemote.sln`. Both list the same source files; keep them in step when adding a file.
- `wxui/mac/Info.plist.in` is the app bundle description (name, version, bundle id `com.zaxcom.novaremote`, minimum macOS 11).
- `wxui/src/serial.cpp` got a working macOS half:
  - Lists only `/dev/cu.*` ports. Every adapter also appears as `/dev/tty.*`; opening that name blocks until carrier detect, which the Nova never asserts. Bluetooth, debug, and wlan ports are hidden.
  - Sets 38400 baud, 8 data bits, odd parity, 1 stop bit, no flow control explicitly, and raises DTR and RTS like the Windows code.
  - Handles partial writes and drains all queued bytes on each read.
- `wxui/src/app.cpp`: the MSVC-only `_wfopen` became `wxFopen`. On a Mac the serial log goes to `~/Library/Application Support/NovaRemote/serial.log`, because the `.app` folder may be read-only.
- `.github/workflows/build.yml` builds on every push: a Windows Release exe and a universal (arm64 + x86_64) `NovaRemote.app`. Download them from the run's Artifacts on the repo's Actions tab.
- Two wxWidgets build options had to be set for the static universal build, both in `CMakeLists.txt`: `wxBUILD_PRECOMP OFF` (precompiled headers cannot be built for two architectures at once) and `wxUSE_WXHTML_HELP OFF` (required once `wxUSE_HTML` is off).

CI proves the program compiles and links on macOS. It has never been run against a Nova from a Mac.

## Build on the Mac

One-time setup:

```sh
xcode-select --install          # Apple compiler and tools
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"   # if Homebrew is missing
brew install cmake git
git clone https://github.com/HowyZax/NovaRemote.git
cd NovaRemote
```

Build:

```sh
./buildMAC.sh run
```

That quits a running NovaRemote, builds Release, and opens the app. Options: `debug` (builds into `build-debug/`), `run`. It is the same as:

```sh
cmake -S wxui -B build -DCMAKE_BUILD_TYPE=Release -DNOVAREMOTE_FETCH_WX=ON
cmake --build build -j
open build/NovaRemote.app
```

The first build downloads wxWidgets 3.2.12 (27 MB) and compiles it, about 5 to 10 minutes. After that only NovaRemote recompiles, in seconds. The result is `build/NovaRemote.app`, which runs on any Mac 11 or later with nothing installed.

To build for the Mac you are on only (faster than CI's universal build), leave `CMAKE_OSX_ARCHITECTURES` unset as above. To match CI, add `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.

If you prefer a Homebrew wxWidgets: `brew install wxwidgets`, then configure without `-DNOVAREMOTE_FETCH_WX=ON`. That links wx dynamically, so the app only runs on Macs that also have Homebrew wx.

Run with a port on the command line:

```sh
build/NovaRemote.app/Contents/MacOS/NovaRemote --port=/dev/cu.usbserial-1420
```

## What to test first

1. Plug in the Nova through the USB-serial adapter. `ls /dev/cu.*` should show one new `cu.usbserial-...` (FTDI) or `cu.wchusbserial...` (CH340) entry. If nothing appears, the adapter needs a driver. FTDI and CH340 use drivers built into macOS; Prolific needs the maker's download.
2. Open the app (first time: right-click, Open, because it is not signed). Press Auto Detect on the Setup page; it probes every `/dev/cu.*` port at once and should connect by itself. If it does not, the Serial menu lists the ports; there should be one entry per adapter, not two.
3. The version handshake should finish within a second or two and the meters go live. If it stays offline, read `~/Library/Application Support/NovaRemote/serial.log`. The most likely cause is the adapter driver ignoring odd parity; the Nova never answers at 8N1.
4. Click through the pages. Mac text is slightly wider and Retina scaling differs, so a few labels may clip. Those are cosmetic fixes in `wxui/src`.
5. Change one safe setting, such as a track name, and confirm it lands on the Nova.
6. Watch CPU in Activity Monitor while meters run. The 50 ms poll timer was tuned on Windows.

## Known gaps

- Not signed or notarized. Users must right-click, Open the first time. Fixing that needs an Apple Developer ID ($99/yr); the signing and notarization steps can then be added to the CI job.
- No app icon yet.
- Debug builds on Windows need the full wx install (`e:\libs\wx-3.2.12` or `/p:WX_DIR=...`); on the Mac, use `-DCMAKE_BUILD_TYPE=Debug` and it just works.

## Working with Cursor on the Mac

Open the cloned folder in Cursor. `.cursor/rules/reference-readonly.mdc` loads automatically and keeps the agent out of `Reference/`. The model advice in `README.md` applies unchanged. Tell the agent to build with the CMake commands above instead of MSBuild.
