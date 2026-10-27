# NovaRemote wxWidgets host

Build, run, and AI instructions are in ../README.md. Prefer that file.

First C++/wx slice of the Nova remote. No Qt, no Lua, no TCP.

Release builds against wxWidgets 3.1.3 in `../Reference/WX_Release_3.1.3`.

## Build

Open `NovaRemote.sln` in Visual Studio and build **Release | Win32**, or:

```
msbuild wxui\NovaRemote.sln /p:Configuration=Release /p:Platform=Win32
```

Output: `wxui\bin\Release\NovaRemote.exe`

The build copies `wxbase313u_vc14x.dll` and `wxmsw313u_core_vc14x.dll` next to the exe. Debug needs a full wx tree that this repo does not ship.

## Run

The window opens on **Home**. Meters animate until a serial port is opened.

- **Serial** menu: pick a COM port (38400 8O1, odd parity)
- **Pages**: Home / Meters
- Click **STOP** to jump to Meters
- Click **Meters TRACK 1-8** to toggle bus vs output meters

When the Nova sends packets, meters, timecode, and transport (STOP/PLAY/REC) update from the 0xAA protocol. Home, the mix pages, and Input Config show the live dump. A click on those controls sends a setting.
