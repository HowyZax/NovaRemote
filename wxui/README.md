# NovaTouch wxWidgets host

Build, run, and AI instructions are in ../README.md. Prefer that file.

First C++/wx slice of the Nova remote. No Qt, no Lua, no TCP.

Uses the same wxWidgets install as howy_term (`HT_LIBS_DIR`, usually `e:\libs\wx`).

## Build

Open `NovaTouch.sln` in Visual Studio and build **Release | Win32**, or:

```
msbuild wxui\NovaTouch.sln /p:Configuration=Release /p:Platform=Win32
```

Output: `wxui\bin\Release\NovaTouch.exe`

Put the wx DLLs on PATH (Visual Studio debugger already does this):

- Release: `e:\libs\wx\lib\vc14x_dll` (`wxbase313u_vc14x.dll`, `wxmsw313u_core_vc14x.dll`)
- Debug: `e:\libs\wx\lib\vc_dll` (`wxbase313ud_vc14x.dll`, `wxmsw313ud_core_vc14x.dll`)

## Run

The window opens on **Home**. Meters animate until a serial port is opened.

- **Serial** menu: pick a COM port (38400 8O1, odd parity)
- **Pages**: Home / Meters
- Click **STOP** to jump to Meters
- Click **Meters TRACK 1-8** to toggle bus vs output meters

When the Nova sends packets, meters, timecode, and transport (STOP/PLAY/REC) update from the 0xAA protocol. Home, the mix pages, and Input Config show the live dump. A click on those controls sends a setting.
