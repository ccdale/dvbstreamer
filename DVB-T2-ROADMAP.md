# DVB-T2 Support Implementation Roadmap

## Overview
This document outlines the plan to extend dvbstreamer to support DVB-T2 (Digital Video Broadcasting - Terrestrial, 2nd Generation) in addition to its current DVB-T support.

## Current Status
- **DVB-T2 Delivery System Recognized:** The `DELSYS_DVBT2` enum value exists and is referenced in delivery system string arrays
- **Hardware Detection:** Partially implemented - DVB-T2 capable frontends (with `FE_CAN_2G_MODULATION` capability) are detected
- **Infrastructure:** Uses Linux DVB API v5, which fully supports DVB-T2
- **Gap:** No actual tuning implementation or parameter handling for DVB-T2

## Key Differences: DVB-T vs DVB-T2
### DVB-T Parameters
- Bandwidth: 6, 7, 8 MHz
- Guard Interval: 1/32, 1/16, 1/8, 1/4
- Transmission Mode: 2K, 8K
- FEC (Code Rates): 1/2, 2/3, 3/4, 5/6, 7/8
- Modulation: QPSK, 16-QAM, 64-QAM
- Hierarchy: NONE, 1, 2, 4 (optional)
- TPS Signalling: 17 bits in the pilot signal

### DVB-T2 Additional/Different Parameters
- Bandwidth: 1.7, 6, 7, 8, 10 MHz (plus 5 MHz for some regions)
- Guard Interval: 1/128, 19/256, 1/32, 1/16, 1/8, 1/4 (more options)
- Transmission Mode: 2K, 4K, 8K (8K is preferred for DVB-T2)
- FEC: LDPC codes with Bch outer code (different structure)
- Modulation: QPSK, 16-QAM, 64-QAM, 256-QAM
- **PLP (Physical Layer Pipe):** Critical - DVB-T2 can carry multiple data streams
- **P1 Signalling:** Replaces DVB-T's TPS signalling
- **Scattered Pilots:** Different pilot pattern (PP1-PP8)
- **Papr Reduction:** Support for active and data PAPR reduction

## Implementation Roadmap

### Phase 1: Tuning Parameter Support (Priority: HIGH)
**Goal:** Enable basic DVB-T2 tuning with fundamental parameters

#### Task 1.1: Define DVB-T2 Property Constants
**File:** `src/dvbadapter.c`
**Changes:**
- Add `#define PROP_IDX_DVBT2_BANDWIDTH` through property indices
- Create `PROP_COUNT_DVBT2` macro for total DVB-T2 properties
- Define set of required properties:
  - DTV_DELIVERY_SYSTEM = SYS_DVBT2
  - DTV_FREQUENCY
  - DTV_BANDWIDTH_HZ
  - DTV_MODULATION
  - DTV_CODE_RATE_HP (for DVB-T2 outer/inner FEC)
  - DTV_CODE_RATE_LP
  - DTV_GUARD_INTERVAL
  - DTV_TRANSMISSION_MODE
  - DTV_PLP_NUMBER (essential for multiple streams)
  - DTV_INVERSION

**Acceptance Criteria:**
- Constants are properly defined and documented
- No compilation errors

#### Task 1.2: Implement ConvertYamlToDTVProperties for DVB-T2
**File:** `src/dvbadapter.c`
**Changes:**
- Add `case DELSYS_DVBT2:` in `ConvertYamlToDTVProperties()` function (around line 1744)
- Set `DTV_DELIVERY_SYSTEM` to `SYS_DVBT2`
- Map YAML tuning parameters to DVB-T2 DTV properties:
  - Bandwidth conversion (support 1.7, 5, 6, 7, 8, 10 MHz)
  - Modulation mapping (include 256-QAM)
  - Code rate mappings
  - Guard interval (with T2-specific values)
  - Transmission mode (including 4K)
  - PLP number (default to 0 if not specified)

**Acceptance Criteria:**
- All DVB-T2 tuning parameters are correctly set in the property array
- Default values are sensible (PLP 0, AUTO for flexible parameters)
- Code compiles without errors

#### Task 1.3: Implement ConvertDTVPropertiesToYaml for DVB-T2
**File:** `src/dvbadapter.c`
**Changes:**
- Add `case DELSYS_DVBT2:` in `ConvertDTVPropertiesToYaml()` function (around line 1810)
- Read back tuned parameters from DTV properties into YAML document
- Reverse map numeric values to human-readable strings for storage

**Acceptance Criteria:**
- Tuned parameters are correctly read back from the adapter
- YAML output is consistent with input format

### Phase 2: Channel File Parsing Support (Priority: HIGH)
**Goal:** Enable reading DVB-T2 channels from scan output files

#### Task 2.1: Extend YAML Parameter Tag Definitions
**File:** `src/dvbadapter.c`
**Changes:**
- Add new parameter tags for DVB-T2-specific fields:
  - `#define TAG_PLP_NUMBER "PLP Number"`
  - Consider: `#define TAG_T2_SYSTEM "T2 System"` if relevant
  - Consider: `#define TAG_SCATTERED_PILOTS "Scattered Pilots"` for advanced tuning

**Acceptance Criteria:**
- New tags are defined and documented
- Naming is consistent with existing tags

#### Task 2.2: Add DVB-T2 Parsing in parsezap.c
**File:** `src/parsezap.c`
**Changes:**
- Add `case DELSYS_DVBT2:` block in `parsezapline()` function (after line 545)
- Parse DVB-T2 specific parameters from channel configuration
- Handle both standard scan format and VDR format variations
- Support optional PLP number extraction
- Preserve backward compatibility with DVB-T format

**Implementation Detail:** DVB-T2 channels.conf format from scan tools typically includes:
```
Channel_Name:frequency:bandwidth:modulation:transmission_mode:guard_interval:fec_hp:fec_lp:inversion:plp_id
```

**Acceptance Criteria:**
- DVB-T2 entries from scan files parse without errors
- All parameters are extracted correctly
- Missing optional parameters (like PLP) are handled gracefully

#### Task 2.3: Update Scanning Command Support
**File:** `src/commands/cmd_scanning.c`
**Changes:**
- Add `case DELSYS_DVBT2:` in scanning parameter initialization (around line 1008)
- Define scanning frequency list for DVB-T2
- Add parameter support checks for DVB-T2 specific features
- Update scan configuration to handle T2 parameters

**Acceptance Criteria:**
- Scanning can detect DVB-T2 multiplexes
- Parameter constraints are properly enforced

### Phase 3: Setup Tool Enhancement (Priority: MEDIUM)
**Goal:** Allow users to initialize DVB-T2 databases on setup

#### Task 3.1: Update setupdvbstreamer Script
**File:** `src/setup.c`
**Changes:**
- Add `-t2` flag option for DVB-T2 terrestrial setup
- Extend help text to document DVB-T2 setup
- Ensure database initialization works with `DELSYS_DVBT2`

**Acceptance Criteria:**
- `setupdvbstreamer -t2 channels.conf` correctly initializes a DVB-T2 database
- Backward compatibility with `-t` (DVB-T) is maintained

#### Task 3.2: Update Documentation
**Files:** `README`, `doc/remotecontrol.txt`
**Changes:**
- Document DVB-T2 setup procedure
- Update command examples to include DVB-T2 option
- Note any known limitations or hardware requirements

**Acceptance Criteria:**
- Users can understand how to set up and use DVB-T2
- Documentation is clear and accurate

### Phase 4: Testing & Validation (Priority: HIGH)
**Goal:** Ensure DVB-T2 support works correctly in real-world scenarios

#### Task 4.1: Create DVB-T2 Test Configuration Files
**File:** `test/dvb-t2.conf`
**Changes:**
- Create sample DVB-T2 tuning configuration files
- Document the tuning parameters used
- Include both 2K and 8K transmission modes

**Acceptance Criteria:**
- Test files can be loaded without errors
- Parameters are correct for known DVB-T2 transmitters

#### Task 4.2: Unit Testing
**Scope:** Test parameter conversion functions
- Test YAML-to-DTV property conversion for DVB-T2
- Test DTV property-to-YAML conversion round-trip
- Test bandwidth conversion with all supported values
- Test PLP number handling

**Acceptance Criteria:**
- All conversions are bidirectional and lossless
- Edge cases are handled

#### Task 4.3: Integration Testing
**Scope:** End-to-end testing with actual DVB-T2 signals (if available)
- Tune to known DVB-T2 multiplexes
- Verify stream reception
- Validate service list extraction
- Test service switching

**Acceptance Criteria:**
- Can successfully lock onto DVB-T2 signals
- Services are correctly identified and accessible
- No crashes or memory leaks

#### Task 4.4: Regression Testing
**Scope:** Ensure DVB-T support is not affected
- Run existing DVB-T scanning
- Verify DVB-T channel loading
- Test service streaming

**Acceptance Criteria:**
- All existing DVB-T functionality remains intact
- No performance degradation

### Phase 5: Advanced Features (Priority: LOW, Future Enhancement)

These features are optional and can be implemented after basic DVB-T2 support is stable:

#### 5.1: Multiple PLP Support
- Allow users to select specific PLPs from DVB-T2 multiplexes
- Add command to list available PLPs
- Support simultaneous streaming of multiple PLPs

#### 5.2: T2-MI (DVB-T2 Modulator Interface) Support
- Support for T2-MI encapsulated streams
- Useful for certain DVB-T2 implementations

#### 5.3: Auto-Detection of T2 System Parameters
- Automatically determine transmission mode, guard interval, etc.
- Improve user experience for regions with DVB-T2

#### 5.4: Monitoring and Diagnostics
- Report T2-specific signal quality metrics
- Display PLP information
- Show P1 signalling data

## Technical References

### Linux DVB API v5 Documentation
- DVB-T2 delivery system support: `SYS_DVBT2`
- DTV Property Documentation: Linux kernel DVB frontend headers
- Key properties: DTV_BANDWIDTH_HZ, DTV_MODULATION, DTV_PLP_NUMBER

### DVB-T2 Standards
- EN 302 755 (DVB-T2 Specification)
- Physical parameters for different regions vary
- Common guard intervals: 1/128, 1/32, 1/16, 1/8, 1/4
- Common transmission modes: 2K, 4K, 8K

### Scan Utility Compatibility
- linuxtv scan tool produces DVB-T2 compatible output
- Channel format compatibility with VDR and other applications

## Risk Assessment

### Low Risk
- Parameter definitions and conversions (straightforward mapping)
- Parsing from channel files (similar to existing DVB-T code)

### Medium Risk
- Edge cases in bandwidth conversion
- PLP number handling in mixed DVB-T/T2 environments
- Backward compatibility with existing configurations

### Mitigation Strategies
- Comprehensive unit tests for all conversion functions
- Extensive validation of parsed parameters
- Graceful degradation for unsupported features
- Clear error messages for invalid configurations

## Success Criteria

1. ✅ DVB-T2 hardware is properly detected and enumerated
2. ✅ Users can initialize DVB-T2 databases with setupdvbstreamer
3. ✅ DVB-T2 channels can be scanned and stored
4. ✅ Services from DVB-T2 multiplexes can be tuned and streamed
5. ✅ All existing DVB-T functionality remains unchanged
6. ✅ No crashes or memory leaks
7. ✅ Documentation is complete and accurate
8. ✅ Code compiles with no warnings

## Timeline Estimate

| Phase | Tasks | Estimated Duration |
|-------|-------|-------------------|
| 1 | Tuning parameter support | 2-3 days |
| 2 | Channel file parsing | 1-2 days |
| 3 | Setup tool enhancement | 1 day |
| 4 | Testing & validation | 3-5 days (depends on hardware availability) |
| 5 | Advanced features | TBD |

**Total Estimated Time:** 7-11 days for core DVB-T2 support (Phases 1-4)

## Notes

- This roadmap assumes Linux DVB API v5 is available in the build environment
- Hardware with DVB-T2 capability is required for practical testing
- The implementation should maintain code style and architecture consistency with existing dvbstreamer code
- Consider contributing improvements back to the dvbstreamer project or documenting any divergences
- Post-release housekeeping: modernize autotools metadata (`configure.in` -> `configure.ac`) and remove deprecated macro usage (for example `STDC_HEADERS`) once the release is validated
