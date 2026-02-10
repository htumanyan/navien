# AGENTS.md — Navien ESPHome Integration

## What this repository is
This repository provides:
- An ESPHome external component for local control and telemetry of Navien water heaters via RS-485
- Reverse-engineered protocol documentation, traces, and message decoding notes
- Optional hardware reference materials for DIY controller builds

Primary goal: reliable, local Home Assistant integration without proprietary cloud dependencies.

The root README is the source of truth for end-user setup and supported configurations.

## Scope & boundaries
- Prefer **backwards-compatible** changes; avoid breaking existing YAML keys, entity names, or IDs.
- Protocol changes must be documented under `doc/` with examples.
- Do **not** add cloud-based features or proprietary APIs.
- **Safety first**: never recommend wiring to unknown or high-voltage pins; explicitly warn when a pin is known to be ~14V or otherwise unsafe.

## Repository layout
- `esphome/` — ESPHome external component and example YAMLs
- `doc/` — protocol reverse-engineering notes and field documentation
- `trace/rs485/` — RS-485 captures used for validation
- `hardware/` — PCB, connector, and harness reference designs
- `src/` — implementation details (protocol parsing, entities)

## Supported workflows

### Build / compile
Follow the workflow in the root README:
- Install ESPHome (`pip install esphome`)
- Copy secrets (`cp secrets.yaml.sample secrets.yaml`)
- Compile: `esphome compile <config>.yml`
- Flash/run: `esphome run <config>.yml`

ESPHome versions can introduce breaking changes; follow README version guidance.

### Runtime validation
When modifying protocol parsing or entity behavior:
- Enable DEBUG logging and capture at least one full request/response cycle
- Verify:
  - Stable RS-485 connection
  - Sensors update correctly
  - Controls (power, hot button, target temp) still work on known-good models

### Protocol decode changes (required checklist)
If protocol parsing changes:
1. Update documentation in `doc/` (byte offsets, scaling, meaning)
2. Include at least one example frame (raw bytes + decoded values)
3. Document model-specific behavior (e.g., NPE vs NCB)
4. Preserve unknown bytes or log them; do not silently drop data

## C++ conventions & formatting

### Indentation / whitespace (required)
- **Spaces only. No tabs.**
- Use **2 spaces** for indentation.
- Strip trailing whitespace.
- End all files with a newline.

### Style
- Favor readability over cleverness.
- Keep functions short and single-purpose.
- Use `const` where applicable; pass large objects by `const&`.
- Prefer `enum class` for new enums.
- Avoid macros unless required by ESPHome.

### Memory & dependencies (embedded best practices)
- **Avoid the C++ standard library** (`std::vector`, `std::map`, `std::string`, streams, etc.).
- **Do not use dynamic allocation**:
  - No `malloc` / `free`
  - No `new` / `delete`
- Prefer **statically allocated or stack-allocated C data structures**:
  - Fixed-size arrays and ring buffers
  - POD `struct`s with explicit sizes
  - Preallocated buffers with strict bounds checks
- If dynamic allocation is absolutely unavoidable:
  - Justify it clearly in the PR
  - Ensure it never occurs in `loop()` or hot paths
  - Document ownership and lifetime explicitly

### Safety & performance
- Never block in hot paths (`loop()`, callbacks).
- Avoid long delays or busy waits.
- Minimize allocations and heavy string operations.
- Treat all inbound RS-485 data as untrusted:
  - Validate lengths before indexing
  - Verify checksums/CRCs if available

### Logging
- Use ESPHome logging macros (`ESP_LOGD`, `ESP_LOGI`, `ESP_LOGW`).
- Log unknown or unsupported frames at DEBUG with:
  - Message/opcode (if known)
  - Raw bytes (trimmed if large)
- Avoid log spam in steady state; throttle repeated warnings.

### Protocol parsing conventions
- Centralize parsing logic; do not duplicate byte-offset logic.
- Keep conversion math close to decode logic and comment units.
- Isolate model-specific branching and document it.

## Adding a new exposed metric (recommended flow)
1. Identify the field in traces (`trace/rs485/`)
2. Document it in `doc/` (offsets, scaling, examples)
3. Implement decode + conversion
4. Expose via ESPHome entity
5. Update README or example YAML if user-visible

## Communication & contributions
- When handling issues/PRs:
  - Ask for model info and PCB/front-panel photos when relevant
  - Request RS-485 traces and ESPHome logs
  - Avoid speculative electrical advice; call out unverified assumptions
- Before pushing commits to any remote branch, obtain explicit user approval.

## What not to do
- Do not recommend bypassing safety systems or modifying heater internals.
- Do not introduce breaking renames without strong justification and migration notes.
- Do not add “magic” auto-detection that can misconfigure pins or voltage assumptions.
