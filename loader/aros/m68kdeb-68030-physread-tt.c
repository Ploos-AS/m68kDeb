#include "m68kdeb-68030-physread-tt.h"
#include "m68kdeb-68030-evidence.h"

int m68kdeb_68030_tt_read_admit(uint32_t logical,
                                uint32_t physical,
                                uint32_t bytes,
                                uint16_t psr,
                                struct m68kdeb_68030_tt_read_proof *out)
{
    uint32_t end;

    if (!out || !bytes)
        return M68KDEB_68030_TTREAD_BAD_ARGUMENT;

    if (psr & (M68KDEB_68030_PSR_BUS_ERROR |
               M68KDEB_68030_PSR_LIMIT |
               M68KDEB_68030_PSR_SUPERVISOR |
               M68KDEB_68030_PSR_INVALID))
        return M68KDEB_68030_TTREAD_TRANSLATION_FAULT;

    if (!(psr & M68KDEB_68030_PSR_TRANSPARENT))
        return M68KDEB_68030_TTREAD_NOT_TRANSPARENT;

    if (logical != physical)
        return M68KDEB_68030_TTREAD_NOT_IDENTITY;

    end = logical + bytes - 1u;
    if (end < logical)
        return M68KDEB_68030_TTREAD_RANGE_OVERFLOW;

    out->logical = logical;
    out->physical = physical;
    out->bytes = bytes;
    out->psr = psr;
    out->transparent = 1u;
    out->identity = 1u;

    return M68KDEB_68030_TTREAD_OK;
}
