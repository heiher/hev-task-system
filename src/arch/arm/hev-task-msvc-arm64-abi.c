/*
 * Compile-time ABI guards for the handwritten Windows ARM64 context restore.
 *
 * These checks deliberately reference the *installed target* MSVC/UCRT
 * <setjmp.h> _JUMP_BUFFER definition rather than a copied Linux/x64 layout.
 * If a future toolset changes any field the assembly accesses, the ARM64
 * library build fails here instead of silently corrupting task contexts.
 *
 * This file is only meaningful for genuine native ARM64.  ARM64EC has a
 * different (x64-compatible) jmp_buf layout and unwind model and is rejected
 * in hev-task-system-private.h.
 */
#if defined(_MSC_VER) && defined(_M_ARM64) && !defined(_M_ARM64EC)

#include <stddef.h>
#include <setjmp.h>

_Static_assert (sizeof (void *) == 8, "ARM64 pointers must be 64 bits");
_Static_assert (sizeof (_JUMP_BUFFER) == 192, "ARM64 _JUMP_BUFFER size changed");
_Static_assert (sizeof (jmp_buf) == 192, "ARM64 jmp_buf size changed");

_Static_assert (offsetof (_JUMP_BUFFER, Frame) == 0, "ARM64 Frame changed");
_Static_assert (offsetof (_JUMP_BUFFER, Reserved) == 8, "ARM64 Reserved changed");

_Static_assert (offsetof (_JUMP_BUFFER, X19) == 16, "ARM64 X19 changed");
_Static_assert (offsetof (_JUMP_BUFFER, X20) == 24, "ARM64 X20 changed");
_Static_assert (offsetof (_JUMP_BUFFER, X21) == 32, "ARM64 X21 changed");
_Static_assert (offsetof (_JUMP_BUFFER, X22) == 40, "ARM64 X22 changed");
_Static_assert (offsetof (_JUMP_BUFFER, X23) == 48, "ARM64 X23 changed");
_Static_assert (offsetof (_JUMP_BUFFER, X24) == 56, "ARM64 X24 changed");
_Static_assert (offsetof (_JUMP_BUFFER, X25) == 64, "ARM64 X25 changed");
_Static_assert (offsetof (_JUMP_BUFFER, X26) == 72, "ARM64 X26 changed");
_Static_assert (offsetof (_JUMP_BUFFER, X27) == 80, "ARM64 X27 changed");
_Static_assert (offsetof (_JUMP_BUFFER, X28) == 88, "ARM64 X28 changed");

_Static_assert (offsetof (_JUMP_BUFFER, Fp) == 96, "ARM64 Fp changed");
_Static_assert (offsetof (_JUMP_BUFFER, Lr) == 104, "ARM64 Lr changed");
_Static_assert (offsetof (_JUMP_BUFFER, Sp) == 112, "ARM64 Sp changed");
_Static_assert (offsetof (_JUMP_BUFFER, Fpcr) == 120, "ARM64 Fpcr changed");
_Static_assert (offsetof (_JUMP_BUFFER, Fpsr) == 124, "ARM64 Fpsr changed");

_Static_assert (offsetof (_JUMP_BUFFER, D) == 128, "ARM64 D8 changed");
_Static_assert (sizeof (((_JUMP_BUFFER *)0)->D) == 64, "ARM64 D8-D15 size changed");

#endif /* _MSC_VER && _M_ARM64 && !_M_ARM64EC */
