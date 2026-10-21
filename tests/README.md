# Native parser regression tests

Run from the repository root on macOS or Linux with a C++17 compiler supporting
AddressSanitizer and UndefinedBehaviorSanitizer (Clang or GCC):

```sh
./tests/run.sh
```

Set `CXX` to select a compiler executable. The shell runner creates temporary ESPHome header
stubs, replaces only the broad `navien.h` include with `navien_link.h`, and
compiles the production parser unchanged otherwise. No ESPHome installation
or heater is required. All UART writes go to a fixed-size mock buffer.
The test harness adds no dynamic allocation; the production command queue
retains its existing `std::list` allocation behavior.
The test executable has a 15-second timeout to catch stalled receive loops.

Coverage:

- All 256 declared payload lengths, both header-only and with buffered bodies.
- Rejection before a body read, notification of multiple visitors, unchanged
  queued commands, and recovery through noise and another oversized header.
- Captured water/gas packets from `src/checksum.cpp`, with both checksum seeds,
  callback payload/source checks, bad checksums, and NaviLink detection.
- Every split point in a water frame, including header and checksum splits.
- Header/body reads that fail before consuming bytes or after consuming one
  or two bytes, followed by recovery and queued-command transmission.

Cases can be run separately:

```sh
./tests/run.sh --case lengths
./tests/run.sh --case packets
./tests/run.sh --case failures
```

To compare with another checkout, use
`--source /path/to/checkout/esphome/components/navien`. Before the receive
fix, `lengths` fails on the rejection assertion and `failures` fails on the
reset/return assertion; `packets` passes. A failing assertion is a regression
result, not evidence that AddressSanitizer reproduced a device heap crash.
The mock also asserts read bounds because an overwrite between fields of
one C++ object may not be caught by AddressSanitizer alone.

These tests verify the parser and its UART/callback boundary. They do not
replace an ESPHome firmware build or live RS-485/control validation.
