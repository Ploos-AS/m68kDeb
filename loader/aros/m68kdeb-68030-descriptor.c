#include "m68kdeb-68030-descriptor.h"

int m68kdeb_68030_decode_terminal_page(uint32_t logical,
                                       unsigned page_shift,
                                       enum m68kdeb_68030_page_format format,
                                       uint32_t word0,
                                       uint32_t word1,
                                       struct m68kdeb_68030_page_result *out)
{
    uint32_t page_mask;
    uint32_t address_word;
    uint16_t status;
    uint8_t dt;

    if (!out)
        return M68KDEB_68030_DESC_BAD_ARGUMENT;

    /* MC68030 page descriptors reserve the low status byte. */
    if (page_shift < 8u || page_shift > 31u)
        return M68KDEB_68030_DESC_BAD_PAGE_SHIFT;

    if (format != M68KDEB_68030_PAGE_SHORT &&
        format != M68KDEB_68030_PAGE_LONG)
        return M68KDEB_68030_DESC_BAD_ARGUMENT;

    status = (uint16_t)(word0 & 0xffffu);
    dt = (uint8_t)(status & 0x3u);

    /* A terminal page descriptor uses DT=01. Pointer/indirect forms are rejected. */
    if (dt != 1u)
        return M68KDEB_68030_DESC_NOT_PAGE;

    address_word = (format == M68KDEB_68030_PAGE_SHORT) ? word0 : word1;
    page_mask = ~((1u << page_shift) - 1u);

    out->logical = logical;
    out->page_offset = logical & ~page_mask;
    out->page_base = address_word & page_mask;
    out->physical = out->page_base | out->page_offset;
    out->status = status;
    out->descriptor_type = dt;
    out->format = (uint8_t)format;

    return M68KDEB_68030_DESC_OK;
}
