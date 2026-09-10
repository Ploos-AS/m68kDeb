#include "m68kdeb-68030-physread.h"

static uint32_t be32(const unsigned char *p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           (uint32_t)p[3];
}

int m68kdeb_68030_bind_terminal_descriptor(
    uint32_t logical,
    uint16_t psr,
    uint32_t last_descriptor_phys,
    unsigned page_shift,
    enum m68kdeb_68030_page_format format,
    m68kdeb_68030_physread_fn reader,
    void *reader_ctx,
    struct m68kdeb_68030_translation_evidence *out)
{
    unsigned char raw[8];
    uint32_t word0;
    uint32_t word1 = 0;
    uint32_t bytes;
    int rc;

    if (!reader || !out || !last_descriptor_phys)
        return M68KDEB_68030_PHYSREAD_BAD_ARGUMENT;

    if (last_descriptor_phys & 3u)
        return M68KDEB_68030_PHYSREAD_UNALIGNED;

    if (format == M68KDEB_68030_PAGE_SHORT)
        bytes = 4u;
    else if (format == M68KDEB_68030_PAGE_LONG)
        bytes = 8u;
    else
        return M68KDEB_68030_PHYSREAD_BAD_ARGUMENT;

    rc = reader(reader_ctx, last_descriptor_phys, raw, bytes);
    if (rc != 0)
        return M68KDEB_68030_PHYSREAD_READER_FAILED;

    word0 = be32(raw);
    if (bytes == 8u)
        word1 = be32(raw + 4);

    rc = m68kdeb_68030_resolve_terminal_evidence(logical,
                                                  psr,
                                                  last_descriptor_phys,
                                                  page_shift,
                                                  format,
                                                  word0,
                                                  word1,
                                                  out);
    if (rc != M68KDEB_68030_EVIDENCE_OK)
        return M68KDEB_68030_PHYSREAD_BAD_EVIDENCE;

    return M68KDEB_68030_PHYSREAD_OK;
}
