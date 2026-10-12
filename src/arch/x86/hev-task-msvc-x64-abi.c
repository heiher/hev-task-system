/* Compile-time ABI guards for the existing Windows x64 MASM restore. */
#if defined(_MSC_VER) && defined(_M_X64)
#include <stddef.h>
#include <setjmp.h>
_Static_assert(sizeof(void *) == 8, "Win64 pointers must be 64 bits");
_Static_assert(offsetof(_JUMP_BUFFER, Rbx) == 8, "x64 jmp_buf Rbx changed");
_Static_assert(offsetof(_JUMP_BUFFER, Rsp) == 16, "x64 jmp_buf Rsp changed");
_Static_assert(offsetof(_JUMP_BUFFER, Rbp) == 24, "x64 jmp_buf Rbp changed");
_Static_assert(offsetof(_JUMP_BUFFER, Rsi) == 32, "x64 jmp_buf Rsi changed");
_Static_assert(offsetof(_JUMP_BUFFER, Rdi) == 40, "x64 jmp_buf Rdi changed");
_Static_assert(offsetof(_JUMP_BUFFER, R12) == 48, "x64 jmp_buf R12 changed");
_Static_assert(offsetof(_JUMP_BUFFER, R13) == 56, "x64 jmp_buf R13 changed");
_Static_assert(offsetof(_JUMP_BUFFER, R14) == 64, "x64 jmp_buf R14 changed");
_Static_assert(offsetof(_JUMP_BUFFER, R15) == 72, "x64 jmp_buf R15 changed");
_Static_assert(offsetof(_JUMP_BUFFER, Rip) == 80, "x64 jmp_buf Rip changed");
_Static_assert(offsetof(_JUMP_BUFFER, MxCsr) == 88, "x64 jmp_buf MxCsr changed");
_Static_assert(offsetof(_JUMP_BUFFER, FpCsr) == 92, "x64 jmp_buf FpCsr changed");
_Static_assert(offsetof(_JUMP_BUFFER, Xmm6) == 96, "x64 jmp_buf Xmm6 changed");
_Static_assert(offsetof(_JUMP_BUFFER, Xmm15) == 240, "x64 jmp_buf Xmm15 changed");
#endif
