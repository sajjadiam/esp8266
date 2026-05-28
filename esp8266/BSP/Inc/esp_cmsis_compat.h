#ifndef ESP_CMSIS_COMPAT_H
#define ESP_CMSIS_COMPAT_H

/* CMSIS cmsis_compiler.h only supports armcc/armclang/GCC/IAR/TI.
 * Cursor/clangd may use a Clang front-end without __GNUC__ (e.g. clang-cl). */
#if defined(__clang__) && !defined(__GNUC__) && !defined(__ARMCC_VERSION) && \
    !defined(__CC_ARM) && !defined(__ICCARM__)
#define __GNUC__
#endif

#endif
