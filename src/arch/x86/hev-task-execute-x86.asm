; Native Win32/x86 coroutine entry and non-unwinding jmp_buf restore.
; This is NOT the Windows x64 ABI.  x86 __cdecl arguments are on the stack,
; with 32-bit callee-saved EBX/ESI/EDI/EBP and a thread-local SEH chain.
; The offsets below are verified by hev-task-msvc-x86-abi.c at compile time.
;
; IMPORTANT: This is a new, UNVERIFIED runtime port. Test on native Win32
; with MSVC, including repeated coroutine switches and SEH behavior.

.386
.model flat
; Explicit leading underscores below are the MSVC x86 __cdecl COFF names.
; Do NOT enable MASM C language mangling as it may double-decorate them.
option casemap:none
.code

PUBLIC _hev_task_execute
_hev_task_execute PROC
    mov eax, esp                     ; original caller stack (return address)
    mov edx, DWORD PTR [esp+4]       ; HevTask *self
    mov ecx, DWORD PTR [esp+8]       ; void (*executer)(HevTask *)
    mov esp, DWORD PTR [edx]         ; task->stack_bottom (offset 0)
    sub esp, 8                       ; match upstream Win32 x86 stack setup
    push eax                         ; preserve original ESP
    push edx                         ; __cdecl executer(self)
    call ecx
    mov esp, DWORD PTR [esp+4]       ; restore original ESP
    ret
_hev_task_execute ENDP

PUBLIC _hev_task_msvc_longjmp
_hev_task_msvc_longjmp PROC
    mov edx, DWORD PTR [esp+4]       ; _JUMP_BUFFER *env
    mov eax, DWORD PTR [esp+8]       ; int value
    test eax, eax
    jnz jump_nonzero
    inc eax                          ; longjmp(env, 0) returns 1
jump_nonzero:
    ; The CRT _longjmp may unwind through a *different* stack. Restore
    ; only the saved execution context (no cross-stack unwinding).
    ; jmp_buf x86 (32-bit): Ebp=0 Ebx=4 Edi=8 Esi=12 Esp=16 Eip=20
    ; and the Win32 x86 SEH registration chain head at offset 24.
    mov ebp, DWORD PTR [edx+0]
    mov ebx, DWORD PTR [edx+4]
    mov edi, DWORD PTR [edx+8]
    mov esi, DWORD PTR [edx+12]
    mov ecx, DWORD PTR [edx+24]
    ; MASM assumes FS:ERROR by default. Temporarily permit the
    ; Windows x86 TEB/SEH chain restoration, then re-enable checking.
    ASSUME FS:NOTHING
    mov DWORD PTR fs:[0], ecx
    ASSUME FS:ERROR
    mov esp, DWORD PTR [edx+16]
    jmp DWORD PTR [edx+20]
_hev_task_msvc_longjmp ENDP

END
