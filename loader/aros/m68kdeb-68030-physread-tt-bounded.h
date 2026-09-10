#ifndef M68KDEB_68030_PHYSREAD_TT_BOUNDED_H
#define M68KDEB_68030_PHYSREAD_TT_BOUNDED_H

#include <stdint.h>
#include "m68kdeb-68030-physread-tt.h"

#define M68KDEB_68030_TTBOUNDED_OK              0
#define M68KDEB_68030_TTBOUNDED_BAD_ARGUMENT    1
#define M68KDEB_68030_TTBOUNDED_BAD_PROOF       2
#define M68KDEB_68030_TTBOUNDED_BAD_SIZE        3
#define M68KDEB_68030_TTBOUNDED_ADDRESS_MISMATCH 4

int m68kdeb_68030_tt_bounded_validate(
    const struct m68kdeb_68030_tt_read_proof *proof,
    uint32_t physical,
    uint32_t bytes);

int m68kdeb_68030_tt_bounded_read(
    const struct m68kdeb_68030_tt_read_proof *proof,
    uint32_t physical,
    void *dst,
    uint32_t bytes);

#endif
