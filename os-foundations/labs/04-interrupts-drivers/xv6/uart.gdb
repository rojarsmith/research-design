# uart.gdb - Stage 04, exercise A: stop where a received character reaches
# the console layer, and look back along the path it took:
# PLIC -> devintr() -> uartintr() -> consoleintr().
#
# run-xv6.sh writes the connection lines and sources this file. By hand:
# "make CPUS=1 qemu-gdb", gdb-multiarch in the same directory, then
# "source <path>/uart.gdb", and type one character at the xv6 prompt.
#
# Do not x/ the UART's RHR (0x10000000) or ISR (0x10000002), or the
# PLIC claim register: reading them has side effects on the device.

set confirm off
break consoleintr
continue

printf "=== one received character ===\n"
bt
printf "character: %d '%c'\n", c, c
printf "scause  0x%016lx  (interrupt bit set, cause 9: supervisor external)\n", $scause
printf "sstatus 0x%016lx  SPP=%ld, so the trap came from %s mode\n", $sstatus, ($sstatus >> 8) & 1, (($sstatus >> 8) & 1) ? "supervisor" : "user"
printf "sepc    0x%016lx\n", $sepc
info symbol $sepc
printf "this hart's current process: %p\n", cpus[0].proc
printf "PLIC pending word 0x0c001000: 0x%08x (bit 10 is the UART)\n", *(unsigned int *)0x0c001000
printf "UART LSR 0x10000005: 0x%02x (bit 0: more input waiting, bit 5: transmitter idle)\n", *(unsigned char *)0x10000005
delete

# devintr() keeps the claimed irq in a register (GDB says it is optimized
# out), so catch it where devintr() hands it back to the PLIC.
break plic_complete
continue
printf "=== devintr() completes the interrupt it claimed ===\n"
bt
printf "irq %d\n", irq

delete
detach
