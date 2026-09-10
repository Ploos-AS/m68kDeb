#include "m68kdeb-physwin.h"

static int add_overflows(uint32_t a, uint32_t b)
{
    return b > 0xffffffffu - a;
}

int m68kdeb_physwin_validate(const struct m68kdeb_physwin_candidate *c)
{
    uint32_t end;
    uint32_t ram_end;

    if (!c || !c->bytes || !c->trampoline_bytes || !c->alignment ||
        !c->ram_bytes)
        return M68KDEB_PHYSWIN_BAD_ARGUMENT;

    if (c->logical_base != c->physical_base)
        return M68KDEB_PHYSWIN_NOT_IDENTITY;

    if ((c->alignment & (c->alignment - 1u)) != 0u ||
        (c->logical_base & (c->alignment - 1u)) != 0u)
        return M68KDEB_PHYSWIN_BAD_ALIGNMENT;

    if (c->bytes < c->trampoline_bytes)
        return M68KDEB_PHYSWIN_TOO_SMALL;

    if (add_overflows(c->logical_base, c->bytes) ||
        add_overflows(c->ram_base, c->ram_bytes))
        return M68KDEB_PHYSWIN_OVERFLOW;

    end = c->logical_base + c->bytes;
    ram_end = c->ram_base + c->ram_bytes;

    if (c->logical_base < c->ram_base || end > ram_end)
        return M68KDEB_PHYSWIN_OUTSIDE_RAM;

    return M68KDEB_PHYSWIN_OK;
}
