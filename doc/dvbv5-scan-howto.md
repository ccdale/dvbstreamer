# DVBv5 Scan File Creation Guide

This guide shows how to create modern scan files with dvbv5 tools and use them with dvbstreamer.

## Scope

- Create or obtain an initial scan file.
- Run dvbv5-scan to discover multiplexes and services.
- Export in formats useful for tooling and migration.
- Use the resulting data with dvbstreamer workflows.

## Tooling

Install the Linux DVBv5 utilities package (often named dvb-tools or v4l-utils, depending on distro).

Main command used here:

- dvbv5-scan

Reference used for this guide:

- man page summary: https://www.mankier.com/1/dvbv5-scan

## Input Files: Initial Tuning Data

dvbv5-scan starts from an initial frequency list file.

Common locations (distro-dependent):

- /usr/share/dvbv5/
- /usr/share/dvb/

Examples by delivery type:

- DVB-T/T2: country or region files under terrestrial folders
- DVB-C: provider or region files
- DVB-S/S2: satellite files

If your distro file set is missing or stale, generate/collect a modern initial file from current regional transmitter data, then keep it in your project assets.

## Core Scanning Commands

### 1) Create a modern dvbv5 output file

dvbv5 native format preserves the richest metadata.

```bash
dvbv5-scan -a 0 -f 0 -d 0 -N -o dvb_channel.conf /usr/share/dvbv5/<initial-file>
```

Notes:

- -N uses NIT-announced parameters when available.
- Output defaults to dvbv5 format.

### 2) Keep only frequencies from the input file

Useful if broadcaster NIT data is noisy or expands scan too broadly.

```bash
dvbv5-scan -F -o dvb_channel.conf /usr/share/dvbv5/<initial-file>
```

### 3) Ask frontend for locked parameters

Useful when moving configs between devices/chipsets.

```bash
dvbv5-scan -G -N -o dvb_channel.conf /usr/share/dvbv5/<initial-file>
```

### 4) Produce legacy format when needed

For compatibility with older tooling:

```bash
dvbv5-scan -O zap -o channels.zap /usr/share/dvbv5/<initial-file>
```

Other legacy output options include channel and vdr.

## DVB-T2 Practical Notes

For DVB-T2, ensure your initial file and scan include:

- Frequency
- Bandwidth
- Modulation
- Transmission mode
- Guard interval
- FEC values
- Optional PLP identifier

If services are missing, retry with:

- higher wait factor: -T <factor>
- extra verbosity: -v or -vv
- explicit adapter/frontend selection: -a and -f

Example:

```bash
dvbv5-scan -a 0 -f 0 -N -T 2 -vv -o dvb_t2.conf /usr/share/dvbv5/<t2-initial-file>
```

## Using Results with dvbstreamer

dvbstreamer supports network scan seed strings. For DVB-T2 in this repo, the accepted scan net shape is:

```text
T2 <freq> <bw> <mod> <transmission-mode> <guard-interval> <fec-hp> <fec-lp> <inversion> [plp]
```

Example:

```text
T2 490000000 8MHz 64QAM 8k 1/32 2/3 1/2 AUTO 0
```

Implementation points in this repo:

- parsezap DVB-T2 handling: src/parsezap.c
- scan net DVB-T2 parsing: src/commands/cmd_scanning.c

## Troubleshooting

- No lock:
  - check antenna signal path and frontend capabilities
  - confirm delivery system (T vs T2, C, S)
  - increase wait time with -T
- Incomplete service discovery:
  - rerun with -N and without -F
  - use -p for other NIT/SDT on some DVB-C networks
- Wrong charset/service names:
  - set country with -C <ISO-3166-1 two-letter code>

## Recommended Workflow for Repeatable Scans

1. Keep a versioned copy of initial tuning files per region/provider.
2. Scan to dvbv5 output first.
3. Validate lock and service count.
4. Export additional legacy formats only when required.
5. Store command lines used for reproducibility.
