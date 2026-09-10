#include "m68kdeb-68030-identity-window.h"
#include "m68kdeb-68030-mmu-state.h"

int m68kdeb_68030_identity_window_from_tc(
    uint32_t tc,
    uint32_t logical_base,
    uint32_t bytes,
    uint32_t trampoline_bytes,
    uint32_t alignment,
    uint32_t ram_base,
    uint32_t ram_bytes,
    struct m68kdeb_physwin_candidate *out)
{
    struct m68kdeb_physwin_candidate c;

    if (!out || !logical_base || !bytes)
        return M68KDEB_68030_IDWIN_BAD_ARGUMENT;

    if (tc & (uint32_t)M68KDEB_68030_TC_ENABLE)
        return M68KDEB_68030_IDWIN_TRANSLATION_ENABLED;

    c.logical_base = logical_base;
    c.physical_base = logical_base;
    c.bytes = bytes;
    c.trampoline_bytes = trampoline_bytes;
    c.alignment = alignment;
    c.ram_base = ram_base;
    c.ram_bytes = ram_bytes;

    if (m68kdeb_physwin_validate(&c) != M68KDEB_PHYSWIN_OK)
        return M68KDEB_68030_IDWIN_BAD_WINDOW;

    *out = c;
    return M68KDEB_68030_IDWIN_OK;
}
