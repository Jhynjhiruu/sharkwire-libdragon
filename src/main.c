#include <libdragon.h>

#include <stdint.h>

// -------- Datel GAL interface --------

// Known GAL IO registers (according to Modman, ppcasm and Jhynjhiruu):
// 0x0000: read pins
// 0x0400: set IO base
// 0x0600: set pin directions? 1 = output, mapped to 'write pins'; 0 = input, mapped to 'read pins'
// 0x0800: write pins

#define IO_READ_PINS  (0x0000)
#define IO_SET_BASE   (0x0400)
#define IO_SET_DIRS   (0x0600)
#define IO_WRITE_PINS (0x0800)

static uint8_t GS_BASE = 0x10;

static const uint8_t IO_BASE = 0x40;

static uint32_t gs_addr(uint32_t addr) {
    const uint32_t masked = addr & 0x00FFFFFF;

    return ((uint32_t)GS_BASE << 24) | (masked);
}

static uint32_t read_gs(uint32_t addr) {
    const uint32_t pi_addr = gs_addr(addr);

    assertf(io_accessible(pi_addr), "PI address out of range\n");

    return io_read(pi_addr);
}

static void write_gs(uint32_t addr, uint32_t data) {
    const uint32_t pi_addr = gs_addr(addr);

    assertf(io_accessible(pi_addr), "PI address out of range\n");

    return io_write(pi_addr, data);
}

static uint32_t read_io(uint16_t addr) {
    const uint32_t gs_addr = ((uint32_t)IO_BASE << 16) | ((uint32_t)addr);

    return read_gs(gs_addr);
}

static void write_io(uint16_t addr, uint16_t data_hi, uint16_t data_lo) {
    const uint32_t gs_addr = ((uint32_t)IO_BASE << 16) | ((uint32_t)addr);

    const uint32_t data = ((uint32_t)data_hi << 16) | ((uint32_t)data_lo);

    return write_gs(gs_addr, data);
}

static void write_io_short(uint16_t addr, uint16_t data) {
    return write_io(addr, data, data);
}

static void set_gs_base(uint8_t base) {
    write_io_short(IO_SET_BASE, (uint16_t)base);
    GS_BASE = base;
}

// SharkWire mapping:
// BIT00 = 1:OUTPUT
// BIT01 = 0:INPUT
// BIT02 = 0:INPUT  (RX)
// BIT03 = 1:OUTPUT (TX)
// BIT04 = 1:OUTPUT (RTS)
// BIT05 = 0:INPUT  (CTS)
// BIT06 = 1:OUTPUT
// BIT07 = 1:OUTPUT
// BIT08 = 0:INPUT
// BIT09 = 0:INPUT
// BIT10 = 1:OUTPUT
// BIT11 = 0:INPUT
// BIT12 = ???
// BIT13 = ???
// BIT14 = ???
// BIT15 = ???

// -------- Modem code --------

// Modem status mask for CTS
#define MODEM_STATUS_CTS (0x20)

// Timeout (in ms)
#define TIMEOUT_MS (2000)

// Serial bit period (for ~20000 baud: ~50μs)
#define BIT_PERIOD_μS (1'000'000 / 20'000)

static bool init_modem(void) {
    uint32_t timeout_start;

    // Set port direction to        IIII_IOII_OOIO_OIIO (I = input | O = output)
    write_io_short(IO_SET_DIRS,   0b0000'0100'1101'1001);

    // Write 0b1100'1000 to output (likely to set serial IDLE state)
    write_io_short(IO_WRITE_PINS, 0b0000'0000'1100'1000);

    // Write 0b0000'0100'1100'1001 to output
    //
    // Write 1 on TX and 0 on RTS to hold serial IDLE (TX high) and signal to
    // the modem that we're ready to send data (active low signal)
    write_io_short(IO_WRITE_PINS, 0b0000'0100'1100'1000);

    // Delay for 200ms
    wait_ms(200);

    // Hold serial IDLE still
    write_io_short(IO_WRITE_PINS, 0b0000'0000'1100'1000);

    // Read the modem status and return true if it's clear to send,
    // respecting the timeout
    timeout_start = TICKS_READ();
    while (TICKS_SINCE(timeout_start) < TICKS_FROM_MS(TIMEOUT_MS)) {
        if ((read_io(IO_READ_PINS) & MODEM_STATUS_CTS) == 0) {
            return true;
        }
    }

    return false;
}

static size_t send_at_cmd(uint8_t * at_cmd, size_t length) {
    uint8_t cmd_buf[10];

    size_t bytes_sent;

    cmd_buf[0] = 0b1100'1000;
    cmd_buf[9] = 0b1100'0000;

    // Set TX line IDLE
    write_io_short(IO_WRITE_PINS, 0b0000'0000'1100'1000);

    // Loop through all of the bytes in the command
    for (bytes_sent = 0; bytes_sent < length; bytes_sent++) {
        // Read modem status and check CTS to see if we're clear to send
        if ((read_io(IO_READ_PINS) & MODEM_STATUS_CTS) == 0) {
            break;
        }

        // Loop through the bits in the current byte
        const uint8_t b = at_cmd[bytes_sent];
        for (int i = 0; i < 8; i++) {
            // Encode the current bit into a byte
            // 0 => 0xC0 | 1 => 0xC8
            if ((b & (1 << i)) == 0) {
                cmd_buf[i + 1] = 0b1100'0000;
            } else {
                cmd_buf[i + 1] = 0b1100'1000;
            }
        }

        // Loop through the encoded bytes
        for (int i = 0; i < 10; i++) {
            // Write the current byte on the pins
            write_io_short(IO_WRITE_PINS, cmd_buf[i]);

            // ...read the pins?
            //read_io(IO_READ_PINS);

            // Delay for one serial bit period (~50μs)
            wait_ticks(TICKS_FROM_US(BIT_PERIOD_μS));
        }
    }

    // Go back to IDLE (TX high)
    write_io_short(IO_WRITE_PINS, 0b0000'0000'1100'1000);

    return bytes_sent;
}

int main(void) {
    console_init();
    console_set_render_mode(RENDER_MANUAL);
    joypad_init();

    // Avoid issues with trying to read from 0xB0400000
    set_gs_base(0x1E);

    bool is_modem_initialised = init_modem();

    while (true) {
        console_clear();
        printf("Modem initialised: %s\n", is_modem_initialised ? "yes" : "no");
        console_render();
    }
}
