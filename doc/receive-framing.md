# Receive framing and buffer bounds

The ESPHome parser uses a 128-byte receive buffer. The header is six bytes;
the byte at offset 5 declares the payload length, excluding the header and
the one-byte checksum. Total frame size is therefore `6 + payload_length + 1`.
The largest payload that fits this buffer is 121 bytes.

For example, this malformed header declares a 122-byte payload:

```text
f7 05 0f 50 00 7a
```

`f7` is the marker, `05` the system type, `0f` the source, `50` the
destination, `00` an unsupported direction, and `7a` the payload length.
The required 129 bytes exceed the receive buffer. The parser rejects the
header before reading its body, regardless of the direction. A length byte
of `ff` is also rejected before adding the checksum length in an 8-bit type
could wrap to zero. The length check uses `size_t` before narrowing to the
UART interface's 8-bit read length.

On rejection, registered visitors receive a communication error and the
parser resumes searching for `f7`. It does not wait for the untrusted length
to arrive. Remaining bytes are scanned as potential packet starts; an
embedded marker can be mistaken for a new header. This is marker-based
resynchronization, not a guarantee of recovering the immediately next frame
from arbitrary noise. No queued command is consumed or transmitted for the
rejected header.

A failed header or body read resets the parser and returns to its caller.
The next call searches for a fresh marker instead of interpreting bytes
using a header that may have been partially overwritten or consumed.
An incomplete packet whose UART read has not started waits for more data.

The bound applies to all system types and source addresses. It describes
this implementation's storage capacity, not a protocol maximum for every
Navien model. No NPE/NCB-specific decoding, checksum selection, or command
timing is changed. Packet-type-specific minimum lengths and general noise
recovery are outside this change.

The native regression suite uses the water/gas captures already recorded in
`src/checksum.cpp`, including cascade packets with source `51`. For example,
this complete water frame has a 34-byte payload and checksum `65`:

```text
f7 05 50 50 90 22 42 00 00 25 14 56 49 49 00 00 00 00 00 00
88 c2 00 20 02 00 00 00 21 03 99 08 00 02 00 00 00 00 00 00 65
```

It routes to the water-status callback with source `50`. The DHW setpoint at
offset 11 is `56` (86 half-degrees, or 43 °C); the inlet and outlet values are
both `49` (36.5 °C). See [the test instructions](../tests/README.md) to run
the parser against these captures and malformed inputs without hardware.
