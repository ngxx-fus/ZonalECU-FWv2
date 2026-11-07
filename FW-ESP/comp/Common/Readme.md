# About

The `Common` module provides core cross-cutting definitions and fundamental types shared across the entire Zonal ECU firmware:

- **Atomic/**: Atomic operations typedefs and wrappers (`Atomic.h`).
- **Config/**: C/C++ cross-compatibility macros (`HEADER_CPP`, `FOOTER_CPP`) and argument attribute/permission macros (`RO`, `WO`, `RW`, `IN`, `OUT`, `INOUT`).
- **Error/**: Standard error codes (`ze_err_t`) mapped 1:1 with ESP-IDF and extended with ECU subsystem ranges, plus string conversion helpers.
- **SharedAPIs.h**: Umbrella header aggregating all common definitions into a single include.
