clang: Ubuntu clang version 18.1.3 (1ubuntu1)

## Implementation-defined contract

| target | CHAR_BIT | short/int/long/long long | pointer | float/double | byte order |
| --- | --- | --- | --- | --- | --- |
| avr | 8 | 2/2/4/8 | 2 | 4/4 | little |
| msp430 | 8 | 2/2/4/8 | 2 | 4/8 | little |
| cortex-m0 | 8 | 2/4/4/8 | 4 | 4/8 | little |
| cortex-m4 | 8 | 2/4/4/8 | 4 | 4/8 | little |
| cortex-m4f | 8 | 2/4/4/8 | 4 | 4/8 | little |
| rv32e | 8 | 2/4/4/8 | 4 | 4/8 | little |
| rv32i | 8 | 2/4/4/8 | 4 | 4/8 | little |
| rv32im | 8 | 2/4/4/8 | 4 | 4/8 | little |
| rv32imf | 8 | 2/4/4/8 | 4 | 4/8 | little |
| rv64gc | 8 | 2/4/8/8 | 8 | 4/8 | little |
| i386 | 8 | 2/4/4/8 | 4 | 4/8 | little |
| x86-64 | 8 | 2/4/8/8 | 8 | 4/8 | little |
| aarch64 | 8 | 2/4/8/8 | 8 | 4/8 | little |
| mips32 | 8 | 2/4/4/8 | 4 | 4/8 | big |
| m68k | 8 | 2/4/4/8 | 4 | 4/8 | big |
| sparc | 8 | 2/4/4/8 | 4 | 4/8 | big |
| wasm32 | 8 | 2/4/4/8 | 4 | 4/8 | little |
| nvptx64 | 8 | 2/4/8/8 | 8 | 4/8 | little |
| bpf | 8 | 2/4/8/8 | 8 | 4/8 | little |

## .text bytes at -O2 (instructions in parentheses)

| target | 01_fib | 02_loops | 03_pointers | 04_globals | 05_logic | 10_mul_div | 11_long_long | 12_float | 13_funcptr_switch | 14_volatile_atomic |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| avr | 314 (124) | 418 (195) | 508 (229) | 490 (166) | 222 (85) | 58 (30) | 156 (80) | 40 (20) | 58 (35) | 76 (33) |
| msp430 | 274 (85) | 288 (114) | 342 (117) | 266 (59) | 162 (47) | 30 (15) | 492 (191) | 86 (39) | 52 (28) | 54 (22) |
| cortex-m0 | 248 (81) | 496 (205) | 648 (277) | 288 (116) | 184 (58) | 68 (15) | 80 (21) | 90 (20) | 108 (25) | 132 (40) |
| cortex-m4 | 242 (77) | 480 (163) | 586 (202) | 282 (88) | 172 (54) | 62 (9) | 104 (25) | 98 (20) | 110 (24) | 144 (41) |
| cortex-m4f | 242 (77) | 480 (163) | 586 (202) | 282 (88) | 172 (54) | 62 (9) | 104 (25) | 106 (19) | 110 (24) | 144 (41) |
| rv32e | 476 (86) | 512 (112) | 548 (112) | 428 (104) | 304 (58) | 44 (11) | 152 (40) | 140 (35) | 84 (27) | 88 (23) |
| rv32i | 484 (88) | 540 (119) | 548 (112) | 428 (104) | 304 (58) | 44 (11) | 152 (40) | 140 (35) | 84 (27) | 88 (23) |
| rv32im | 484 (88) | 396 (89) | 548 (112) | 428 (104) | 304 (58) | 32 (12) | 128 (35) | 140 (35) | 84 (27) | 88 (23) |
| rv32imf | 484 (88) | 396 (89) | 548 (112) | 428 (104) | 304 (58) | 32 (12) | 128 (35) | 80 (23) | 84 (27) | 88 (23) |
| rv64gc | 410 (88) | 294 (89) | 440 (113) | 310 (104) | 258 (58) | 24 (12) | 22 (12) | 30 (15) | 52 (27) | 58 (23) |
| i386 | 483 (108) | 382 (111) | 691 (185) | 491 (73) | 307 (59) | 59 (20) | 146 (46) | 109 (32) | 165 (40) | 103 (24) |
| x86-64 | 389 (97) | 328 (96) | 644 (159) | 356 (45) | 229 (50) | 55 (20) | 73 (24) | 69 (15) | 99 (27) | 79 (17) |
| aarch64 | 332 (85) | 344 (89) | 532 (137) | 352 (59) | 220 (57) | 36 (13) | 32 (12) | 40 (15) | 80 (26) | 92 (25) |
| mips32 | 636 (150) | 528 (130) | 632 (156) | 520 (121) | 396 (89) | 104 (18) | 184 (38) | 96 (17) | 164 (35) | 184 (39) |
| m68k | 398 (99) | 474 (125) | 548 (138) | 388 (66) | 246 (55) | 88 (28) | 192 (57) | 110 (35) | 136 (47) | 122 (33) |
| sparc | 360 (94) | 468 (129) | 480 (132) | 456 (120) | 272 (72) | 68 (25) | 112 (36) | 152 (52) | 352 (100) | 152 (45) |
| wasm32 | 358 (121) | 363 (163) | 756 (333) | 409 (98) | 240 (79) | 33 (20) | 34 (21) | 59 (32) | 72 (35) | 92 (36) |
| nvptx64 | (411) | (202) | (390) | (145) | (209) | (32) | (41) | (38) | (102) | FAIL |
| bpf | 736 (95) | FAIL | 984 (132) | 760 (94) | 424 (53) | FAIL | FAIL | FAIL | 496 (65) | FAIL |

## Compiler runtime calls (hardware the target lacks) and errors

| target | program | libcalls or error |
| --- | --- | --- |
| avr | 02_loops.c | `__divmodhi4`, `__udivmodhi4` |
| avr | 04_globals.c | `__do_clear_bss` |
| avr | 05_logic.c | `__do_clear_bss` |
| avr | 10_mul_div.c | `__divmodhi4`, `__udivmodhi4` |
| avr | 11_long_long.c | `__divdi3`, `__muldi3` |
| avr | 12_float.c | `__addsf3`, `__divsf3`, `__fixsfsi`, `__floatsisf`, `__mulsf3` |
| avr | 14_volatile_atomic.c | `__atomic_fetch_add_2`, `__atomic_load_2`, `__do_clear_bss`, `__do_copy_data` |
| msp430 | 02_loops.c | `__mspabi_mpyi`, `__mspabi_remi`, `__mspabi_remu` |
| msp430 | 10_mul_div.c | `__mspabi_divi`, `__mspabi_divu`, `__mspabi_mpyi`, `__mspabi_remi` |
| msp430 | 11_long_long.c | `__mspabi_divlli`, `__mspabi_mpyll` |
| msp430 | 12_float.c | `__mspabi_addf`, `__mspabi_divd`, `__mspabi_fixfli`, `__mspabi_fltlid`, `__mspabi_mpyf` |
| msp430 | 14_volatile_atomic.c | `__atomic_fetch_add_2`, `__atomic_load_2` |
| cortex-m0 | 02_loops.c | `__aeabi_idivmod`, `__aeabi_uidivmod` |
| cortex-m0 | 10_mul_div.c | `__aeabi_idiv`, `__aeabi_idivmod`, `__aeabi_uidiv` |
| cortex-m0 | 11_long_long.c | `__aeabi_ldivmod`, `__aeabi_llsl`, `__aeabi_lmul` |
| cortex-m0 | 12_float.c | `__aeabi_ddiv`, `__aeabi_f2iz`, `__aeabi_fadd`, `__aeabi_fmul`, `__aeabi_i2d` |
| cortex-m0 | 14_volatile_atomic.c | `__atomic_fetch_add_4`, `__atomic_load_4` |
| cortex-m4 | 11_long_long.c | `__aeabi_ldivmod` |
| cortex-m4 | 12_float.c | `__aeabi_ddiv`, `__aeabi_f2iz`, `__aeabi_fadd`, `__aeabi_fmul`, `__aeabi_i2d` |
| cortex-m4f | 11_long_long.c | `__aeabi_ldivmod` |
| cortex-m4f | 12_float.c | `__aeabi_ddiv`, `__aeabi_i2d` |
| rv32e | 02_loops.c | `__modsi3`, `__mulsi3`, `__umodsi3` |
| rv32e | 10_mul_div.c | `__divsi3`, `__modsi3`, `__mulsi3`, `__udivsi3` |
| rv32e | 11_long_long.c | `__divdi3`, `__muldi3` |
| rv32e | 12_float.c | `__addsf3`, `__divdf3`, `__fixsfsi`, `__floatsidf`, `__mulsf3` |
| rv32e | 14_volatile_atomic.c | `__atomic_fetch_add_4`, `__atomic_load_4` |
| rv32i | 02_loops.c | `__modsi3`, `__mulsi3`, `__umodsi3` |
| rv32i | 10_mul_div.c | `__divsi3`, `__modsi3`, `__mulsi3`, `__udivsi3` |
| rv32i | 11_long_long.c | `__divdi3`, `__muldi3` |
| rv32i | 12_float.c | `__addsf3`, `__divdf3`, `__fixsfsi`, `__floatsidf`, `__mulsf3` |
| rv32i | 14_volatile_atomic.c | `__atomic_fetch_add_4`, `__atomic_load_4` |
| rv32im | 11_long_long.c | `__divdi3` |
| rv32im | 12_float.c | `__addsf3`, `__divdf3`, `__fixsfsi`, `__floatsidf`, `__mulsf3` |
| rv32im | 14_volatile_atomic.c | `__atomic_fetch_add_4`, `__atomic_load_4` |
| rv32imf | 11_long_long.c | `__divdi3` |
| rv32imf | 12_float.c | `__divdf3`, `__floatsidf` |
| rv32imf | 14_volatile_atomic.c | `__atomic_fetch_add_4`, `__atomic_load_4` |
| i386 | 11_long_long.c | `__divdi3` |
| mips32 | 11_long_long.c | `__divdi3` |
| m68k | 02_loops.c | `__modsi3`, `__mulsi3`, `__umodsi3` |
| m68k | 10_mul_div.c | `__divsi3`, `__modsi3`, `__mulsi3`, `__udivsi3` |
| m68k | 11_long_long.c | `__divdi3`, `__muldi3` |
| m68k | 12_float.c | `__addsf3`, `__divdf3`, `__fixsfsi`, `__floatsidf`, `__mulsf3` |
| m68k | 14_volatile_atomic.c | `__atomic_fetch_add_4`, `__atomic_load_4` |
| sparc | 11_long_long.c | `__ashldi3`, `__divdi3` |
| sparc | 14_volatile_atomic.c | `__atomic_fetch_add_4`, `__atomic_load_4` |
| nvptx64 | 14_volatile_atomic.c | error: error in backend: Cannot select: i32,ch = AtomicLoad<(dereferenceable load acquire (s32) from @counter, addrsp... |
| bpf | 02_loops.c | error: unsupported signed division, please convert to unsigned div/mod. |
| bpf | 10_mul_div.c | error: unsupported signed division, please convert to unsigned div/mod. |
| bpf | 11_long_long.c | error: unsupported signed division, please convert to unsigned div/mod. |
| bpf | 12_float.c | error: A call to built-in function '__addsf3' is not supported. |
| bpf | 14_volatile_atomic.c | error: error in backend: Invalid usage of the XADD return value |
