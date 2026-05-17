# DVBStreamer Command Reference

This document contains detailed usage/help text for core interactive commands.

## quit

Usage:

```text
quit
```

Exit the program. Can be used in a startup file to stop processing.

## lsservices

Usage:

```text
lsservices [mux | <multiplex uid>]
```

Lists services in the database, services on the current mux, or services on a
specified multiplex uid.

## lsmuxes

Usage:

```text
lsmuxes
```

List all multiplexes.

## select

Usage:

```text
select <service name>
```

Sets the specified service as primary output, which may retune multiplex.

## current

Usage:

```text
current
```

Shows the currently streamed service.

## serviceinfo

Usage:

```text
serviceinfo <service name>
```

Displays running status, conditional access status, and EPG presence.

## pids

Usage:

```text
pids <service name>
```

List PIDs for the specified service.

## stats

Usage:

```text
stats
```

Display packet counts for PSI/SI filters and service/manual outputs.

## addsf

Usage:

```text
addsf <output name> <mrl>
```

Adds a secondary service destination.

## rmsf

Usage:

```text
rmsf <output name>
```

Removes a secondary service destination.

## lssfs

Usage:

```text
lssfs
```

Lists secondary service filters, destinations, and selected services.

## setsf

Usage:

```text
setsf <output name> <service name>
```

Streams the specified service to a secondary output.

## setsfmrl

Usage:

```text
setsfmrl <output name> <mrl>
```

Changes destination MRL for a service filter output.

## setsfavsonly

Usage:

```text
setsfavsonly <output name> on|off
```

When enabled, rewrites PMT to include only first video, normal audio, and subtitles.

## festatus

Usage:

```text
festatus
```

Displays lock status, BER, SNR, and signal strength.

## scan

Usage:

```text
scan <multiplex>
```

Tunes to specified multiplex and waits 5 seconds for PAT/PMT/SDT.

## help

Usage:

```text
help [<command>]
```

Lists all commands or help for one command.

## lsplugins

Usage:

```text
lsplugins
```

Lists plugins loaded at startup.

## plugininfo

Usage:

```text
plugininfo <pluginname>
```

Displays version, author, and description for a plugin.