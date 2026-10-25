/* Offline regression for the actual lbp16.c reload entry point. GPL-2.0-or-later. */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include "lbp16.h"

/* Literal wire oracles: deliberately independent of production packet macros. */
static const unsigned char query[] = {0x01, 0x5d, 0x12, 0x00};
static const unsigned char efinix_user[] = {
    0x01,0xd9,0x1e,0x00,0x05,0x00, 0x01,0xd9,0x1e,0x00,0x0d,0x00
};
static const unsigned char efinix_fallback[] = {
    0x01,0xd9,0x1e,0x00,0x04,0x00, 0x01,0xd9,0x1e,0x00,0x0c,0x00
};
static const unsigned char xilinx_user[] = {
    0x01,0xd9,0x1e,0x00,0xff,0xff, 0x01,0xd9,0x1e,0x00,0xff,0xff,
    0x01,0xd9,0x1e,0x00,0x99,0xaa, 0x01,0xd9,0x1e,0x00,0x66,0x55,
    0x01,0xd9,0x1e,0x00,0x61,0x32, 0x01,0xd9,0x1e,0x00,0x00,0x00,
    0x01,0xd9,0x1e,0x00,0x81,0x32, 0x01,0xd9,0x1e,0x00,0x00,0x0b,
    0x01,0xd9,0x1e,0x00,0xa1,0x30, 0x01,0xd9,0x1e,0x00,0x0e,0x00,
    0x01,0xd9,0x1e,0x00,0x00,0x20, 0x01,0xd9,0x1e,0x00,0x00,0x20,
    0x01,0xd9,0x1e,0x00,0x00,0x20, 0x01,0xd9,0x1e,0x00,0x00,0x20
};
static const unsigned char xilinx_fallback[] = {
    0x01,0xd9,0x1e,0x00,0xff,0xff, 0x01,0xd9,0x1e,0x00,0xff,0xff,
    0x01,0xd9,0x1e,0x00,0x99,0xaa, 0x01,0xd9,0x1e,0x00,0x66,0x55,
    0x01,0xd9,0x1e,0x00,0x61,0x32, 0x01,0xd9,0x1e,0x00,0x00,0x00,
    0x01,0xd9,0x1e,0x00,0x81,0x32, 0x01,0xd9,0x1e,0x00,0x01,0x0b,
    0x01,0xd9,0x1e,0x00,0xa1,0x30, 0x01,0xd9,0x1e,0x00,0x0e,0x00,
    0x01,0xd9,0x1e,0x00,0x00,0x20, 0x01,0xd9,0x1e,0x00,0x00,0x20,
    0x01,0xd9,0x1e,0x00,0x00,0x20, 0x01,0xd9,0x1e,0x00,0x00,0x20
};

struct event {
    int receive, transport, size;
    unsigned char bytes[84];
};
static struct event events[4];
static int event_count, event_fd = -1, selected_fpga;
static int fail_stage, fail_kind;
static u16 version;
static unsigned cases, failures;
static const char *case_name;

static void check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL %s: %s\n", case_name, message);
        failures++;
    }
}

static int transfer(void *packet, int size, int receive, int transport)
{
    struct event event;
    size_t copy_size;
    ssize_t written;
    memset(&event, 0, sizeof(event));
    event.receive = receive;
    event.transport = transport;
    event.size = size;
    if (!receive && size > 0) {
        /* Do not inspect the baseline's uninitialized Efinix tail. The length
         * mismatch alone proves the defect; only its initialized prefix is read. */
        copy_size = event_count == 0 ? sizeof(query) :
            (selected_fpga == FPGA_TYPE_EFINIX ? sizeof(efinix_user) : sizeof(xilinx_user));
        if (copy_size > (size_t)size) copy_size = (size_t)size;
        if (copy_size > sizeof(event.bytes)) copy_size = sizeof(event.bytes);
        memcpy(event.bytes, packet, copy_size);
    }
    if (event_count < 4) events[event_count] = event;
    event_count++;
    if (event_fd >= 0) {
        do { written = write(event_fd, &event, sizeof(event)); }
        while (written < 0 && errno == EINTR);
        if (written != (ssize_t)sizeof(event)) _exit(92);
    }
    if (event_count == fail_stage) {
        errno = EIO;
        if (fail_kind == 0) return -1;
        if (fail_kind == 1) return 0;
        return size - 1;
    }
    if (receive && size == (int)sizeof(version)) memcpy(packet, &version, sizeof(version));
    return size;
}

/* These four definitions replace transport implementations at link time.
 * No socket, serial device, board discovery or reload CLI is linked or called. */
int eth_send_packet(void *packet, int size) { return transfer(packet, size, 0, BOARD_ETH); }
int eth_recv_packet(void *packet, int size) { return transfer(packet, size, 1, BOARD_ETH); }
int serial_send_packet(void *packet, int size) { return transfer(packet, size, 0, BOARD_SER); }
int serial_recv_packet(void *packet, int size) { return transfer(packet, size, 1, BOARD_SER); }

static void prepare(board_t *board, int transport, int fpga, u16 firmware)
{
    memset(board, 0, sizeof(*board));
    memset(events, 0, sizeof(events));
    board->type = transport;
    board->fpga_type = fpga;
    board->llio.board = board;
    selected_fpga = fpga;
    version = firmware;
    event_count = fail_stage = 0;
    event_fd = -1;
    lbp16_init(transport);
}

static void verify_event(int index, int transport, int receive,
                         const unsigned char *bytes, size_t size, int check_size)
{
    struct event *event = &events[index];
    check(event->receive == receive && event->transport == transport, "transport/order mismatch");
    if (check_size && event->size != (int)size) {
        fprintf(stderr, "  event %d actual length %d expected %zu\n", index, event->size, size);
        check(0, "packet length mismatch");
    }
    if (bytes) check(memcmp(event->bytes, bytes, size) == 0, "literal packet bytes mismatch");
}

static void success_case(int transport, int fpga, int fallback, u16 firmware, int refused)
{
    board_t board;
    char name[100];
    const unsigned char *expected;
    size_t expected_size;
    size_t i, captured_size;
    int result;
    snprintf(name, sizeof(name), "%s %s flag=%d firmware=%u",
             transport == BOARD_ETH ? "ethernet" : "serial",
             fpga == FPGA_TYPE_EFINIX ? "efinix" : "xilinx", fallback, firmware);
    case_name = name;
    cases++;
    prepare(&board, transport, fpga, firmware);
    result = lbp16_board_reload(&board.llio, fallback);
    check(result == (refused ? -1 : 0), "return value mismatch");
    check(event_count == (refused ? 2 : 3), "unexpected transfer count");
    verify_event(0, transport, 0, query, sizeof(query), 1);
    verify_event(1, transport, 1, NULL, 2, 1);
    printf("capture %s: result=%d query[%d]=", name, result, events[0].size);
    for (i = 0; i < sizeof(query); i++) printf("%02x", events[0].bytes[i]);
    if (!refused) {
        if (fpga == FPGA_TYPE_EFINIX) {
            expected = fallback == 1 ? efinix_fallback : efinix_user;
            expected_size = sizeof(efinix_user);
        } else {
            expected = fallback == 1 ? xilinx_fallback : xilinx_user;
            expected_size = sizeof(xilinx_user);
        }
        verify_event(2, transport, 0, expected, expected_size, 1);
        captured_size = expected_size;
        if (events[2].size >= 0 && captured_size > (size_t)events[2].size)
            captured_size = (size_t)events[2].size;
        printf(" reload[attempted=%d,captured=%zu]=", events[2].size, captured_size);
        for (i = 0; i < captured_size; i++) printf("%02x", events[2].bytes[i]);
        if (events[2].size > (int)captured_size) printf(" (tail deliberately unread)");
    } else {
        printf(" reload=refused");
    }
    putchar('\n');
}

static void abort_case(int transport, int fpga, int stage, int kind)
{
    int descriptors[2], status = 0, count = 0;
    pid_t child, waited;
    struct event event;
    ssize_t length;
    char name[100];
    snprintf(name, sizeof(name), "abort transport=%d fpga=%d stage=%d failure=%d", transport, fpga, stage, kind);
    case_name = name;
    cases++;
    if (pipe(descriptors) != 0) { check(0, "pipe failed"); return; }
    fflush(NULL);
    child = fork();
    if (child == 0) {
        board_t board;
        struct rlimit core = {0, 0};
        close(descriptors[0]);
        if (setrlimit(RLIMIT_CORE, &core) != 0) _exit(93);
        alarm(3); /* A broken abort path must not hang the regression. */
        prepare(&board, transport, fpga, transport == BOARD_ETH ? 15 : 2);
        event_fd = descriptors[1];
        fail_stage = stage;
        fail_kind = kind;
        (void)lbp16_board_reload(&board.llio, 0);
        _exit(94);
    }
    close(descriptors[1]);
    if (child < 0) { close(descriptors[0]); check(0, "fork failed"); return; }
    do { waited = waitpid(child, &status, 0); } while (waited < 0 && errno == EINTR);
    check(waited == child && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT,
          "checked helper did not terminate with SIGABRT");
    memset(events, 0, sizeof(events));
    for (;;) {
        do { length = read(descriptors[0], &event, sizeof(event)); }
        while (length < 0 && errno == EINTR);
        if (length == 0) break;
        if (length != (ssize_t)sizeof(event)) { check(0, "incomplete event pipe record"); break; }
        if (count < 4) events[count] = event;
        count++;
    }
    close(descriptors[0]);
    check(count == stage, "transfers continued after checked helper failure");
    verify_event(0, transport, 0, query, sizeof(query), 1);
    if (stage >= 2) verify_event(1, transport, 1, NULL, 2, 1);
    /* Length correctness belongs to success cases. Error controls also run on
     * the defective baseline, whose attempted Efinix send is 84 bytes. */
    if (stage == 3) verify_event(2, transport, 0,
        fpga == FPGA_TYPE_EFINIX ? efinix_user : xilinx_user,
        fpga == FPGA_TYPE_EFINIX ? sizeof(efinix_user) : sizeof(xilinx_user), 0);
}

int main(void)
{
    int transports[] = {BOARD_ETH, BOARD_SER};
    int t, fpga, fallback, stage, kind;
    for (t = 0; t < 2; t++) {
        for (fpga = FPGA_TYPE_XILINX; fpga <= FPGA_TYPE_EFINIX; fpga++) {
            for (fallback = 0; fallback <= 1; fallback++)
                success_case(transports[t], fpga, fallback, t == 0 ? 15 : 2, 0);
            success_case(transports[t], fpga, 1, t == 0 ? 14 : 1, 1);
            for (stage = 1; stage <= 3; stage++)
                for (kind = 0; kind < 3; kind++) abort_case(transports[t], fpga, stage, kind);
        }
    }
    success_case(BOARD_ETH, FPGA_TYPE_EFINIX, 2, 65535, 0);
    success_case(BOARD_ETH, FPGA_TYPE_EFINIX, -1, 16, 0);
    printf("reload packet regression: %u cases, %u failed assertions\n", cases, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
