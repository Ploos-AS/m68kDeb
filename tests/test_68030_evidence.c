#include <assert.h>
#include <stdio.h>

#include "../loader/aros/m68kdeb-68030-evidence.h"

int main(void)
{
    struct m68kdeb_68030_translation_evidence out;
    int rc;

    rc = m68kdeb_68030_resolve_terminal_evidence(
        0x00123abcu, 0x0003u, 0x00008000u, 12u,
        M68KDEB_68030_PAGE_SHORT,
        0x00456001u, 0u, &out);
    assert(rc == M68KDEB_68030_EVIDENCE_OK);
    assert(out.logical == 0x00123abcu);
    assert(out.physical == 0x00456abcu);
    assert(out.last_descriptor_phys == 0x00008000u);
    assert(out.levels == 3u);

    rc = m68kdeb_68030_resolve_terminal_evidence(
        0x1000u, M68KDEB_68030_PSR_INVALID | 1u, 0x2000u, 12u,
        M68KDEB_68030_PAGE_SHORT, 0x3001u, 0u, &out);
    assert(rc == M68KDEB_68030_EVIDENCE_TRANSLATION_FAULT);

    rc = m68kdeb_68030_resolve_terminal_evidence(
        0x1000u, 0u, 0x2000u, 12u,
        M68KDEB_68030_PAGE_SHORT, 0x3001u, 0u, &out);
    assert(rc == M68KDEB_68030_EVIDENCE_NO_LEVELS);

    rc = m68kdeb_68030_resolve_terminal_evidence(
        0x1000u, 1u, 0u, 12u,
        M68KDEB_68030_PAGE_SHORT, 0x3001u, 0u, &out);
    assert(rc == M68KDEB_68030_EVIDENCE_BAD_ARGUMENT);

    rc = m68kdeb_68030_resolve_terminal_evidence(
        0x1000u, 1u, 0x2000u, 12u,
        M68KDEB_68030_PAGE_SHORT, 0x3002u, 0u, &out);
    assert(rc == M68KDEB_68030_EVIDENCE_BAD_DESCRIPTOR);

    printf("M68KDEB_68030_EVIDENCE_TEST_PASS\n");
    return 0;
}
