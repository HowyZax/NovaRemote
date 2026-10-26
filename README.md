# NovaTouch

NovaTouch is a Windows remote for a Zaxcom Nova recorder. The program to build is the C++ project in `wxui\`. It uses wxWidgets.

`lua\` is the old screen and protocol scripts, kept so a page can be matched. They are not built and they are not linked into the program. `NovaSide_v5.23\` is a copy of the Nova firmware files that describe the serial protocol. Those files do not build either.

## Build on Windows

You need Visual Studio 2022 (toolset v143). wxWidgets 3.1.3 Release is already in `WX_Release_3.1.3\`, including its licence. The project is Win32, not x64. Debug still expects the private wx tree; the public build is Release.

Close `NovaTouch.exe` if it is already running. Then, in PowerShell:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" wxui\NovaTouch.sln /p:Configuration=Release /p:Platform=Win32 /m
```

The program is `wxui\bin\Release\NovaTouch.exe`. The build copies `wxbase313u_vc14x.dll` and `wxmsw313u_core_vc14x.dll` next to it.

You can also open `wxui\NovaTouch.sln` and build Release | Win32.

## Run

Plug in the Nova. From the Serial menu, pick its COM port. Or start with the port on the command line:

```text
wxui\bin\Release\NovaTouch.exe --port=COM16
```

The link is 38400 baud, 8 data bits, odd parity, 1 stop bit, no flow control. The program saves the last port and reopens it next time.

A click on a mix cell, an input control, record arm, sample rate, frame rate, preroll, tone, or a track name sends that setting to the Nova.

## Changing the program with Cursor

Use Cursor. Open this folder (File, Open Folder). Use the Agent chat, the one that can edit files and run the build, rather than a chat that only answers. Before you send a task, set the model in the picker at the top of the chat. Tell the agent to read this file and `NovaSide_v5.23\README.md` before it edits.

A prompt that works:

```text
Read README.md and NovaSide_v5.23\README.md. Then <what you want>.
Build Release Win32 when you are done. Leave the connected Nova's
sample rate, routing, names, and input setup alone unless I asked
you to change them.
```

The source files the agent must treat carefully already start with a comment marked `AI:`. Those comments name the files and the rules.

## Which model to pick

For serial, settings, mix pages, Input Config, or anything in `NovaSide_v5.23`, pick a current large coding model:

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
- `NovaSide_v5.23\`

Skip Auto when the picker is set to prefer a fast model and the task mentions the COM port, a setting name, a mix cell, or an input channel. A fast model will "correct" the input map, switch the port to 8N1, or trust the 2016 bounds written in `NovaSide_v5.23\protocol.h`. Those three mistakes are how this program loses the Nova.

If a fast model already edited one of those files, revert the edit and run the same request on Opus, Sonnet, GPT-5, or Grok 4. Asking the fast model to repair its own protocol edit makes a second wrong patch.

## What the agent should already know

`NovaSide_v5.23\README.md` is the contract for new features: packet layout, setting ids, the two channel maps, mix flags, and the input-page units. The AI comments at the top of the wxui files above are the short version for the PC program.

`lua\remote\pages` is the old screen layout. `lua\settings` and `lua\serial` are the old setting names and packet code.
