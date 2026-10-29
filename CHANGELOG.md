# Changelog

Each version has a part for users and a part for developers. The Nova firmware keeps its own history in the comment at the top of `Source/Version.cpp` in the Nova repo.

## Nova firmware compatibility

| Feature | Nova firmware |
| --- | --- |
| Connect, settings, mixes, analog and return inputs | any version with the Nova Touch option |
| Change AES and RX inputs on Input Config | 5.18 or later |
| Empty `cRequest` resends all settings | 5.24 or later |

On firmware older than 5.18, AES and RX inputs show their settings but changes are not sent.

## 0.92.11 - Oct 29 2026

### For users

- AES 1-8 and RX 1-8 can now be changed on the Input Config page: level, 48V, phase, delay, high-pass filter, notch, compressor and trim. Before this, changes to those inputs did nothing or changed the wrong input.
- Needs Nova firmware 5.18 or later. On older firmware those inputs are read-only. Changes made in the first couple of seconds after connecting are not sent, because NovaRemote waits for the Nova to report its firmware version.
- The version in the status line and the About box now always matches the program.

### For developers

- Nova 5.18-5.23 store per-input writes (`ApplySetting` in `Remote_PC.cpp`) without `ConvertPCchanToNova`. Packed inputs 7-22 land in `gChannelConfig` slots 6-21: AES and RX 1-2 hit dummy slots, RX 3-8 hit AES 1-6. Reads and the Nova's own echoes use the packed index and were always right.
- NovaRemote reads the firmware from the version string the Nova sends every 2 s (`<model> v5.23 sn...`) and picks the index for inputs 7-22: 5.24 and later sends the packed index, 5.18-5.23 sends packed + 10 (the `gChannelConfig` slot), older or not yet known sends nothing. The local value is always stored under the packed index. See the Input Config sink in `app.cpp`.
- `SettingStore::encodeNumber` takes an optional `wireIdx`, sent in place of `idx` without changing where the value is stored.
- Nova 5.24 converts those writes itself, so do not add 10 for it. The cutoff of 5.18 is the oldest firmware in the Nova repo history; lower it in `app.cpp` only after checking `ApplySetting` in that build.
- The Nova drops every zero-length packet, so an empty `cInit`, `cRequest`, `cFaders` or `cVersion` from the PC does nothing on 5.23 and older. The full settings dump comes from the first `PC...` version string of a new connection. Nova 5.24 also dumps on an empty `cRequest` from a connected PC, unless a dump is pending or ended less than 2 s ago. NovaRemote still sends one on connect; 5.24 skips it because of the connect dump.

## Earlier versions

Versions up to 0.92.10 were not recorded here. See `git log`.
