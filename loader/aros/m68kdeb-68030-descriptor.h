#ifndef M68KDEB_68030_DESCRIPTOR_H
#define M68KDEB_68030_DESCRIPTOR_H

#include <stdint.h>

#define M68KDEB_68030_DESC_OK              0
#define M68KDEB_68030_DESC_BAD_ARGUMENT    1
#define M68KDEB_68030_DESC_BAD_PAGE_SHIFT  2
#define M68KDEB_68030_DESC_NOT_PAGE        3

enum m68kdeb_68030_page_format {
    M68KDEB_68030_PAGE_SHORT = 1,
    M68KDEB_68030_PAGE_LONG = 2
};

struct m68kdeb_68030_page_result {
    uint32_t logical;
    uint32_t physical;
    uint32_t page_base;
    uint32_t page_offset;
    uint16_t status;
    uint8_t descriptor_type;
    uint8_t format;
};

int m68kdeb_68030_decode_terminal_page(uint32_t logical,
                                       unsigned page_shift,
                                       enum m68kdeb_68030_page_format format,
                                       uint32_t word0,
                                       uint32_t word1,
                                       struct m68kdeb_68030_page_result *out);

#endif
