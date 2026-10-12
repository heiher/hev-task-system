/* Compile-time ABI guards for the handwritten Win32 x86 MASM restore. */
#if defined(_MSC_VER) && defined(_M_IX86)
#include <stddef.h>
#include <setjmp.h>
_Static_assert(sizeof(void *) == 4, "Win32 pointers must be 32 bits");
_Static_assert(offsetof(_JUMP_BUFFER, Ebp) == 0, "x86 jmp_buf Ebp changed");
_Static_assert(offsetof(_JUMP_BUFFER, Ebx) == 4, "x86 jmp_buf Ebx changed");
_Static_assert(offsetof(_JUMP_BUFFER, Edi) == 8, "x86 jmp_buf Edi changed");
_Static_assert(offsetof(_JUMP_BUFFER, Esi) == 12, "x86 jmp_buf Esi changed");
_Static_assert(offsetof(_JUMP_BUFFER, Esp) == 16, "x86 jmp_buf Esp changed");
_Static_assert(offsetof(_JUMP_BUFFER, Eip) == 20, "x86 jmp_buf Eip changed");
_Static_assert(offsetof(_JUMP_BUFFER, Registration) == 24, "x86 jmp_buf SEH changed");
_Static_assert(sizeof(jmp_buf) >= 64, "x86 jmp_buf is unexpectedly short");
#endif
