# Copilot Instructions (Navien ESPHome / RS-485)

Follow the repository playbook in `AGENTS.md` (source of truth).

Key rules:
- Spaces only (2 spaces). No tabs.
- Avoid STL and dynamic allocation: no `std::*`, no `malloc/free`, no `new/delete`. Prefer static/stack C-style buffers/structs.
- Protocol changes require updates in `doc/` + at least one example frame and expected decoded output.
- Do not provide speculative/unsafe hardware wiring advice; avoid unknown/high-voltage pins.

How to validate changes:
- Build/compile with ESPHome per README.
- For protocol/entity changes: provide DEBUG logs and a trace sample if available.
