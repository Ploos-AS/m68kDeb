#ifndef M68KDEB_PHYSWIN_H
#define M68KDEB_PHYSWIN_H

#include <stdint.h>

#define M68KDEB_PHYSWIN_OK                 0
#define M68KDEB_PHYSWIN_BAD_ARGUMENT       1
#define M68KDEB_PHYSWIN_NOT_IDENTITY       2
#define M68KDEB_PHYSWIN_BAD_ALIGNMENT      3
#define M68KDEB_PHYSWIN_TOO_SMALL          4
#define M68KDEB_PHYSWIN_OVERFLOW           5
#define M68KDEB_PHYSWIN_OUTSIDE_RAM        6

struct m68kdeb_physwin_candidate {
    uint32_t logical_base;
    uint32_t physical_base;
    uint32_t bytes;
    uint32_t trampoline_bytes;
    uint32_t alignment;
    uint32_t ram_base;
    uint32_t ram_bytes;
};

int m68kdeb_physwin_validate(const struct m68kdeb_physwin_candidate *c);

#endif
