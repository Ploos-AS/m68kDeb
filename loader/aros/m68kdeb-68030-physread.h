#ifndef M68KDEB_68030_PHYSREAD_H
#define M68KDEB_68030_PHYSREAD_H

#include <stdint.h>
#include "m68kdeb-68030-evidence.h"

#define M68KDEB_68030_PHYSREAD_OK             0
#define M68KDEB_68030_PHYSREAD_BAD_ARGUMENT   1
#define M68KDEB_68030_PHYSREAD_UNALIGNED      2
#define M68KDEB_68030_PHYSREAD_READER_FAILED  3
#define M68KDEB_68030_PHYSREAD_BAD_EVIDENCE   4

typedef int (*m68kdeb_68030_physread_fn)(void *ctx,
                                         uint32_t physical,
                                         void *dst,
                                         uint32_t bytes);

int m68kdeb_68030_bind_terminal_descriptor(
    uint32_t logical,
    uint16_t psr,
    uint32_t last_descriptor_phys,
    unsigned page_shift,
    enum m68kdeb_68030_page_format format,
    m68kdeb_68030_physread_fn reader,
    void *reader_ctx,
    struct m68kdeb_68030_translation_evidence *out);

#endif
