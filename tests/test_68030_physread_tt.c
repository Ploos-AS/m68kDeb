#include <stdint.h>
#include <stdio.h>
#include "../loader/aros/m68kdeb-68030-physread-tt.h"
#include "../loader/aros/m68kdeb-68030-evidence.h"

static int expect(int got, int want, const char *name)
{
    if (got != want) {
        fprintf(stderr, "%s: got %d want %d\n", name, got, want);
        return 1;
    }
    return 0;
}

int main(void)
{
    struct m68kdeb_68030_tt_read_proof out;
    int failed = 0;

    failed |= expect(m68kdeb_68030_tt_read_admit(
        0x00200000u, 0x00200000u, 8u,
        M68KDEB_68030_PSR_TRANSPARENT, &out),
        M68KDEB_68030_TTREAD_OK, "valid transparent identity");

    failed |= expect(m68kdeb_68030_tt_read_admit(
        0x00200000u, 0x00201000u, 8u,
        M68KDEB_68030_PSR_TRANSPARENT, &out),
        M68KDEB_68030_TTREAD_NOT_IDENTITY, "non identity");

    failed |= expect(m68kdeb_68030_tt_read_admit(
        0x00200000u, 0x00200000u, 8u, 0u, &out),
        M68KDEB_68030_TTREAD_NOT_TRANSPARENT, "not transparent");

    failed |= expect(m68kdeb_68030_tt_read_admit(
        0x00200000u, 0x00200000u, 8u,
        M68KDEB_68030_PSR_TRANSPARENT | M68KDEB_68030_PSR_INVALID, &out),
        M68KDEB_68030_TTREAD_TRANSLATION_FAULT, "invalid translation");

    failed |= expect(m68kdeb_68030_tt_read_admit(
        0xfffffffcu, 0xfffffffcu, 8u,
        M68KDEB_68030_PSR_TRANSPARENT, &out),
        M68KDEB_68030_TTREAD_RANGE_OVERFLOW, "range overflow");

    failed |= expect(m68kdeb_68030_tt_read_admit(
        0x00200000u, 0x00200000u, 0u,
        M68KDEB_68030_PSR_TRANSPARENT, &out),
        M68KDEB_68030_TTREAD_BAD_ARGUMENT, "zero bytes");

    if (failed)
        return 1;

    puts("M68KDEB_68030_TTREAD_TEST_PASS");
    return 0;
}
