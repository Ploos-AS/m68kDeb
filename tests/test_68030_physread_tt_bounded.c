#include <stdio.h>
#include "../loader/aros/m68kdeb-68030-physread-tt-bounded.h"

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
    struct m68kdeb_68030_tt_read_proof proof;
    int failed = 0;

    proof.logical = 0x00200000u;
    proof.physical = 0x00200000u;
    proof.bytes = 4u;
    proof.psr = 0x0040u;
    proof.transparent = 1u;
    proof.identity = 1u;

    failed |= expect(m68kdeb_68030_tt_bounded_validate(
        &proof, 0x00200000u, 4u),
        M68KDEB_68030_TTBOUNDED_OK, "valid 4-byte proof");

    proof.bytes = 8u;
    failed |= expect(m68kdeb_68030_tt_bounded_validate(
        &proof, 0x00200000u, 8u),
        M68KDEB_68030_TTBOUNDED_OK, "valid 8-byte proof");

    failed |= expect(m68kdeb_68030_tt_bounded_validate(
        &proof, 0x00200000u, 6u),
        M68KDEB_68030_TTBOUNDED_BAD_SIZE, "reject non descriptor size");

    failed |= expect(m68kdeb_68030_tt_bounded_validate(
        &proof, 0x00201000u, 8u),
        M68KDEB_68030_TTBOUNDED_ADDRESS_MISMATCH, "reject physical mismatch");

    proof.identity = 0u;
    failed |= expect(m68kdeb_68030_tt_bounded_validate(
        &proof, 0x00200000u, 8u),
        M68KDEB_68030_TTBOUNDED_BAD_PROOF, "reject non identity proof");

    proof.identity = 1u;
    proof.transparent = 0u;
    failed |= expect(m68kdeb_68030_tt_bounded_validate(
        &proof, 0x00200000u, 8u),
        M68KDEB_68030_TTBOUNDED_BAD_PROOF, "reject non transparent proof");

    if (failed)
        return 1;

    puts("M68KDEB_68030_TTBOUNDED_TEST_PASS");
    return 0;
}
