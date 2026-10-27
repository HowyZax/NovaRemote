# NovaRemote

NovaRemote is a Windows and macOS remote for a Zaxcom Nova recorder. The program to build is the C++ project in `wxui\`. It uses wxWidgets 3.2.

Make changes in `wxui\` only. Everything under `Reference\` is there to read, not to edit:

- `Reference\lua\` is the old screen and protocol scripts, kept so a page can be matched. They are not built and they are not linked into the program.
- `Reference\NovaSide_v5.23\` is a copy of the Nova firmware files that describe the serial protocol. Those files do not build either. Changing them does not change the Nova.
- `Reference\WX_Release_3.2.12\` is the wxWidgets library the Windows build links against: headers, the Release import libs, and the two DLLs the program needs.

`.cursor\rules\reference-readonly.mdc` tells the Cursor agent the same thing.

## Build on Windows

You need Visual Studio 2022 (toolset v143). wxWidgets 3.2.12 Release is already in `Reference\WX_Release_3.2.12\`, including its licence. The project is Win32, not x64.

The easy way, from this folder:

```text
buildPC            Release build
buildPC run        Release build, then start it
```

It closes a running `NovaRemote.exe` first, finds MSBuild, and builds. By hand, after closing the program:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" wxui\NovaRemote.sln /p:Configuration=Release /p:Platform=Win32 /m
```

The program is `wxui\bin\Release\NovaRemote.exe`. The build copies `wxbase32u_vc14x.dll` and `wxmsw32u_core_vc14x.dll` next to it.

You can also open `wxui\NovaRemote.sln` and build Release | Win32.

Debug needs the debug DLLs, which are not in `Reference`. Unpack the official `wxWidgets-3.2.12-headers.7z`, `wxMSW-3.2.12_vc14x_Dev.7z`, and `wxMSW-3.2.12_vc14x_ReleaseDLL.7z` from the [wxWidgets 3.2.12 release](https://github.com/wxWidgets/wxWidgets/releases/tag/v3.2.12) into one folder and either put it at `e:\libs\wx-3.2.12` or pass `/p:WX_DIR=<that folder>` to MSBuild.

## Build on macOS

`MAC_BUILD.md` has the full story: what was done for the Mac, what to test first, and the known gaps. The short version:

You need Xcode command line tools (`xcode-select --install`) and CMake (`brew install cmake`). Then, in Terminal, from this folder:

```sh
./buildMAC.sh run
```

It quits a running NovaRemote, builds Release into `build/NovaRemote.app`, and opens it. `./buildMAC.sh debug` builds into `build-debug/`. By hand:

```sh
cmake -S wxui -B build -DCMAKE_BUILD_TYPE=Release -DNOVAREMOTE_FETCH_WX=ON
cmake --build build -j
open build/NovaRemote.app
```

`NOVAREMOTE_FETCH_WX=ON` downloads wxWidgets 3.2.12 and compiles it into the app, so the `.app` runs on any Mac without installing anything. The first build takes about ten minutes; after that only the program recompiles. If you already have `brew install wxwidgets`, leave the option off and it links against that instead.

The app is not signed. The first time, right-click `NovaRemote.app`, choose Open, and confirm. The serial log is written to `~/Library/Application Support/NovaRemote/serial.log`.

## Builds from GitHub

Every push runs `.github\workflows\build.yml`, which builds the Windows exe and a universal (Apple Silicon and Intel) macOS app. Open the run under the repo's Actions tab and download `NovaRemote-Windows` or `NovaRemote-macOS` from its Artifacts. This is the way to get a Mac build without a Mac; it cannot test the serial link, only that the program compiles.

## Run

Plug in the Nova. Press **Search Serial Ports** on the Setup page (or Serial menu, Search serial ports, Ctrl-D). It opens every serial port at once, sends the version string on each, and connects to the one a Nova answers on, usually within half a second. Or pick the port yourself from the Serial menu: `COM16` style on Windows, `/dev/cu.usbserial-...` on a Mac. Or start with the port on the command line:

```text
wxui\bin\Release\NovaRemote.exe --port=auto
wxui\bin\Release\NovaRemote.exe --port=COM16
build/NovaRemote.app/Contents/MacOS/NovaRemote --port=/dev/cu.usbserial-1420
```

The link is 38400 baud, 8 data bits, odd parity, 1 stop bit, no flow control. The program saves the last port and reopens it next time. On a Mac the USB-serial adapter must support odd parity; FTDI and CH340 adapters work with the drivers built into macOS, Prolific adapters need the maker's driver.

A click on a mix cell, an input control, record arm, sample rate, frame rate, preroll, tone, or a track name sends that setting to the Nova.

## Changing the program with Cursor

Use Cursor. Open this folder (File, Open Folder). Use the Agent chat, the one that can edit files and run the build, rather than a chat that only answers. Before you send a task, set the model in the picker at the top of the chat. Tell the agent to read this file and `Reference\NovaSide_v5.23\README.md` before it edits.

A prompt that works:

```text
Read README.md and Reference\NovaSide_v5.23\README.md. Then <what you want>.
Build Release Win32 when you are done (or the CMake build on a Mac). Leave the connected Nova's
sample rate, routing, names, and input setup alone unless I asked
you to change them.
```

The source files the agent must treat carefully already start with a comment marked `AI:`. Those comments name the files and the rules.

## Which model to pick

For serial, settings, mix pages, Input Config, or anything in `Reference\NovaSide_v5.23`, pick a current large coding model:

- Claude Opus
- Claude Sonnet
- GPT-5
- Grok 4

Any one of those can hold the protocol and the firmware notes at the same time. Opus is the one to retry with when a change touches channel indexes or the cSetting encoder and the first attempt was wrong.

A fast model is fine for colors, spacing, labels, and comments. In Cursor that is Composer, or a model whose name says Fast, Mini, Flash, Nano, or Lite. Before you send that kind of task, say which files it may edit, and keep it out of these:

- `wxui\src\protocol.h`
- `wxui\src\protocol.cpp`
- `wxui\src\serial.h`
- `wxui\src\serial.cpp`
- `wxui\src\settings.h`
- `wxui\src\settings.cpp`
- `wxui\src\app.cpp`
- `wxui\src\mixpages.cpp`
- `wxui\src\pages_more.cpp`
- `Reference\NovaSide_v5.23\`

Skip Auto when the picker is set to prefer a fast model and the task mentions the COM port, a setting name, a mix cell, or an input channel. A fast model will "correct" the input map, switch the port to 8N1, or trust the 2016 bounds written in `Reference\NovaSide_v5.23\protocol.h`. Those three mistakes are how this program loses the Nova.

If a fast model already edited one of those files, revert the edit and run the same request on Opus, Sonnet, GPT-5, or Grok 4. Asking the fast model to repair its own protocol edit makes a second wrong patch.

## What the agent should already know

`Reference\NovaSide_v5.23\README.md` is the contract for new features: packet layout, setting ids, the two channel maps, mix flags, and the input-page units. The AI comments at the top of the wxui files above are the short version for the PC program.

`Reference\lua\remote\pages` is the old screen layout. `Reference\lua\settings` and `Reference\lua\serial` are the old setting names and packet code.
