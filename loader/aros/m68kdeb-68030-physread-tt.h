#ifndef M68KDEB_68030_PHYSREAD_TT_H
#define M68KDEB_68030_PHYSREAD_TT_H

#include <stdint.h>

#define M68KDEB_68030_TTREAD_OK                0
#define M68KDEB_68030_TTREAD_BAD_ARGUMENT      1
#define M68KDEB_68030_TTREAD_NOT_TRANSPARENT   2
#define M68KDEB_68030_TTREAD_TRANSLATION_FAULT 3
#define M68KDEB_68030_TTREAD_RANGE_OVERFLOW    4
#define M68KDEB_68030_TTREAD_NOT_IDENTITY      5

struct m68kdeb_68030_tt_read_proof {
    uint32_t logical;
    uint32_t physical;
    uint32_t bytes;
    uint16_t psr;
    uint8_t transparent;
    uint8_t identity;
};

int m68kdeb_68030_tt_read_admit(uint32_t logical,
                                uint32_t physical,
                                uint32_t bytes,
                                uint16_t psr,
                                struct m68kdeb_68030_tt_read_proof *out);

#endif
