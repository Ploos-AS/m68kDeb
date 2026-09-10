#include <assert.h>
#include <stdio.h>

#include "../loader/aros/m68kdeb-physwin.h"

static struct m68kdeb_physwin_candidate good(void)
{
    struct m68kdeb_physwin_candidate c;
    c.logical_base = 0x00100000u;
    c.physical_base = 0x00100000u;
    c.bytes = 4096u;
    c.trampoline_bytes = 128u;
    c.alignment = 4096u;
    c.ram_base = 0x00100000u;
    c.ram_bytes = 0x01000000u;
    return c;
}

int main(void)
{
    struct m68kdeb_physwin_candidate c = good();

    assert(m68kdeb_physwin_validate(&c) == M68KDEB_PHYSWIN_OK);

    c = good(); c.physical_base += 4096u;
    assert(m68kdeb_physwin_validate(&c) == M68KDEB_PHYSWIN_NOT_IDENTITY);

    c = good(); c.logical_base += 2u; c.physical_base = c.logical_base;
    assert(m68kdeb_physwin_validate(&c) == M68KDEB_PHYSWIN_BAD_ALIGNMENT);

    c = good(); c.bytes = 64u;
    assert(m68kdeb_physwin_validate(&c) == M68KDEB_PHYSWIN_TOO_SMALL);

    c = good(); c.logical_base = 0xfffff000u; c.physical_base = c.logical_base; c.bytes = 8192u;
    assert(m68kdeb_physwin_validate(&c) == M68KDEB_PHYSWIN_OVERFLOW);

    c = good(); c.logical_base = 0x02000000u; c.physical_base = c.logical_base;
    assert(m68kdeb_physwin_validate(&c) == M68KDEB_PHYSWIN_OUTSIDE_RAM);

    puts("M68KDEB_PHYSWIN_TEST_PASS");
    return 0;
}
