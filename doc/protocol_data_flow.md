# Navien Protocol Data Flow (RS-485)

This document summarizes observed and inferred data-flow rules for Navien packet headers.

## Header Field Semantics

### Source (`src`)

| Field | Value | Role / Semantics | Where It Is Used |
|---|---:|---|---|
| `src` | `0x01` | Peer/panel endpoint used by observed handshake/control-init traffic | Header comments and observed traffic |
| `src` | `0x0F` | Local gateway/controller ID (outbound sender) | Outbound builders set `src=0x0F` |
| `src` | `0x50` | Primary boiler telemetry source | Main parser status path, CRC seed `0x4B` branch |
| `src` | `0x50..0x5F` | Cascade unit source range (`0x50` base + unit index) | `Navien::on_water/on_gas` filter uses `PACKET_SRC_STATUS + src_`; Navien uses `0x50` and above for cascaded units (second, third, ... up to 15 units) |

### Destination (`dst`)

| Field | Value | Role / Semantics | Where It Is Used |
|---|---:|---|---|
| `dst` | `0x01` | Handshake target peer | Handshake builder sets `dst=0x01` |
| `dst` | `0x0F` | Local gateway/controller destination | Parser accepts for status import |
| `dst` | `0x50` | Main boiler destination path | Parser accepts for status import |
| `dst` | other values | Not imported into water/gas status callbacks | `parse_status_packet()` switches only on `0x50`/`0x0F` |

## Related Fields

| Related Field | Value | Semantics |
|---|---:|---|
| `direction` | `0x10` | Control packet path |
| `direction` | `0x90` | Status packet path |
| parser gate | `direction == 0x90` | Required for status packet parsing |
| status apply gate | `src == 0x50 + src_` | Required per-instance for water/gas state apply |

## Observed / Inferred Flow Patterns

| Src | Dst | Direction | Meaning | Status |
|---:|---:|---:|---|---|
| `0x0F` | `0x01` | `0x10` | Local gateway sends handshake/control-init packet to peer/panel endpoint | Observed |
| `0x01` | `0x0F` (likely) | `0x10` | Peer/panel-origin packet path | Inferred from comments and observed traffic |
| `0x0F` | `0x50` | `0x10` | Local gateway sends control requests toward main boiler node | Observed (command templates/builders) |
| `0x50` | `0x0F` | `0x10` or `0x90` | Main boiler telemetry/status accepted by parser for local import | Observed |
| `0x50` | `0x50` | `0x10` or `0x90` | Main-node loop/forwarded status path still accepted by parser | Observed |
| `0x50..0x5F` | `0x50` or `0x0F` | `0x90` | Cascade unit source range; per-instance apply uses `src-0x50` mapping | Inferred from instance filtering |

## Validation Rules

| Validation Rule | Semantics |
|---|---|
| `direction == 0x90` | Status CRC/parse path |
| `direction == 0x10 and src == 0x0F` | Control CRC/parse path |
| `direction == 0x90 and src == 0x50` | Status CRC uses seed `0x4B` |
| `direction == 0x90 and src != 0x50` | Status CRC uses seed `0x62` |
| `direction == 0x90 and dst in {0x50, 0x0F}` | Status segment routed to water/gas callbacks |
| `src == 0x50 + src_` (in Navien instance) | Water/gas state applied for matching unit only |
