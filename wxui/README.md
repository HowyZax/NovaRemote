# NovaRemote wxWidgets host

Build, run, and AI instructions are in ../README.md. Prefer that file.

First C++/wx slice of the Nova remote. No Qt, no Lua, no TCP.

Windows Release builds against wxWidgets 3.2.12 in `../Reference/WX_Release_3.2.12`. macOS builds with `CMakeLists.txt` (see ../README.md).

## Build

Open `NovaRemote.sln` in Visual Studio and build **Release | Win32**, or:

```
msbuild wxui\NovaRemote.sln /p:Configuration=Release /p:Platform=Win32
```

Output: `wxui\bin\Release\NovaRemote.exe`

The build copies `wxbase32u_vc14x.dll` and `wxmsw32u_core_vc14x.dll` next to the exe. Debug needs the full wx 3.2.12 install (`e:\libs\wx-3.2.12` or `/p:WX_DIR=...`), which this repo does not ship.

Keep the source list in `CMakeLists.txt` in step with `wxui.vcxproj` when adding files.

## Run

The window opens on **Home**. Meters animate until a serial port is opened.

- **Serial** menu: pick a port (38400 8O1, odd parity). `COMn` on Windows, `/dev/cu.*` on a Mac
- **Pages**: Home / Meters
- Click **STOP** to jump to Meters
- Click **Meters TRACK 1-8** to toggle bus vs output meters

When the Nova sends packets, meters, timecode, and transport (STOP/PLAY/REC) update from the 0xAA protocol. Home, the mix pages, and Input Config show the live dump. A click on those controls sends a setting.
