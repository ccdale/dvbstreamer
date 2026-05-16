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

### MRLs (Media Resource Locator)

MRLs are in the form `<transport>://<details>` and specify where output is sent.

| Transport | Usage             | Description |
| --- | --- | --- |
| udp | udp://host:port | Packets are sent via UDP (7 TS packets per UDP packet) to the specified host/port. |
| file | file://filepath | Packets are written to the specified file. |
| null | null:// | Packets are discarded. |

## Commands

- quit: Exit the program.
- lsservices: List all services or services for a specific multiplex.
- lsmuxes: List multiplexes.
- select: Select a new service to stream.
- current: Print currently streamed service.
- serviceinfo: Display information about a service.
- pids: List PIDs for a service.
- stats: Display stats for PAT/PMT/service PID filters.
- lssfs: List secondary service filters.
- setsf: Select service for a secondary service output.
- setsfmrl: Set service filter MRL.
- setsfavsonly: Enable/disable Audio/Video/Subtitles-only streaming.
- festatus: Display tuner status.
- scan: Scan multiplexes for services.
- help: List commands or help for a specific command.
- lsplugins: List loaded plugins.
- plugininfo: Display plugin information.

### quit

Usage:

```text
quit
```

Exit the program. Can be used in a startup file to stop processing.

### lsservices

Usage:

```text
lsservices [mux | <multiplex uid>]
```

Lists services in the database, services on the current mux, or services on a
specified multiplex uid.

### lsmuxes

Usage:

```text
lsmuxes
```

List all multiplexes.

### select

Usage:

```text
select <service name>
```

Sets the specified service as primary output, which may retune multiplex.

### current

Usage:

```text
current
```

Shows the currently streamed service.

### serviceinfo

Usage:

```text
serviceinfo <service name>
```

Displays running status, conditional access status, and EPG presence.

### pids

Usage:

```text
pids <service name>
```

List PIDs for the specified service.

### stats

Usage:

```text
stats
```

Display packet counts for PSI/SI filters and service/manual outputs.

### addsf

Usage:

```text
addsf <output name> <mrl>
```

Adds a secondary service destination.

### rmsf

Usage:

```text
rmsf <output name>
```

Removes a secondary service destination.

### lssfs

Usage:

```text
lssfs
```

Lists secondary service filters, destinations, and selected services.

### setsf

Usage:

```text
setsf <output name> <service name>
```

Streams the specified service to a secondary output.

### setsfmrl

Usage:

```text
setsfmrl <output name> <mrl>
```

Changes destination MRL for a service filter output.

### setsfavsonly

Usage:

```text
setsfavsonly <output name> on|off
```

When enabled, rewrites PMT to include only first video, normal audio, and subtitles.

### festatus

Usage:

```text
festatus
```

Displays lock status, BER, SNR, and signal strength.

### scan

Usage:

```text
scan <mulitplex>
```

Tunes to specified multiplex and waits 5 seconds for PAT/PMT/SDT.

### help

Usage:

```text
help [<command>]
```

Lists all commands or help for one command.

### lsplugins

Usage:

```text
lsplugins
```

Lists plugins loaded at startup.

### plugininfo

Usage:

```text
plugininfo <pluginname>
```

Displays version, author, and description for a plugin.

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

- Adam Charrett: Author
- Tero Pelander: IPv6 support, patches, DVB-C testing
- Thomas Sattler: Extensive testing for 0.5
- Nicholas Humfrey: PowerPC testing and patch
- Alex Luccisano: DVB-S testing
- Jonathan Isom: ATSC testing
- Michael Treuer: Patch to DVB text code
- Steve VanDeBogart: Several patches and traffic monitor plugin
- Ramsey Ammar: First donor
- Paul Kelly: Patch to make festatus work on DVB-S
- Louis Croisez: Help finding critical MRL bug and testing
- Chris Allison: Patch adding Series/Content IDs to xmltv output
- Marcel Ritter: Bug fix
- A C G Mennucc: Several bug fixes/patches
- Tom Albers: Test stream and testing of extended event descriptor patch
- Samuli Suominen: Patch to use system libltdl instead of in-tree copy
- Issa Gorissen: Donated TeVII S660 for DVB-S2 support
- Luca Dasseto: Donated TeVII S660 for DVB-S2 support

VideoLan libdvbpsi Team for libdvbpsi:

http://www.videolan.org/developers/libdvbpsi.html

MythTV Team for huffman decode tables for Freesat EPG.

## History

- 2.0 Next Gen Release
- 1.1 Ubuntu build fix Release
- 1.0 The first .0 Release
- 0.9 EPG Release
- 0.8 ATSC/DVB-S Release
- 0.7 Remote interface Release
- 0.6 Stability Release
- 0.5 Plugin Release
- 0.4 Daemon mode
- 0.3 Service Addition/Removal
- 0.2 Add new commands (addoutput etc)
- 0.1 First public release
