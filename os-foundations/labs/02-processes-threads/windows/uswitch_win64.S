# uswitch_win64.S - a user-level context switch for the Windows x64 ABI.
#
#   void uswitch(struct uctx *old, struct uctx *new);
#
# The same job as xv6's kernel/swtch.S: save the registers a callee must
# preserve into *old, load them from *new, and return on the new stack.
# Windows x64 passes the first two arguments in rcx and rdx, and treats
# rbx, rbp, rdi, rsi, rsp, r12-r15 and xmm6-xmm15 as nonvolatile.
#
# There is no ra register on x86-64: `call` pushed the return address on
# the old stack, so saving rsp saves it, and the final `ret` pops the
# return address that is on the new stack.
#
# Build with -DSKIP_XMM to leave xmm6-xmm15 out on purpose (exercise D).
#
# Layout of struct uctx (must match uthread_win64.c):
#   0x00 rsp, 0x08 rbx, 0x10 rbp, 0x18 rdi, 0x20 rsi,
#   0x28 r12, 0x30 r13, 0x38 r14, 0x40 r15, 0x48 padding,
#   0x50 xmm6 ... 0xE0 xmm15 (16 bytes each), size 0xF0.

        .intel_syntax noprefix
        .text
        .globl  uswitch
uswitch:
        mov     [rcx + 0x00], rsp
        mov     [rcx + 0x08], rbx
        mov     [rcx + 0x10], rbp
        mov     [rcx + 0x18], rdi
        mov     [rcx + 0x20], rsi
        mov     [rcx + 0x28], r12
        mov     [rcx + 0x30], r13
        mov     [rcx + 0x38], r14
        mov     [rcx + 0x40], r15
#ifndef SKIP_XMM
        movups  [rcx + 0x50], xmm6
        movups  [rcx + 0x60], xmm7
        movups  [rcx + 0x70], xmm8
        movups  [rcx + 0x80], xmm9
        movups  [rcx + 0x90], xmm10
        movups  [rcx + 0xA0], xmm11
        movups  [rcx + 0xB0], xmm12
        movups  [rcx + 0xC0], xmm13
        movups  [rcx + 0xD0], xmm14
        movups  [rcx + 0xE0], xmm15
#endif

        mov     rsp, [rdx + 0x00]
        mov     rbx, [rdx + 0x08]
        mov     rbp, [rdx + 0x10]
        mov     rdi, [rdx + 0x18]
        mov     rsi, [rdx + 0x20]
        mov     r12, [rdx + 0x28]
        mov     r13, [rdx + 0x30]
        mov     r14, [rdx + 0x38]
        mov     r15, [rdx + 0x40]
#ifndef SKIP_XMM
        movups  xmm6,  [rdx + 0x50]
        movups  xmm7,  [rdx + 0x60]
        movups  xmm8,  [rdx + 0x70]
        movups  xmm9,  [rdx + 0x80]
        movups  xmm10, [rdx + 0x90]
        movups  xmm11, [rdx + 0xA0]
        movups  xmm12, [rdx + 0xB0]
        movups  xmm13, [rdx + 0xC0]
        movups  xmm14, [rdx + 0xD0]
        movups  xmm15, [rdx + 0xE0]
#endif
        ret
