/*
 ============================================================================
 Name        : asm.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 - 2025 everyone.
 Description :
 ============================================================================
 */

#if defined(__APPLE__) || defined(__MACH__)

# define NESTED(symbol) \
    .globl _##symbol; \
_##symbol:

# define END(symbol)

#elif defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__)

/* COFF has no ELF .type/.size directives.  Windows x64 symbols are not
 * decorated, but 32-bit COFF decorates __cdecl symbols with a leading
 * underscore (as the Apple branch above). */
#if defined(__i386__)
# define NESTED(symbol) \
    .globl _##symbol; \
_##symbol:
#else
# define NESTED(symbol) \
    .globl symbol; \
symbol:
#endif

# define END(symbol)

#elif defined(__MSYS__)

# define NESTED(symbol) \
    .globl symbol; \
symbol:

# define END(symbol)

#else

# define NESTED(symbol) \
    .globl symbol; \
    .type symbol, @function; \
symbol:

# define END(symbol) \
    .size symbol, . - symbol;

#endif
