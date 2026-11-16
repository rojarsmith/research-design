# translate.gdb - Stage 03, exercise F: confirm a hand translation of
# &marker in vmprinttest. Do the translation on paper first.
#
# run-xv6.sh writes the connection lines (file, target remote) and then
# sources this file. By hand: start "make CPUS=1 qemu-gdb", start
# gdb-multiarch in the same directory, then "source <path>/translate.gdb"
# and type vmprinttest at the xv6 prompt.
#
# The breakpoint is in the kernel, so satp holds the kernel page table.
# xv6 maps all of RAM at its physical address there (kvmmake), which is
# why GDB can read page-table pages and data pages by physical address.
#
# Careful with convenience variables: $a0-$a7, $t0-$t6, $s0-$s11, $sp, $ra
# and $tp name CPU registers, and "set $t0 = ..." would overwrite one.

set confirm off
add-symbol-file user/_vmprinttest
break sys_vmprint
continue

set $va = (unsigned long)&marker
set $root = (unsigned long)cpus[0].proc->pagetable
printf "process %s, pid %d\n", cpus[0].proc->name, cpus[0].proc->pid
printf "va of marker       0x%016lx\n", $va
printf "root page table    0x%016lx  (its satp value: 0x%016lx)\n", $root, (8UL << 60) | ($root >> 12)
printf "satp right now     0x%016lx  (the kernel's page table)\n", $satp

set $ix2 = ($va >> 30) & 0x1ff
set $ix1 = ($va >> 21) & 0x1ff
set $ix0 = ($va >> 12) & 0x1ff
set $off = $va & 0xfff
printf "indexes            L2 %lu, L1 %lu, L0 %lu, offset 0x%lx\n", $ix2, $ix1, $ix0, $off

set $at2 = $root + 8 * $ix2
set $pte2 = *(unsigned long *)$at2
printf "L2 PTE at 0x%lx  0x%016lx  flags 0x%03lx\n", $at2, $pte2, $pte2 & 0x3ff
set $at1 = (($pte2 >> 10) << 12) + 8 * $ix1
set $pte1 = *(unsigned long *)$at1
printf "L1 PTE at 0x%lx  0x%016lx  flags 0x%03lx\n", $at1, $pte1, $pte1 & 0x3ff
set $at0 = (($pte1 >> 10) << 12) + 8 * $ix0
set $pte0 = *(unsigned long *)$at0
printf "L0 PTE at 0x%lx  0x%016lx  flags 0x%03lx\n", $at0, $pte0, $pte0 & 0x3ff
set $pa = (($pte0 >> 10) << 12) | $off
printf "physical address   0x%016lx\n", $pa
printf "string at that physical address:\n"
x/s $pa

# The same walk for TRAPFRAME, which uses the top entries of every level.
# The kernel knows this page by its physical address too: p->trapframe.
set $va = 0x3fffffe000UL
set $ix2 = ($va >> 30) & 0x1ff
set $ix1 = ($va >> 21) & 0x1ff
set $ix0 = ($va >> 12) & 0x1ff
printf "\nva of TRAPFRAME    0x%016lx  indexes L2 %lu, L1 %lu, L0 %lu\n", $va, $ix2, $ix1, $ix0
set $pte2 = *(unsigned long *)($root + 8 * $ix2)
set $pte1 = *(unsigned long *)((($pte2 >> 10) << 12) + 8 * $ix1)
set $pte0 = *(unsigned long *)((($pte1 >> 10) << 12) + 8 * $ix0)
printf "PTEs               0x%016lx  0x%016lx  0x%016lx\n", $pte2, $pte1, $pte0
printf "leaf flags 0x%03lx: U is %s\n", $pte0 & 0x3ff, ($pte0 & 0x10) ? "set" : "clear"
printf "physical address   0x%016lx\n", ($pte0 >> 10) << 12
printf "p->trapframe       0x%016lx\n", (unsigned long)cpus[0].proc->trapframe

delete
detach
