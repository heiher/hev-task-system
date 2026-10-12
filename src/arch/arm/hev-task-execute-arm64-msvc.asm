; ============================================================================
; hev-task-execute-arm64-msvc.asm - native Windows ARM64 context switch
;
; Assembled with armasm64.exe (Microsoft ARM64 macro assembler).  This is NOT
; the ELF/GNU AArch64 source (src/arch/arm/hev-task-execute-aarch64.s) and it
; is NOT the x86/x64 MASM.  It implements the Microsoft ARM64 (AAPCS64 with
; Windows modifications) calling convention, not the SysV/AAPCS64 ABI:
;
;   * x0-x7 hold the first eight arguments; there is NO 32-byte home/shadow
;     space requirement (unlike Windows x64).
;   * the stack pointer is kept 16-byte aligned at all times.
;   * x19-x28, x29 (fp) and the low 64 bits of v8-v15 are nonvolatile.
;   * x18 is reserved and must not be touched by application code.
;
; void hev_task_execute (HevTask *self, void *executer);
;   x0 = self, x1 = executer.
;   [x0] (offset 0 of HevTask) is task->stack_bottom, the 16-byte aligned
;   top of a heap-allocated stack.  The routine switches SP to that stack so
;   that the context saved by the first _setjmp lives on the task stack, calls
;   the executer once, then restores the original SP and returns.
;
; Unwind data (.pdata/.xdata)
; ----------------------------------------------------------------------------
; armasm64 does not generate unwind data automatically, so the .pdata and
; .xdata records for hev_task_execute are emitted by hand below, following
; "ARM64 exception handling" (Microsoft Learn).
;
; hev_task_execute is a frame-chained function in the canonical "alloca"
; form: the prologue saves <x29,lr> with a pre-indexed stp and sets
; x29 = sp.  The SP switch to the task stack cannot be expressed by ARM64
; unwind codes (those can only describe sub sp,sp,#imm style allocations),
; but for chained frames the unwinder does not use SP at all: it recovers
; the caller frame from the x29 chain (RtlVirtualUnwind applies set_fp as
; Sp = x29 and reads <x29,lr> back from [x29]/[x29+8]).  Because the task's
; compiler-generated frames preserve x29 (every function that modifies x29
; must save and have it restored by its own unwind codes), an exception
; unwinding out of task code reaches the address after `blr x1` with
; x29 pointing at the chain on the kernel stack and is *expected* to resume
; the walk in hev_task_system_run_new_task.  Unwinding mid-prologue and
; mid-epilogue is also described by the shared unwind codes.
;
; Pointer authentication (verified against the installed target UCRT)
; ----------------------------------------------------------------------------
; Disassembly of __intrinsic_setjmpex in the ARM64 vcruntime of the installed
; toolset (MSVC 14.51) shows that the value stored in jmp_buf->Lr (offset
; 0x68) is a PLAIN, unsigned return address: setjmp signs its own frame
; return address with pacibsp only so it can return to its caller, strips
; the signature again with xpaclri, stores the plain address together with
; the caller's SP (0x70), and finally re-authenticates its own LR with
; autibsp for its own ret.  UCRT's __longjmp_internal likewise passes the
; stored LR to RtlUnwind without authenticating it, and MSVC-generated
; ARM64 code does not sign return addresses at call sites.  hev_task_msvc_longjmp
; therefore restores LR and SP and returns directly, WITHOUT autibsp.
; Executing autibsp on a plain pointer would corrupt it on PAC-capable
; hardware (autibsp is only a no-op on cores without the feature); the
; previous unconditional autibsp relied on exactly that accident.
;
; Supported and unsupported unwinding scenarios
; ----------------------------------------------------------------------------
; * SEH/C++ exceptions raised inside task code are *expected* to unwind
;   through the task frames, through hev_task_execute (via the x29 chain)
;   and continue on the kernel stack, with destructors/finally blocks in
;   the task running normally.  This has NOT been validated: no native
;   Windows ARM64 runtime test has demonstrated exception propagation
;   across coroutine stacks yet.  Do not rely on it until then.
; * A context switch (this library's non-unwinding jmp_buf restore) never
;   runs unwind sequences, so C++ objects between the switch points are not
;   destroyed on switch - the same limitation as the x64/x86 backends.
; * hev_task_msvc_longjmp is a leaf for unwind purposes (no SP changes, no
;   saved nonvolatile registers) and is deliberately left without a .pdata
;   record; the unwinder takes the return address from LR directly.  The
;   only window where SP points at the task stack while PC is still inside
;   it spans the final `mov sp,x10` / `ret` pair, where no faulting
;   instruction exists.
; ============================================================================

    AREA |.text|, CODE, READONLY, ALIGN=2

    EXPORT hev_task_execute
hev_task_execute PROC
    stp  x29, x30, [sp, #-0x10]!    ; save_fplr_x: chain <x29,lr> (unwind: save_fplr_x)
    mov  x29, sp                    ; set_fp: x29 anchors the caller frame
    ldr  x3, [x0]                   ; x3 = task->stack_bottom (16-byte aligned)
    mov  sp, x3                     ; switch SP to the task stack
    blr  x1                         ; executer (x0 still holds self)
    mov  sp, x29                    ; epilogue: restore the kernel stack from x29
    ldp  x29, x30, [sp], #0x10      ; save_fplr_x reversed
    ret                             ; end
    ENDP

; .pdata: FunctionStartRVA + ExceptionInformationRVA (Flag = 00, full .xdata).
; armasm64 has no imagerel operator; DCD + RELOC 2 emits the ADDR32NB
; (image-relative) relocations that MSVC itself uses in .pdata records.
    AREA |.pdata|, DATA, READONLY, ALIGN=2
    DCD  hev_task_execute
    RELOC 2
    DCD  |$unwind$hev_task_execute|
    RELOC 2

; .xdata for hev_task_execute.
;   Function Length = 8 (32 bytes), Vers = 0, X = 0, E = 0,
;   Epilog Count = 1 (one scope), Code Words = 1.
;   Epilog scope: start offset 5 (the `mov sp,x29` at byte 0x14),
;   start index 0 (the epilogue mirrors the shared unwind codes).
;   Unwind codes (undo order, closest to the body first):
;     set_fp      (0xe1) - mov x29,sp
;     save_fplr_x (0x81) - stp x29,x30,[sp,#-0x10]!
;     end         (0xe4)
;   The single epilogue reuses the same code sequence.
    AREA |.xdata|, DATA, READONLY, ALIGN=2
|$unwind$hev_task_execute|
    DCD  0x08400008                 ; header: FuncLen 8, EpilogCount 1, CodeWords 1
    DCD  0x00000005                 ; epilog scope: offset 5, index 0
    DCD  0x00e481e1                 ; codes: set_fp, save_fplr_x(16), end, pad

; ============================================================================
; hev_task_msvc_longjmp - non-unwinding jmp_buf restore for native ARM64.
;
; x0 = env (the jmp_buf written by the UCRT _setjmp/_setjmpex), w1 = value.
;
; The UCRT longjmp() first calls __except_validate_jump_buffer(), which
; requires env->Sp to lie inside the *thread's* registered stack limits and
; env->Frame to be non-zero, then calls __longjmp_internal(), which invokes
; RtlUnwind.  A coroutine stack allocated on the heap is outside the thread's
; stack limits, so the UCRT path aborts.  The restore below therefore only
; reinstates the saved machine context and never unwinds.
;
; The layout was verified against the disassembly of the installed target
; UCRT __intrinsic_setjmpex and is guarded at compile time by
; src/arch/arm/hev-task-msvc-arm64-abi.c:
;
;    0x00 Frame     (env->Frame; ignored, we never unwind)
;    0x08 Reserved
;    0x10 X19      0x18 X20      0x20 X21      0x28 X22
;    0x30 X23      0x38 X24      0x40 X25      0x48 X26
;    0x50 X27      0x58 X28
;    0x60 Fp       (x29)
;    0x68 Lr       (x30; stored PLAIN/unsigned, see the PAC note above)
;    0x70 Sp       (x31 at the _setjmp call site)
;    0x78 Fpcr     (u32)
;    0x7C Fpsr     (u32)
;    0x80 D8      0x88 D9 ... 0xB8 D15   (nonvolatile low 64 bits of v8-v15)
;
; No pointer authentication is applied on restore: the saved Lr is unsigned
; (see the PAC note above), so `ret` returns through the unsigned call-site
; return address directly.  There is no signing modifier to reproduce.
; ============================================================================

    ; The longjmp routine is plain code again: switch back from the .xdata AREA.
    AREA |.text|, CODE, READONLY, ALIGN=2

    EXPORT hev_task_msvc_longjmp
hev_task_msvc_longjmp PROC
    mov  x9, x0                     ; x9 = env (jmp_buf)
    cmp  w1, #0
    csinc w1, w1, wzr, ne           ; value = (value == 0) ? 1 : value

    ldr  x19, [x9, #0x10]
    ldr  x20, [x9, #0x18]
    ldr  x21, [x9, #0x20]
    ldr  x22, [x9, #0x28]
    ldr  x23, [x9, #0x30]
    ldr  x24, [x9, #0x38]
    ldr  x25, [x9, #0x40]
    ldr  x26, [x9, #0x48]
    ldr  x27, [x9, #0x50]
    ldr  x28, [x9, #0x58]
    ldr  x29, [x9, #0x60]           ; frame pointer
    ldr  w10, [x9, #0x78]           ; FPCR
    ldr  w11, [x9, #0x7C]           ; FPSR
    ldr  d8,  [x9, #0x80]
    ldr  d9,  [x9, #0x88]
    ldr  d10, [x9, #0x90]
    ldr  d11, [x9, #0x98]
    ldr  d12, [x9, #0xA0]
    ldr  d13, [x9, #0xA8]
    ldr  d14, [x9, #0xB0]
    ldr  d15, [x9, #0xB8]
    msr  fpcr, x10
    msr  fpsr, x11

    ldr  x10, [x9, #0x70]           ; saved SP on the task stack
    ldr  x30, [x9, #0x68]           ; saved (plain, unsigned) LR
    mov  w0, w1                     ; _setjmp resumes with this value
    mov  sp, x10                    ; restore the task stack
    ret
    ENDP

    END
