# Nova PC interface, firmware v5.23

These files are Zaxcom copyright, all rights reserved. See `COPYRIGHT.txt`. They are not under the MIT License at the repository root.

Snapshot of the Nova recorder sources that define what a PC remote can do. `gSoftwareVersion` in `Version.cpp` is `"v5.23"`. These files are a reference for adding NovaTouch features. They are not a firmware project and will not build by themselves.

`ZaxLan.h` in this folder matches Nova v5.23 plus the command names that QRX400 (`common/ZaxLanCmdCodes.h` and every `common_copy`) and HowyTerm (`howy_term/ZaxLanCmdCodes.h`) already agree on. Opcode values between those two trees matched. Nova still had the old unused names `cUNUrx12_9` / `cUNUrx12_a` and a single `cHT_Button` at `0xB1`. Those slots are now also named `cGNVDATA`, `cPICObytes`, and the HowyTerm block `cHT_PlaceButton` (`0xB0`) through `cHT_Cmd` (`0xB6`). `cHT_Button` is an alias of `cHT_ButtonPress` (`0xB2`). The old header had that name at `0xB1`, which is `cHT_EditButton`. Nova's handler only prints the packet, and it now matches a press. `cPICO_STATUS` (`0x81`) and `cProgramLarge` (`0x82`) were added. The commented-out `cPeaksMeters_PC*` block in Nova overlaps `0x80`–`0x85` and is not live. QRX4-SDpico's own `ZaxLan.h` was missing `cHT_DefineTable`, `cHT_UpdateTable`, and `cHT_Cmd`; those three were added there too.

## Files

| File | Use it for |
|---|---|
| `protocol.h` | Setting id, name, menu, and the array shape the 2016 generator recorded. |
| `ProtocolStrings.h` | The same catalog as strings. |
| `PC_GUI_OFFSETS.h` | Packed mix-source indexes. |
| `ZaxLan.h` | Command bytes on the wire. |
| `ZaxLanCmdStrings.cpp` | Command bytes by name. |
| `RemoteOasis.cpp` | Frame layout, version reply, and meter / timecode send. |
| `Remote_PC.cpp` | How a `cSetting` is encoded, dumped, and applied. |
| `AudioMix.h` | `gChannelConfig` channel numbers, including the dummy slots. |
| `NomadUIsettings.h` | Declarations for the handlers `Remote_PC.cpp` calls. |
| `NomadUIsettings.cpp` | What a received value actually changes, and `ConvertPCchanToNova`. |

`protocol.h` says `PROTOCOL_VERSION "00.01"`. That string is stale. The PC must identify as `PCnovaT-0.92.10#01.01`. The `01.01` after `#` is the protocol version the recorder accepts.

## Wire

Serial port: 38400 baud, 8 data bits, odd parity, 1 stop bit, no flow control. Even parity or 8N1 never completes the handshake.

A short packet is 6 bytes plus payload:

| Offset | Field |
|---|---|
| 0 | `0xAA` |
| 1 | Checksum, low 8 bits of the sum of every byte from offset 2 onward |
| 2 | Payload length |
| 3 | Source. Zero on PC packets. For commands `>= 0x6f` this byte is the high half of a 16-bit length (`IS_BLEN16_CMD`) |
| 4 | Destination, zero |
| 5 | Command |

PC commands are all below `0x6f`, so the length is the single byte at offset 2.

| Command | Value | Role |
|---|---|---|
| `cInPeaksMix8` … `cInPeaksPC_COMP` | `0x10`–`0x1C` | Meter peaks. `0x10` Mix8, `0x11` track/bus, `0x12` output, `0x18` inputs. `+9` is the compressor twin |
| `cTimecode` | `0x16` | Timecode. Bit 6 of the format byte means ignore the time |
| `cVersion` | `0x20` | Empty payload asks for the version. Nova replies `NOVA1 v5.23 sn######` |
| `cRequest` | `0x54` | Empty payload asks Nova to dump settings |
| `cSetting` | `0x55` | One setting, PC to Nova or Nova to PC |
| `cCommand` | `0x58` | A command string such as `mp3.record` |

On connect the PC sends `cVersion` with `PCnovaT-0.92.10#01.01`, then an empty `cRequest`. Without that request a reconnect only gets battery and status. No packets for 3 seconds means the link is down. Meter packets can be skipped in logs; they are continuous.

`cInPeaksMix8` ends with one extra peak for whatever input `Nomad.Setup.Chan` currently is. That is the Input Config meter. The packet can be shorter than 17 bytes when the trailing peaks are quiet.

## Settings

`cSetting` payload, little-endian:

1. Setting id, `uint16`. Ids are the numbers in `protocol.h` and match `settings/id.lua` (id = table index).
2. Kind, `int8`. `0` C string, `1`–`4` signed integer of that many bytes, `5` key/value strings.
3. Rank, then one bound byte per dimension.
4. For an array: count of index bytes, then those indexes, **0-based on the wire**. NovaTouch stores them 1-based.
5. Value. A zero number or an empty string is omitted. Later values in the same packet may send only the last index byte until `0xFF`, which starts a full index again.

Five settings are stored one higher than the wire value (`protocol.offset = -1`): `trackmix.preset`, `outputmix.preset`, `hpmix.preset`, `input.channel`, `input.configure`. Menu values are 0-based (`OFF` = 0).

The bounds written in `protocol.h` are the 2016 generator, not what v5.23 transmits. Trust `Remote_PC.cpp` `SendAllToTablet` / the `SendSetting_Array*` calls. Examples: `trackmix.matrix` is dumped as `[8,16,22]`, `outputmix.matrix` as `[1,8,50]`, input arrays as 22 wide (`NUM_PC_INPUTS`).

## Two channel maps

Mix sources and the PC input list are packed. `PC_GUI_OFFSETS.h`, 0-based:

| Source | Index |
|---|---|
| Analog 1–4 | 0–3 |
| Return 1–2 | 4–5 |
| AES 1–8 | 6–13 |
| RX 1–8 | 14–21 |
| Output 1–4 | 22–25 (`0x16`–`0x19`) |
| Track 1–16 | 30–45 (`0x1e`–`0x2d`) |

`gChannelConfig` is a different list (`AudioMix.h`): analog 0–3, return 4–5, **ten dummy slots 6–15**, AES 16–23, RX 24–31. `ConvertPCchanToNova` adds 10 when the PC index is `>= 6`. `ConvertNovaChanToPC` subtracts 10 when the Nova channel is `>= 16`.

Dumps of `input.level`, 48V, delay, HPF, phase, trim, and compressor use that conversion for the **value**, and send the **packed PC index**. `input.channel` and `input.configure` are converted on the way in. The other input settings in `ApplySetting` do `Nomad.Setup.Chan = cell->Col` with no conversion. A packed index of 6 (AES 1) therefore lands on dummy channel 6. Analog 1–4 and return 1–2 are the same in both lists, so those clicks line up.

There used to be four phantom digital inputs between AES and RX. The skip is the commented `j += 4` in the output-mix dump. v5.23 does not skip them.

## Mix

`SetCrosspointFromPC` reads `cell->Col` as the bus and `cell->Row` as the source. A 3-index setting arrives as `{preset, bus, source}` and `ApplySetting` stores that as `Zdim`, `Col`, `Row`.

Flags in the matrix byte: assigned `0x01`, post-fade `0x02`, phase invert `0x04`. No post bit means pre-fade. Outputs ignore fader and phase.

Output buses 0–3 take sources 0–21 (analog, return, AES, RX). Buses 4–7 take returns at 4–5, outputs at `0x16`–`0x19`, and tracks at `0x1e`–`0x2d`. The disk mixer takes 0–21 plus outputs `0x16`–`0x19`.

Track mix is advertised with a source bound of 22, but outputs are still written and accepted at wire indexes 22–25. An encoder that refuses anything past the advertised bound cannot assign OUT1–4 to a track.

Headphone menus `hpmix.tracks`, `hpmix.outputs`, `hpmix.returns`: `''`, `L`, `R`, `LR`, `/L`, `/R`, `/LR`, `L/R`, `/L/R` (0–8). Presets are 1–12 with offset −1. Enable is per preset, 0 off / 1 on. Phase is the scalar Normal / Invert.

## Input page values

`input.level` is `0` MIC, `1` LINE, and the firmware inverts it into `fMicLevel`. `input.power48V`, delay enable, HPF enable, phase (`0` normal, `1` invert), and notch enable are 0/1. Delay time is in 0.1 ms steps and travels as one byte (25.5 ms max on the wire). HPF frequency is Hz, 40–230. Notch frequency is sent as one byte even though the DSP value is an `int16` in Hz. Trim level is a signed byte, −20…30 dB.

`input.compress.enable` is per channel, 0/1. `input.compress.link` is dumped as 0–3 (`NONE`, `GROUP A`, `GROUP B`, `GROUP C`). Both ids are applied by `OnSetting_InputLimiter_enable`, which reads one combined code: `0` off, `1` on with no group, `2`–`4` on and group A–C. A link value sent as 0–3 is interpreted in that combined scale.

Attack, decay, ratio, threshold, and gain are the single input compressor, array index 0 (1-based index 1), not per channel. Attack and decay are milliseconds (`seconds * 1000` on the wire). Ratio is tenths (`4.0` is sent as 40). Threshold and gain are dB. Threshold is forced to a signed byte.

## Adding a feature

Find the setting id in `protocol.h`, then find every `SETTING_` use in `Remote_PC.cpp` (dump and `ApplySetting`) and the `OnSetting*` handler in `NomadUIsettings.cpp`. The dump shows the index space and the units. The handler shows what the recorder will do with a value the PC sends, including whether `ConvertPCchanToNova` is called. Menu text in the `protocol.h` comment is the 0-based order.
