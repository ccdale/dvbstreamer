# DVB-T2 Validation Test Plan

This plan is for running on a machine that has a working DVB adapter and signal source.

## Goal

Validate the recently added DVB-T2 work:

- setupdvbstreamer `-2` import path
- DVB-T2 parsezap handling
- DVB-T2 network scan input parsing (`scan net "T2 ..."`)
- no regression to existing DVB-T behavior

## Scope

In scope:

- Build/install sanity on target machine
- Functional checks for DVB-T2 setup and tuning
- Service discovery and selection checks
- Basic streaming smoke test
- DVB-T regression checks

Out of scope:

- Long-duration reliability burn-in
- Performance benchmarking
- Advanced multi-PLP features (future roadmap)

## Test Environment

Record before running:

- Hostname:
- OS/version:
- Kernel version (`uname -r`):
- DVB adapter model/chipset:
- Frontend delivery systems supported:
- Antenna/source details:
- Signal region/transmitters:

Useful commands:

```bash
uname -a
ls /dev/dvb
for f in /sys/class/dvb/dvb*.frontend*/device/uevent; do echo "=== $f"; cat "$f"; done
```

## Required Inputs

1. A known-good DVB-T2 channels file (`channels_t2.conf`) for your region.
2. A known-good DVB-T channels file (`channels_t.conf`) for regression.
3. At least one known DVB-T2 service name expected to be present.

Optional but recommended:

- One initial tuning line for `scan net` testing, format:

```text
T2 <freq> <bw> <mod> <transmission-mode> <guard-interval> <fec-hp> <fec-lp> <inversion> [plp]
```

Example:

```text
T2 490000000 8MHz 64QAM 8k 1/32 2/3 1/2 AUTO 0
```

## Pre-Flight

1. Clean build tree.
2. Build from current `main`.
3. Confirm binaries exist.

```bash
cd /path/to/dvbstreamer-2.1.0
make clean
./configure
make -j"$(nproc)"
ls -l src/dvbstreamer src/setupdvbstreamer
```

Pass criteria:

- Build completes with exit code 0.
- `src/dvbstreamer` and `src/setupdvbstreamer` are present.

## Git Update Hygiene (Before `git pull`)

On test/build machines, local autotools runs may modify generated files (especially `configure`).
If `git pull` fails with "local changes would be overwritten", use one of the flows below.

Discard local generated changes:

```bash
git status
git restore configure
git clean -fd
git pull --ff-only
```

Keep local changes temporarily:

```bash
git stash push -u
git pull --ff-only
git stash pop
```

Note:

- Prefer the discard flow for generated artifacts on dedicated build machines.

## Test Cases

### TC1: Setup Import for DVB-T2 (`-2` flag)

Steps:

```bash
rm -f ~/.dvbstreamer/adapter0.db
src/setupdvbstreamer -a 0 -2 /path/to/channels_t2.conf
```

Expected:

- Command exits successfully.
- Output includes non-zero service/multiplex counts.
- No parse syntax errors.

Record:

- Services count:
- Multiplex count:
- Any warnings/errors:

### TC2: Basic Runtime Startup with DVB-T2 DB

Steps:

```bash
src/dvbstreamer -a 0 -o udp://127.0.0.1:1234
```

At prompt:

```text
lsmuxes
lsservices
```

Expected:

- App starts cleanly.
- Multiplexes and services list without errors.

### TC3: Tune and Lock DVB-T2 Service

At prompt:

```text
select "<known_dvb_t2_service>"
festatus
stats
```

Expected:

- `select` succeeds.
- `festatus` indicates lock.
- `stats` packet counters increase.

Record:

- Service selected:
- Lock state:
- Signal metrics shown:

### TC4: Network Scan Parsing for T2 Seed (`scan net "T2 ..."`)

At prompt:

```text
scan net "T2 <freq> <bw> <mod> <transmission-mode> <guard-interval> <fec-hp> <fec-lp> <inversion> [plp]"
```

Expected:

- Command parses successfully (no parse error message).
- Scan starts and reports progress/events.
- If signal is present, services/multiplexes discovered or updated.

Negative check:

- Run with clearly malformed line and verify graceful parse failure (no crash).

### TC5: Optional PLP Behavior

Run TC4 twice using same mux:

1. With explicit PLP (for example `... AUTO 0`).
2. Without PLP field (if your test input path supports omission).

Expected:

- Explicit PLP path parses and tunes.
- Missing PLP handled safely (default behavior, no crash).

### TC6: Regression - DVB-T Setup and Tune Still Works

Steps:

```bash
rm -f ~/.dvbstreamer/adapter0.db
src/setupdvbstreamer -a 0 -t /path/to/channels_t.conf
src/dvbstreamer -a 0 -o udp://127.0.0.1:1234
```

At prompt:

```text
select "<known_dvb_t_service>"
festatus
stats
```

Expected:

- DVB-T import still works.
- DVB-T service selection/lock works.
- No behavior regressions compared to baseline.

### TC7: Log and Error Handling Sanity

Run setup with invalid path and malformed file.

Expected:

- Clear error messages.
- Non-zero exit for invalid inputs.
- No crash.

## Pass/Fail Summary

Mark each test case:

- TC1: PASS/FAIL
- TC2: PASS/FAIL
- TC3: PASS/FAIL
- TC4: PASS/FAIL
- TC5: PASS/FAIL/NOT-APPLICABLE
- TC6: PASS/FAIL
- TC7: PASS/FAIL

Overall release recommendation:

- GO if TC1/TC2/TC3/TC4/TC6 all pass and no crashers.
- NO-GO otherwise.

## Artifacts To Keep

- `setupdvbstreamer` command lines used
- sample `scan net` seed lines used
- terminal logs for failures
- exact commit hash under test

Capture commit hash:

```bash
git rev-parse --short HEAD
```

## Notes for Follow-Up

If failures are found, include:

- exact command
- full error output
- adapter/frontend details
- whether issue reproduces after restart
