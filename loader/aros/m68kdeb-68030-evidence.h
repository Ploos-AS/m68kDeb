#ifndef M68KDEB_68030_EVIDENCE_H
#define M68KDEB_68030_EVIDENCE_H

#include <stdint.h>
#include "m68kdeb-68030-descriptor.h"

#define M68KDEB_68030_EVIDENCE_OK                 0
#define M68KDEB_68030_EVIDENCE_BAD_ARGUMENT       1
#define M68KDEB_68030_EVIDENCE_TRANSLATION_FAULT  2
#define M68KDEB_68030_EVIDENCE_NO_LEVELS          3
#define M68KDEB_68030_EVIDENCE_BAD_DESCRIPTOR     4

#define M68KDEB_68030_PSR_BUS_ERROR        0x8000u
#define M68KDEB_68030_PSR_LIMIT            0x4000u
#define M68KDEB_68030_PSR_SUPERVISOR       0x2000u
#define M68KDEB_68030_PSR_WRITE_PROTECTED  0x0800u
#define M68KDEB_68030_PSR_INVALID          0x0400u
#define M68KDEB_68030_PSR_MODIFIED         0x0200u
#define M68KDEB_68030_PSR_TRANSPARENT      0x0040u
#define M68KDEB_68030_PSR_LEVELS_MASK      0x0007u

struct m68kdeb_68030_translation_evidence {
    uint32_t logical;
    uint32_t physical;
    uint32_t last_descriptor_phys;
    uint16_t psr;
    uint8_t levels;
    uint8_t write_protected;
    uint8_t modified;
    uint8_t reserved;
    struct m68kdeb_68030_page_result page;
};

int m68kdeb_68030_resolve_terminal_evidence(
    uint32_t logical,
    uint16_t psr,
    uint32_t last_descriptor_phys,
    unsigned page_shift,
    enum m68kdeb_68030_page_format format,
    uint32_t descriptor_word0,
    uint32_t descriptor_word1,
    struct m68kdeb_68030_translation_evidence *out);

#endif
