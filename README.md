# DVBStreamer

## About

DVBStreamer is a console-based application to stream an entire DVB service over UDP.
It differs from other DVB streaming applications by using DVB-SI to determine the PIDs
to stream and to correctly stream services that use a separate PCR PID.

## Requirements

DVBStreamer requires the following libraries:

- sqlite3 (>= 3.3)
- readline
- libev
- libltdl (>= 2.2.6b)
- libyaml

## Debian/Ubuntu Build Dependencies

On Debian-based systems, install build tools and development headers with:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  autoconf \
  automake \
  libtool \
  pkg-config \
  libev4 \
  libev-dev \
  zlib1g-dev \
  sqlite3 \
  libsqlite3-dev \
  libreadline-dev \
  libltdl-dev \
  libyaml-dev
```

Optional useful tools:

- dvb-tools (for dvbv5-scan and related utilities)
- v4l-utils (often contains DVB utilities on some releases)

## Installation

After installing the dependencies and extracting the source, run configure to set up
the makefiles. Add any options you would normally pass to configure.

```bash
./configure
make
make install
```

## Setup

Before you can use dvbstreamer, initialize its service database using a channels.conf
file produced by scan tools.

Note: `setupdvbstreamer` expects legacy zap/VDR style channels files, not the native
section-based dvbv5 output format. If you scan with `dvbv5-scan`, export as:

```bash
dvbv5-scan -O zap -o channels.conf <initial-file>
```

or:

```bash
dvbv5-scan -O vdr -o channels.conf <initial-file>
```

When you have a channels.conf file, run setupdvbstreamer with the file you created.

DVB-T:

```bash
setupdvbstreamer -t channels.conf
```

DVB-T2:

```bash
setupdvbstreamer -2 channels.conf
```

DVB-S:

```bash
setupdvbstreamer -s channels.conf
```

DVB-C:

```bash
setupdvbstreamer -c channels.conf
```

If you are setting up an adapter other than 0, add the adapter option:

```text
-a <adapter number>
```

If you have 2 or more cards of the same type, you can hard-link the first card's
database to the second in ~/.dvbstreamer.

Example:

```bash
ln ~/.dvbstreamer/adapter1.db ~/.dvbstreamer/adapter2.db
```

## Running

- Starting DVBStreamer
- Using a startup file
- MRLs

### Starting DVBStreamer

To start dvbstreamer, specify an IP address and UDP port where the selected service
will be streamed. You can optionally specify which adapter to use with -a.

Example:

```bash
dvbstreamer -a 2 -o udp://192.168.1.1:1234
```

To select a service to stream:

```text
select "BBC ONE"
```

This tunes to the multiplex containing BBC ONE and streams packets for that service.

Use stats to verify packets are being processed and sent to the selected destination.

### Startup files

Startup files are useful for actions you always want set before reaching the prompt.

To use a startup file, add -f and specify the file:

```bash
dvbstreamer -o udp://localhost:1234 -f eitredirect
```

Example startup file content:

```text
# Redirect EIT tables to port 1235 on localhost
addmf eitoutput udp://localhost:1235
addmfpid eitoutput 0x12
```

### Remote control authentication

`dvbstreamer` and `dvbctrl` load credentials from (in priority order):

1. **`dvbctrl` only**: explicit `-u <username>` and `-p <password>` flags — useful when controlling a remote host that has different credentials.
2. `$HOME/.config/dvbstreamer/userconfig.json` on the machine running the tool.
3. Built-in defaults: `dvbstreamer` / `control`.

Expected config format:

```json
{
  "username": "dvbstreamer",
  "password": "control"
}
```

`setupdvbstreamer` creates this file on the server host after importing channels.

If the file is missing, malformed, or has insecure permissions, both tools
fall back to the built-in defaults and log a warning.

#### Controlling a remote dvbstreamer host

If you run `dvbctrl` on a different machine to the `dvbstreamer` host, pass the
credentials explicitly since the remote machine does not have the server's config file:

```bash
dvbctrl -h <host> -u dvbstreamer -p control <command>
```

Alternatively, copy the server's config file to the remote machine **preserving
permissions** (the file must be `0600` — owner-read/write only; a group- or
world-readable copy will be rejected):

```bash
scp ~/.config/dvbstreamer/userconfig.json remote:.config/dvbstreamer/userconfig.json
# or after a plain cp:
chmod 600 ~/.config/dvbstreamer/userconfig.json
```

### MRLs (Media Resource Locator)

MRLs are in the form `<transport>://<details>` and specify where output is sent.

| Transport | Usage             | Description |
| --- | --- | --- |
| udp | udp://host:port | Packets are sent via UDP (7 TS packets per UDP packet) to the specified host/port. |
| file | file://filepath | Packets are written to the specified file. |
| null | null:// | Packets are discarded. |

Important:

- `udp://` and `file://` handlers are provided by plugins.
- If `setmrl` or `setsfmrl` reports MRL parse failures for valid values, run `make install` so plugins are installed to the configured plugin directory.

## Commands

- quit: Exit the program.
- lsservices: List all services or services for a specific multiplex.
- lsmuxes: List multiplexes.
- select: Select a new service to stream.
- current: Print currently streamed service.
- serviceinfo: Display information about a service.
- pids: List PIDs for a service.
- stats: Display stats for PAT/PMT/service PID filters.
- setmrl: Set MRL for the primary service filter.
- getmrl: Get MRL for the primary service filter.
- addsf: Add a secondary service filter output.
- rmsf: Remove a secondary service filter output.
- lssfs: List secondary service filters.
- setsf: Select service for a secondary service output.
- getsf: Show selected service for a secondary service output.
- setsfmrl: Set service filter MRL.
- getsfmrl: Get service filter MRL.
- setsfavsonly: Enable or disable Audio/Video/Subtitles-only streaming.
- getsfavsonly: Show Audio/Video/Subtitles-only mode state.
- festatus: Display tuner status.
- scan: Scan multiplexes for services.
- help: List commands or help for a specific command.
- lsplugins: List loaded plugins.
- plugininfo: Display plugin information.

Detailed command usage and per-command help text are now in:

- `doc/command-reference.md`

## Known Issues

- `setupdvbstreamer` reports syntax errors for malformed channels files, but may still exit with status `0` for some malformed-input cases.
- Missing or unreadable channels files correctly return a non-zero exit status.

## Plugins

Plugins allow DVBStreamer to be extended beyond basic filtering and streaming.

Plugins can:

- add new delivery methods (see fileoutput.c for a simple example)
- add new commands
- add new PID filters (for example, EPG extraction)
- detect service/multiplex changes

Use lsplugins and plugininfo to inspect loaded plugins.

Interface documentation can be generated with doxygen using doxygen.config in
the project root.

## Help / More Information

See the wiki:

http://dvbstreamer.sourceforge.net/mediawiki/index.php/Main_Page

## Bugs

Please report bugs to SourceForge trackers:

http://sourceforge.net/tracker/?group_id=164687&atid=832723

## Credits

See [CONTRIBUTORS.md](CONTRIBUTORS.md) for project credits and attribution.

## History

See [doc/history.md](doc/history.md) for release history.
