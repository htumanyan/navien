#include <cassert>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "navien_link.h"
#include "frames.h"

using namespace esphome::navien;

struct Uart : NavienUartI {
  uint8_t input[1024]{};
  size_t size = 0;
  size_t offset = 0;
  unsigned reads = 0;
  unsigned writes = 0;
  unsigned fail_on_read = 0;
  uint8_t consume_on_failure = 0;
  uint8_t last_write[64]{};
  uint8_t last_write_size = 0;

  int available() override { return static_cast<int>(size - offset); }
  uint8_t peek_byte(uint8_t *b) override {
    if (!available()) return false;
    *b = input[offset];
    return true;
  }
  uint8_t read_byte(uint8_t *b) override {
    if (!peek_byte(b)) return false;
    ++offset;
    return true;
  }
  bool read_array(uint8_t *b, uint8_t len) override {
    ++reads;
    // The parser reads either a six-byte header or at most 122 body/checksum bytes.
    assert(len <= 128 - HDR_SIZE);
    assert(available() >= len);
    if (reads == fail_on_read) {
      assert(consume_on_failure <= len);
      memcpy(b, input + offset, consume_on_failure);
      offset += consume_on_failure;
      return false;
    }
    memcpy(b, input + offset, len);
    offset += len;
    return true;
  }
  void write_array(const uint8_t *b, uint8_t len) override {
    assert(len <= sizeof(last_write));
    memcpy(last_write, b, len);
    last_write_size = len;
    ++writes;
  }
  void append(const uint8_t *data, size_t len) {
    assert(size + len <= sizeof(input));
    memcpy(input + size, data, len);
    size += len;
  }
  void header(uint8_t length) {
    // Unknown direction isolates framing from status/control decoding.
    const uint8_t bytes[] = {0xf7, 0x05, 0x0f, 0x50, 0x00, length};
    append(bytes, sizeof(bytes));
  }
};

struct Visitor : NavienLinkVisitorI {
  unsigned water_count = 0;
  unsigned gas_count = 0;
  unsigned errors = 0;
  uint8_t water_src = 0;
  uint8_t gas_src = 0;
  WATER_DATA water{};
  GAS_DATA gas{};
  void on_water(const WATER_DATA &value, uint8_t src) override {
    water = value;
    water_src = src;
    ++water_count;
  }
  void on_gas(const GAS_DATA &value, uint8_t src) override {
    gas = value;
    gas_src = src;
    ++gas_count;
  }
  void on_error() override { ++errors; }
};

struct Link : NavienLink {
  using NavienLink::NavienLink;
  size_t queued() const { return cmd_buffer.size(); }
  READ_STATE state() const { return recv_state; }
};

static void assert_turn_on(const Uart &uart) {
  assert(uart.last_write_size == sizeof(TURN_ON_CMD));
  assert(memcmp(uart.last_write, TURN_ON_CMD, sizeof(TURN_ON_CMD)) == 0);
}

static void test_lengths() {
  for (unsigned length = 0; length <= 255; ++length) {
    // Exercise both a header on its own and a complete buffered body.
    for (unsigned buffered = 0; buffered <= 1; ++buffered) {
      Uart uart;
      Link link(&uart);
      Visitor first, second;
      link.add_visitor(&first);
      link.add_visitor(&second, 15);
      link.send_turn_on_cmd();
      uart.header(length);
      const uint8_t body[256]{};
      if (buffered) uart.append(body, length + 1);
      link.receive();
      if (length <= 121) {
        assert(first.errors == 0 && second.errors == 0);
        if (!buffered) {
          assert(link.state() == HEADER_PARSED && uart.writes == 0);
          uart.append(body, length + 1);
          link.receive();
        }
        assert(link.state() == INITIAL && uart.available() == 0);
        assert(link.queued() == 1 && uart.writes == 1 && uart.reads == 2);
        assert_turn_on(uart);
      } else {
        assert(link.state() == INITIAL && uart.available() == 0);
        assert(link.queued() == 2 && uart.writes == 0 && uart.reads == 1);
        assert(first.errors == 1 && second.errors == 1);
        uart.append(WATER_FRAME, sizeof(WATER_FRAME));
        link.receive();
        assert(link.state() == INITIAL && link.queued() == 1 && uart.writes == 1);
        assert(first.water_count == 1 && second.water_count == 1);
        assert(first.errors == 1 && second.errors == 1);
        assert_turn_on(uart);
      }
    }
  }
  // Reject another oversized marker inside discarded noise, then find a valid packet.
  Uart uart;
  Link link(&uart);
  Visitor visitor;
  link.add_visitor(&visitor);
  link.send_turn_on_cmd();
  uart.header(122);
  const uint8_t noise[] = {0x01, 0x02, 0x03};
  uart.append(noise, sizeof(noise));
  uart.header(255);
  uart.append(noise, sizeof(noise));
  uart.append(WATER_FRAME, sizeof(WATER_FRAME));
  link.receive();
  assert(visitor.errors == 2 && visitor.water_count == 1);
  assert(link.queued() == 1 && uart.writes == 1 && uart.available() == 0);
}

static void test_packets() {
  Uart uart;
  Link link(&uart);
  Visitor visitor;
  link.add_visitor(&visitor);
  uart.append(WATER_FRAME, sizeof(WATER_FRAME));
  uart.append(GAS_FRAME, sizeof(GAS_FRAME));
  link.receive();
  assert(visitor.water_count == 1 && visitor.gas_count == 1 && visitor.errors == 0);
  assert(visitor.water_src == 0x50 && visitor.gas_src == 0x50);
  assert(memcmp(&visitor.water, WATER_FRAME + HDR_SIZE, sizeof(WATER_DATA)) == 0);
  assert(memcmp(&visitor.gas, GAS_FRAME + HDR_SIZE, sizeof(GAS_DATA)) == 0);
  assert(visitor.water.dhw_set_temp == 0x56 && visitor.gas.current_gas_lo == 0x74);
  assert(uart.writes == 2 && uart.last_write_size == sizeof(NAVILINK_PRESENT));
  assert(memcmp(uart.last_write, NAVILINK_PRESENT, sizeof(NAVILINK_PRESENT)) == 0);

  uart.append(CASCADE_WATER_FRAME, sizeof(CASCADE_WATER_FRAME));
  uart.append(CASCADE_GAS_FRAME, sizeof(CASCADE_GAS_FRAME));
  link.receive();
  assert(visitor.water_count == 2 && visitor.gas_count == 2 && visitor.errors == 0);
  assert(visitor.water_src == 0x51 && visitor.gas_src == 0x51);
  assert(memcmp(&visitor.water, CASCADE_WATER_FRAME + HDR_SIZE, sizeof(WATER_DATA)) == 0);
  assert(memcmp(&visitor.gas, CASCADE_GAS_FRAME + HDR_SIZE, sizeof(GAS_DATA)) == 0);

  // A bad checksum must not publish another status update.
  uart.append(WATER_FRAME, sizeof(WATER_FRAME));
  uart.input[uart.size - 1] ^= 1;
  link.receive();
  assert(visitor.water_count == 2);
  uart.append(NAVILINK_PRESENT, sizeof(NAVILINK_PRESENT));
  uart.input[uart.size - 1] ^= 1;
  link.receive();
  assert(!link.is_other_navilink_installed() && visitor.errors == 1);
  uart.append(NAVILINK_PRESENT, sizeof(NAVILINK_PRESENT));
  link.receive();
  assert(link.is_other_navilink_installed());

  // Every header/body split, including a missing checksum, must wait without sending.
  for (size_t split = 1; split < sizeof(WATER_FRAME); ++split) {
    Uart fragmented;
    Link parser(&fragmented);
    Visitor observer;
    parser.add_visitor(&observer);
    parser.send_turn_on_cmd();
    fragmented.append(WATER_FRAME, split);
    parser.receive();
    assert(fragmented.writes == 0 && parser.queued() == 2 && observer.water_count == 0);
    fragmented.append(WATER_FRAME + split, sizeof(WATER_FRAME) - split);
    parser.receive();
    assert(observer.water_count == 1 && observer.errors == 0);
    assert(parser.state() == INITIAL && parser.queued() == 1 && fragmented.writes == 1);
    assert_turn_on(fragmented);
  }
}

static void test_failures() {
  for (unsigned failed_read = 1; failed_read <= 2; ++failed_read) {
    for (unsigned consumed = 0; consumed <= 2; ++consumed) {
      Uart uart;
      Link link(&uart);
      Visitor visitor;
      link.add_visitor(&visitor);
      link.send_turn_on_cmd();
      uart.append(WATER_FRAME, sizeof(WATER_FRAME));
      uart.fail_on_read = failed_read;
      uart.consume_on_failure = consumed;
      link.receive();
      assert(link.state() == INITIAL && link.queued() == 2 && uart.writes == 0);
      assert(visitor.errors == 1 && visitor.water_count == 0 && uart.reads == failed_read);
      // Discard the remaining partial frame and recover on the next marker.
      // With zero bytes consumed by a failed header read, the intact original is retried.
      const unsigned recovered = (failed_read == 1 && consumed == 0) ? 2 : 1;
      uart.append(WATER_FRAME, sizeof(WATER_FRAME));
      link.receive();
      assert(visitor.water_count == recovered && visitor.errors == 1);
      assert(uart.writes == recovered && link.queued() == 2 - recovered);
      assert(link.state() == INITIAL && uart.available() == 0);
      assert_turn_on(uart);
    }
  }
}

int main(int argc, char **argv) {
  // Bound a regression that leaves receive() looping without making progress.
  alarm(15);
  const char *test = argc > 1 ? argv[1] : "all";
  const bool all = strcmp(test, "all") == 0;
  if (all || strcmp(test, "lengths") == 0) test_lengths();
  if (all || strcmp(test, "packets") == 0) test_packets();
  if (all || strcmp(test, "failures") == 0) test_failures();
  printf("PASS: %s\n", test);
}
