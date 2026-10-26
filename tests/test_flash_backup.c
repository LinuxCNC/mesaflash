/* Offline regression for the actual eeprom.c flash_backup entry point. GPL-2.0-or-later.
 *
 * Links the real eeprom.c only. No bitfile.c, eeprom_local.c,
 * eeprom_remote.c or any physical transport is linked or called: page
 * reads come from a deterministic memory stub installed in eeprom_access,
 * and stdio failures are injected with GNU ld --wrap (fopen, fwrite,
 * fclose, fprintf). Run with: make -f tests/Makefile.test-backup check
 *
 * On the unfixed baseline, cases 2 to 7 must FAIL (the baseline ignores
 * fwrite/fclose/fprintf results, leaves partial files behind and returns
 * 0) while case 1 passes. Do not weaken the assertions to pass there.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L
#include <limits.h>
#include <sha256.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "boards.h"
#include "eeprom.h"
#include "eeprom_local.h"
#include "eeprom_remote.h"
#include "bitfile.h"

/* ID_EEPROM_1M is 128 KiB: 512 pages of 256 bytes. Literals on purpose,
 * independent of the production size helper. */
#define TEST_FLASH_SIZE 131072u
#define TEST_FLASH_PAGES 512

extern spi_eeprom_dev_t eeprom_access;

static unsigned cases, failures;
static const char *case_name;
static board_t board;
static int read_counter;

/* Test-only stdio injection state, reset before every case. */
static FILE *data_stream, *checksum_stream;
static int data_fwrite_calls;
static int fail_fwrite_at;      /* 1-based data fwrite call, 0 means never */
static size_t fail_fwrite_len;  /* bytes really written on the failed call */
static int fail_data_fclose, fail_checksum_fclose, fail_checksum_fprintf;

extern FILE *__real_fopen(const char *path, const char *mode);
extern size_t __real_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
extern int __real_fclose(FILE *stream);
/* No __real_fprintf forwarding is possible for varargs; __wrap_fprintf
 * below passes through with vfprintf instead. */

static void check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL %s: %s\n", case_name, message);
        failures++;
    }
}

/* Link stubs for symbols eeprom.c references from other translation
 * units. flash_backup never calls them; they exist only so eeprom.c
 * links without bitfile.c, eeprom_local.c or eeprom_remote.c. */
int print_bitfile_header(FILE *fp, char *part_name, int verbose_flag)
{
    (void)fp; (void)part_name; (void)verbose_flag;
    return 0;
}
void open_spi_access_local(llio_t *self) { (void)self; }
void close_spi_access_local(llio_t *self) { (void)self; }
void open_spi_access_remote(llio_t *self) { (void)self; }
void close_spi_access_remote(llio_t *self) { (void)self; }

/* Deterministic memory-backed page read: byte at absolute flash offset
 * (addr + i) has value (addr + i) & 0xFF. Verification recomputes the
 * expected bytes from the same absolute-offset rule independently. */
static void memory_read_page(llio_t *self, u32 addr, void *buff)
{
    unsigned char *bytes = (unsigned char *)buff;
    int i;
    (void)self;
    for (i = 0; i < PAGE_SIZE; i++)
        bytes[i] = (unsigned char)((addr + (u32)i) & 0xFFu);
    read_counter++;
}

static int has_suffix(const char *path, const char *suffix)
{
    size_t path_len = strlen(path), suffix_len = strlen(suffix);
    if (path_len < suffix_len) return 0;
    return strcmp(path + path_len - suffix_len, suffix) == 0;
}

FILE *__wrap_fopen(const char *path, const char *mode)
{
    FILE *fp = __real_fopen(path, mode);
    if (fp != NULL && path != NULL && mode != NULL) {
        if (has_suffix(path, ".sha256"))
            checksum_stream = fp;
        else if (strchr(mode, 'w') != NULL)
            data_stream = fp;
    }
    return fp;
}

size_t __wrap_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if (stream != NULL && stream == data_stream) {
        data_fwrite_calls++;
        if (fail_fwrite_at > 0 && data_fwrite_calls == fail_fwrite_at) {
            size_t total = size * nmemb;
            size_t want = fail_fwrite_len;
            size_t written;
            if (want > total) want = total;
            if (want == 0) return 0;
            written = __real_fwrite(ptr, 1, want, stream);
            return size != 0 ? written / size : 0;
        }
    }
    return __real_fwrite(ptr, size, nmemb, stream);
}

int __wrap_fclose(FILE *stream)
{
    int is_data = stream != NULL && stream == data_stream;
    int is_checksum = stream != NULL && stream == checksum_stream;
    int fail = (is_data && fail_data_fclose) ||
               (is_checksum && fail_checksum_fclose);
    int rc = __real_fclose(stream);
    if (is_data) data_stream = NULL;
    if (is_checksum) checksum_stream = NULL;
    return fail ? EOF : rc;
}

int __wrap_fprintf(FILE *stream, const char *format, ...)
{
    va_list ap;
    int rc;
    if (stream != NULL && stream == checksum_stream && fail_checksum_fprintf)
        return -1;
    va_start(ap, format);
    rc = vfprintf(stream, format, ap);
    va_end(ap);
    return rc;
}

static int path_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Expected digest over the expected byte sequence, computed here from
 * the absolute-offset pattern rule, not by production code. */
static void compute_expected_sha256(unsigned char digest[SHA256_DIGEST_LENGTH])
{
    SHA256_CTX ctx;
    unsigned char buf[4096];
    size_t off = 0;
    SHA256_Init(&ctx);
    while (off < TEST_FLASH_SIZE) {
        size_t n = TEST_FLASH_SIZE - off;
        size_t j;
        if (n > sizeof(buf)) n = sizeof(buf);
        for (j = 0; j < n; j++)
            buf[j] = (unsigned char)((off + j) & 0xFFu);
        SHA256_Update(&ctx, buf, n);
        off += n;
    }
    SHA256_Final(digest, &ctx);
}

static void verify_data_file(const char *path)
{
    struct stat st;
    FILE *fp;
    unsigned char buf[4096];
    size_t off = 0, n, first_bad = 0;
    unsigned mismatches = 0;
    check(stat(path, &st) == 0, "cannot stat data file");
    if (stat(path, &st) == 0 && st.st_size != (off_t)TEST_FLASH_SIZE) {
        fprintf(stderr, "  data size actual %lld expected %u\n",
                (long long)st.st_size, TEST_FLASH_SIZE);
        check(0, "data file size mismatch");
    }
    fp = fopen(path, "rb");
    check(fp != NULL, "cannot open data file for verification");
    if (fp == NULL) return;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        size_t j;
        for (j = 0; j < n; j++) {
            if (buf[j] != (unsigned char)((off + j) & 0xFFu)) {
                if (mismatches == 0) first_bad = off + j;
                mismatches++;
            }
        }
        off += n;
    }
    check(ferror(fp) == 0, "read error during data verification");
    fclose(fp);
    if (off != TEST_FLASH_SIZE) {
        fprintf(stderr, "  data bytes read %zu expected %u\n", off, TEST_FLASH_SIZE);
        check(0, "data file length mismatch");
    }
    if (mismatches != 0) {
        fprintf(stderr, "  %u byte mismatches, first at offset %zu\n",
                mismatches, first_bad);
        check(0, "data byte pattern mismatch");
    }
}

static void verify_sidecar(const char *path, const char *basename_str)
{
    unsigned char expected[SHA256_DIGEST_LENGTH];
    unsigned char parsed[SHA256_DIGEST_LENGTH];
    char content[512];
    FILE *fp;
    size_t n, i;
    int parsed_ok = 1;
    fp = fopen(path, "rt");
    check(fp != NULL, "cannot open sidecar for verification");
    if (fp == NULL) return;
    n = fread(content, 1, sizeof(content) - 1, fp);
    check(ferror(fp) == 0, "read error during sidecar verification");
    fclose(fp);
    content[n] = '\0';
    check(n >= 64, "sidecar too short for a sha256 hex string");
    if (n < 64) return;
    for (i = 0; i < (size_t)SHA256_DIGEST_LENGTH; i++) {
        int hi = hex_value(content[2 * i]);
        int lo = hex_value(content[2 * i + 1]);
        if (hi < 0 || lo < 0) { parsed_ok = 0; break; }
        parsed[i] = (unsigned char)((hi << 4) | lo);
    }
    check(parsed_ok, "sidecar does not start with 64 hex characters");
    compute_expected_sha256(expected);
    if (parsed_ok && memcmp(parsed, expected, SHA256_DIGEST_LENGTH) != 0) {
        fprintf(stderr, "  sidecar prefix %.64s\n", content);
        check(0, "sidecar sha256 mismatch");
    }
    check(strstr(content, " *./") != NULL, "sidecar missing binary marker");
    check(strstr(content, basename_str) != NULL, "sidecar missing basename");
    check(n > 0 && content[n - 1] == '\n', "sidecar missing trailing newline");
}

struct backup_case {
    const char *name;
    int fail_fwrite_at;
    size_t fail_fwrite_len;
    int fail_data_fclose;
    int fail_checksum_fprintf;
    int fail_checksum_fclose;
    int expected_ret;
    int expected_reads;
    int expect_success;
};

static void run_case(const struct backup_case *cfg)
{
    unsigned failures_before = failures;
    char dir_template[] = "/tmp/flash-backup-test-XXXXXX";
    char *dir;
    char data_path[PATH_MAX], sha_path[PATH_MAX];
    int ret, reads, wrote;
    cases++;
    case_name = cfg->name;

    data_stream = NULL;
    checksum_stream = NULL;
    data_fwrite_calls = 0;
    read_counter = 0;
    fail_fwrite_at = cfg->fail_fwrite_at;
    fail_fwrite_len = cfg->fail_fwrite_len;
    fail_data_fclose = cfg->fail_data_fclose;
    fail_checksum_fprintf = cfg->fail_checksum_fprintf;
    fail_checksum_fclose = cfg->fail_checksum_fclose;

    dir = mkdtemp(dir_template);
    check(dir != NULL, "mkdtemp failed");
    if (dir == NULL) {
        printf("FAIL %s: no temp dir\n", cfg->name);
        return;
    }
    wrote = snprintf(data_path, sizeof(data_path), "%s/backup.bin", dir);
    check(wrote >= 0 && (size_t)wrote < sizeof(data_path), "data path too long");
    wrote = snprintf(sha_path, sizeof(sha_path), "%s/backup.bin.sha256", dir);
    check(wrote >= 0 && (size_t)wrote < sizeof(sha_path), "sidecar path too long");

    memset(&board, 0, sizeof(board));
    board.flash_id = ID_EEPROM_1M;
    board.llio.board = &board;
    board.llio.verbose = 0;
    snprintf(board.llio.board_name, sizeof(board.llio.board_name), "%s", "TESTBD");
    memset(&eeprom_access, 0, sizeof(eeprom_access));
    eeprom_access.read_page = memory_read_page;

    /* data_path does not exist yet, so flash_backup takes its plain
     * file path (no auto naming, no already-exists refusal). */
    ret = flash_backup(&board.llio, data_path);
    reads = read_counter;

    /* Stop injection so the verification stdio below is not intercepted.
     * Counters above are already captured. */
    fail_fwrite_at = 0;
    fail_data_fclose = 0;
    fail_checksum_fprintf = 0;
    fail_checksum_fclose = 0;
    data_stream = NULL;
    checksum_stream = NULL;

    if (ret != cfg->expected_ret) {
        fprintf(stderr, "  ret actual %d expected %d\n", ret, cfg->expected_ret);
        check(0, "return value mismatch");
    }
    if (reads != cfg->expected_reads) {
        fprintf(stderr, "  reads actual %d expected %d\n", reads, cfg->expected_reads);
        check(0, "read count mismatch");
    }
    if (cfg->expect_success) {
        check(path_exists(data_path), "data file missing after success");
        check(path_exists(sha_path), "sidecar missing after success");
        if (path_exists(data_path)) verify_data_file(data_path);
        if (path_exists(sha_path)) verify_sidecar(sha_path, "backup.bin");
    } else {
        check(!path_exists(data_path), "data file left behind after failure");
        check(!path_exists(sha_path), "sidecar left behind after failure");
    }

    unlink(data_path);
    unlink(sha_path);
    rmdir(dir);
    printf("%s %s: ret=%d reads=%d\n",
           failures == failures_before ? "PASS" : "FAIL", cfg->name, ret, reads);
}

int main(void)
{
    /* Cases 2 to 7 are also the caller-receives-failure evidence: each
     * asserts ret == -1 from flash_backup. mesaflash.c assigns the
     * anyio_dev_backup_flash result to ret and returns it from main, so
     * a -1 here is a failing process exit for the backup path. */
    static const struct backup_case backup_cases[] = {
        {"success",                    0,   0, 0, 0, 0,  0, TEST_FLASH_PAGES, 1},
        {"short write first page",     1, 128, 0, 0, 0, -1, 1,                0},
        {"zero write first page",      1,   0, 0, 0, 0, -1, 1,                0},
        {"later-page write failure",  10,   0, 0, 0, 0, -1, 10,               0},
        {"delayed data-close failure", 0,   0, 1, 0, 0, -1, TEST_FLASH_PAGES, 0},
        {"checksum write failure",     0,   0, 0, 1, 0, -1, TEST_FLASH_PAGES, 0},
        {"checksum close failure",     0,   0, 0, 0, 1, -1, TEST_FLASH_PAGES, 0},
    };
    size_t i;
    for (i = 0; i < sizeof(backup_cases) / sizeof(backup_cases[0]); i++)
        run_case(&backup_cases[i]);
    printf("flash backup regression: %u cases, %u failed assertions\n", cases, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
