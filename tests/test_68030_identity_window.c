#include <stdio.h>
#include "../loader/aros/m68kdeb-68030-identity-window.h"

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
    struct m68kdeb_physwin_candidate out;
    int failed = 0;

    failed |= expect(m68kdeb_68030_identity_window_from_tc(
        0x00000000u, 0x00200000u, 0x00001000u, 512u, 4096u,
        0x00000000u, 0x01000000u, &out),
        M68KDEB_68030_IDWIN_OK, "translation disabled identity window");
    failed |= expect(out.logical_base == out.physical_base,
                     1, "logical equals physical");

    failed |= expect(m68kdeb_68030_identity_window_from_tc(
        0x80000000u, 0x00200000u, 0x00001000u, 512u, 4096u,
        0x00000000u, 0x01000000u, &out),
        M68KDEB_68030_IDWIN_TRANSLATION_ENABLED,
        "translation enabled rejected");

    failed |= expect(m68kdeb_68030_identity_window_from_tc(
        0x00000000u, 0x00200001u, 0x00001000u, 512u, 4096u,
        0x00000000u, 0x01000000u, &out),
        M68KDEB_68030_IDWIN_BAD_WINDOW, "misaligned window rejected");

    failed |= expect(m68kdeb_68030_identity_window_from_tc(
        0x00000000u, 0x00fff000u, 0x00002000u, 512u, 4096u,
        0x00000000u, 0x01000000u, &out),
        M68KDEB_68030_IDWIN_BAD_WINDOW, "outside ram rejected");

    if (failed)
        return 1;

    puts("M68KDEB_68030_IDWIN_TEST_PASS");
    return 0;
}
