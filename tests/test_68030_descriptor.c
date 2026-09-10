#include <stdio.h>
#include <string.h>

#include "../loader/aros/m68kdeb-68030-descriptor.h"

static int expect(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    struct m68kdeb_68030_page_result r;
    int failed = 0;
    int rc;

    memset(&r, 0, sizeof(r));
    rc = m68kdeb_68030_decode_terminal_page(0x00123456u, 12,
                                            M68KDEB_68030_PAGE_SHORT,
                                            0x0089a001u, 0, &r);
    failed |= expect(rc == M68KDEB_68030_DESC_OK, "short page accepted");
    failed |= expect(r.page_base == 0x0089a000u, "short page base");
    failed |= expect(r.page_offset == 0x456u, "short page offset");
    failed |= expect(r.physical == 0x0089a456u, "short physical");
    failed |= expect(r.descriptor_type == 1u, "short DT");

    memset(&r, 0, sizeof(r));
    rc = m68kdeb_68030_decode_terminal_page(0x00abc123u, 12,
                                            M68KDEB_68030_PAGE_LONG,
                                            0x00000001u, 0x01234000u, &r);
    failed |= expect(rc == M68KDEB_68030_DESC_OK, "long page accepted");
    failed |= expect(r.page_base == 0x01234000u, "long page base");
    failed |= expect(r.page_offset == 0x123u, "long page offset");
    failed |= expect(r.physical == 0x01234123u, "long physical");

    rc = m68kdeb_68030_decode_terminal_page(0, 7,
                                            M68KDEB_68030_PAGE_SHORT,
                                            1, 0, &r);
    failed |= expect(rc == M68KDEB_68030_DESC_BAD_PAGE_SHIFT,
                     "reject page shift below status field");

    rc = m68kdeb_68030_decode_terminal_page(0, 12,
                                            M68KDEB_68030_PAGE_SHORT,
                                            0, 0, &r);
    failed |= expect(rc == M68KDEB_68030_DESC_NOT_PAGE,
                     "reject invalid descriptor");

    rc = m68kdeb_68030_decode_terminal_page(0, 12,
                                            M68KDEB_68030_PAGE_SHORT,
                                            2, 0, &r);
    failed |= expect(rc == M68KDEB_68030_DESC_NOT_PAGE,
                     "reject pointer descriptor");

    rc = m68kdeb_68030_decode_terminal_page(0, 12,
                                            (enum m68kdeb_68030_page_format)99,
                                            1, 0, &r);
    failed |= expect(rc == M68KDEB_68030_DESC_BAD_ARGUMENT,
                     "reject unknown format");

    if (failed)
        return 1;

    puts("M68KDEB_68030_DESCRIPTOR_TEST_PASS");
    return 0;
}
