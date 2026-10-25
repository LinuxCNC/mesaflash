# Offline LBP16 reload regression

Run only on the approved ARM64 Azure build VM. From this project root:

```
cc -std=c99 -DMESAFLASH_IO=0 -Wall -Wextra -Werror -I. \
  lbp16.c tests/test_lbp16_reload.c -o /owned/build/test_lbp16_reload
/owned/build/test_lbp16_reload
```

The test links the actual `lbp16.c` and calls `lbp16_init` followed by
`lbp16_board_reload`. Its four replacement Ethernet/serial callbacks capture
packets in memory; no transport implementation, socket, serial device, board
discovery or mesaflash CLI is linked or invoked. Literal expected bytes do not
use production packet-construction macros.

There are 50 cases:

* Eight successful packets: both transports, both FPGA families, user/fallback.
* Two additional Efinix cases retain the existing `fallback_flag == 1` semantics
  for flags 2 and -1, with firmware versions 65535 and 16.
* Four version refusals: Ethernet 14 and serial 1, for both FPGA families.
  Successful cases cover the accepted minimum versions Ethernet 15 and serial 2.
* Thirty-six checked-helper failures: both transports, both FPGA families,
  query send/version receive/reload send, each returning -1, zero or size-minus-1.

Each nonfailure case checks result, exact transfer count/order, callback route,
query bytes, version-receive size, reload length and every initialized reload
byte. Logs include the actual captured query/reload hex and attempted lengths.
Failure children have core dumps disabled and a three-second alarm; the parent
requires SIGABRT and exactly the transfers up to the failed operation, including
their initialized payloads. These tests preserve the existing checked-helper
abort policy. Expected `perror` diagnostics from those children are retained.

At the pinned defective source, the default suite must compile successfully and
exit unsuccessfully with six Efinix length assertions (attempted 84, expected 12).
It still runs the other 44 cases. The Efinix capture intentionally reads only the
initialized 12-byte prefix, never the baseline's indeterminate 72-byte tail.
Error-path controls check the attempted initialized bytes and termination; exact
successful packet lengths are checked by the success cases. The corrected suite
must report 50 cases and zero failed assertions; Xilinx remains 84 bytes.

This is host-side packet construction and error propagation evidence only. It
does not exercise physical Ethernet/serial behavior, board acceptance, reload,
boot selection, recovery or firmware installation. No selected image is changed.
