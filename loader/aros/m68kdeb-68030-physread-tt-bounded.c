#include <stdint.h>
#include "m68kdeb-68030-physread-tt-bounded.h"

int m68kdeb_68030_tt_bounded_validate(
    const struct m68kdeb_68030_tt_read_proof *proof,
    uint32_t physical,
    uint32_t bytes)
{
    if (!proof)
        return M68KDEB_68030_TTBOUNDED_BAD_ARGUMENT;

    if (!proof->transparent || !proof->identity ||
        proof->logical != proof->physical)
        return M68KDEB_68030_TTBOUNDED_BAD_PROOF;

    if (bytes != 4u && bytes != 8u)
        return M68KDEB_68030_TTBOUNDED_BAD_SIZE;

    if (proof->bytes != bytes || proof->physical != physical)
        return M68KDEB_68030_TTBOUNDED_ADDRESS_MISMATCH;

    return M68KDEB_68030_TTBOUNDED_OK;
}

int m68kdeb_68030_tt_bounded_read(
    const struct m68kdeb_68030_tt_read_proof *proof,
    uint32_t physical,
    void *dst,
    uint32_t bytes)
{
    const volatile unsigned char *src;
    unsigned char *out;
    uint32_t i;
    int rc;

    if (!dst)
        return M68KDEB_68030_TTBOUNDED_BAD_ARGUMENT;

    rc = m68kdeb_68030_tt_bounded_validate(proof, physical, bytes);
    if (rc != M68KDEB_68030_TTBOUNDED_OK)
        return rc;

    /*
     * The pointer conversion is permitted only after 5l supplied an exact
     * transparent-translation identity proof for this address and span.
     */
    src = (const volatile unsigned char *)(uintptr_t)proof->logical;
    out = (unsigned char *)dst;

    for (i = 0; i < bytes; ++i)
        out[i] = src[i];

    return M68KDEB_68030_TTBOUNDED_OK;
}
