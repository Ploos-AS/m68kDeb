#include "m68kdeb-68030-evidence.h"

int m68kdeb_68030_resolve_terminal_evidence(
    uint32_t logical,
    uint16_t psr,
    uint32_t last_descriptor_phys,
    unsigned page_shift,
    enum m68kdeb_68030_page_format format,
    uint32_t descriptor_word0,
    uint32_t descriptor_word1,
    struct m68kdeb_68030_translation_evidence *out)
{
    unsigned levels;
    int rc;

    if (!out || !last_descriptor_phys)
        return M68KDEB_68030_EVIDENCE_BAD_ARGUMENT;

    if (psr & (M68KDEB_68030_PSR_BUS_ERROR |
               M68KDEB_68030_PSR_LIMIT |
               M68KDEB_68030_PSR_SUPERVISOR |
               M68KDEB_68030_PSR_INVALID))
        return M68KDEB_68030_EVIDENCE_TRANSLATION_FAULT;

    levels = (unsigned)(psr & M68KDEB_68030_PSR_LEVELS_MASK);
    if (!levels)
        return M68KDEB_68030_EVIDENCE_NO_LEVELS;

    rc = m68kdeb_68030_decode_terminal_page(logical,
                                             page_shift,
                                             format,
                                             descriptor_word0,
                                             descriptor_word1,
                                             &out->page);
    if (rc != M68KDEB_68030_DESC_OK)
        return M68KDEB_68030_EVIDENCE_BAD_DESCRIPTOR;

    out->logical = logical;
    out->physical = out->page.physical;
    out->last_descriptor_phys = last_descriptor_phys;
    out->psr = psr;
    out->levels = (uint8_t)levels;
    out->write_protected =
        (psr & M68KDEB_68030_PSR_WRITE_PROTECTED) ? 1u : 0u;
    out->modified = (psr & M68KDEB_68030_PSR_MODIFIED) ? 1u : 0u;
    out->reserved = 0u;

    return M68KDEB_68030_EVIDENCE_OK;
}
