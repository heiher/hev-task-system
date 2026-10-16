; ============================================================================
; hev-task-execute-x86_64.asm - native MASM context switch for hev-task-system
;
; void hev_task_execute (HevTask *self, void *executer);
;
; Windows x64 calling convention:
;   RCX = self (points to task; [RCX] is the task stack bottom pointer)
;   RDX = executer (entry function called with RCX = self)
;
; Semantics (matches the GNU as version):
;   1. save current RSP
;   2. switch RSP to the task stack
;   3. save previous RSP on the task stack
;   4. reserve Windows shadow space
;   5. call executer
;   6. restore previous RSP and return
; ============================================================================

.code

hev_task_execute proc
    mov     rax, rsp                ; save current RSP
    mov     rsp, [rcx]              ; switch to task stack
    push    rax                     ; save previous RSP on task stack
    sub     rsp, 28h                ; reserve shadow space (32) + alignment
    call    rdx                     ; executer (RCX still holds self)
    mov     rsp, [rsp+28h]          ; restore previous RSP
    ret
hev_task_execute endp

; ============================================================================
; hev_task_msvc_longjmp (void *env, int val)
;
; RCX = env (jmp_buf saved by _setjmp), RDX = val
;
; The UCRT longjmp() calls RtlUnwind before restoring, which aborts with
; STATUS_BAD_STACK (0xC00000FF) whenever the target stack is a coroutine
; stack allocated on the heap (outside the thread's registered stack
; limits). Task switching in hev-task-system jumps between such stacks,
; so the restore below replicates the UCRT non-unwind fast path
; (register file + XMM6-15 + MXCSR/x87 control word) and never unwinds.
; The binary is linked with /CETCOMPAT:NO because a bare jmp cannot
; maintain a hardware shadow stack.
; ============================================================================
hev_task_msvc_longjmp proc
    test    rdx, rdx
    jne     notzero
    inc     rdx
notzero:
    mov     rax, rdx
    mov     rbx, qword ptr [rcx+08h]
    mov     rsi, qword ptr [rcx+20h]
    mov     rdi, qword ptr [rcx+28h]
    mov     r12, qword ptr [rcx+30h]
    mov     r13, qword ptr [rcx+38h]
    mov     r14, qword ptr [rcx+40h]
    mov     r15, qword ptr [rcx+48h]
    ldmxcsr dword ptr [rcx+58h]
    fnclex
    fldcw   word ptr [rcx+5Ch]
    movdqa  xmm6,  xmmword ptr [rcx+60h]
    movdqa  xmm7,  xmmword ptr [rcx+70h]
    movdqa  xmm8,  xmmword ptr [rcx+80h]
    movdqa  xmm9,  xmmword ptr [rcx+90h]
    movdqa  xmm10, xmmword ptr [rcx+0A0h]
    movdqa  xmm11, xmmword ptr [rcx+0B0h]
    movdqa  xmm12, xmmword ptr [rcx+0C0h]
    movdqa  xmm13, xmmword ptr [rcx+0D0h]
    movdqa  xmm14, xmmword ptr [rcx+0E0h]
    movdqa  xmm15, xmmword ptr [rcx+0F0h]
    mov     rdx, qword ptr [rcx+50h]
    mov     rbp, qword ptr [rcx+18h]
    mov     rsp, qword ptr [rcx+10h]
    jmp     rdx
hev_task_msvc_longjmp endp

end
