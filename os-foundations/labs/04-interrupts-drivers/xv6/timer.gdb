# timer.gdb - Stage 04, exercise C: catch the timer interrupt that takes
# the CPU away from the preempt program, then follow one timer tick.
#
# run-xv6.sh writes the connection lines and sources this file. By hand:
# "make CPUS=1 qemu-gdb", gdb-multiarch in the same directory, then
# "source <path>/timer.gdb", and run "preempt 10" at the xv6 prompt.
#
# QEMU 8.2's gdbstub did not offer stimecmp when this was written, so
# the script reads the pending bit in sip instead.

set confirm off
add-symbol-file user/_preempt

# yield() is called only from the two timer paths in trap.c. SPP = 0 in
# sstatus means the trap came from user mode, so this is usertrap()'s call.
break yield if $_streq(cpus[0].proc->name, "preempt") && (($sstatus >> 8) & 1) == 0
continue

printf "=== a timer interrupt preempts a user process ===\n"
bt
printf "process %s, pid %d, interrupted at user pc 0x%lx:\n", cpus[0].proc->name, cpus[0].proc->pid, cpus[0].proc->trapframe->epc
info symbol cpus[0].proc->trapframe->epc
printf "scause  0x%016lx  (interrupt bit set, cause 5: supervisor timer)\n", $scause
printf "ticks   %u\n", ticks
printf "preempted so far: %lu\n", cpus[0].proc->preempted
delete

break clockintr
continue
printf "=== the next timer interrupt, inside clockintr() ===\n"
bt
printf "time %lu\n", $time
printf "sip 0x%lx before clockintr() runs (bit 5, STIP: timer interrupt pending)\n", $sip
finish
printf "sip 0x%lx after clockintr() wrote the next deadline into stimecmp\n", $sip
printf "ticks %u\n", ticks

delete
detach
