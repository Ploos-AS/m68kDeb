#include <exec/types.h>
#include <proto/exec.h>
#include <stdio.h>
#include <stdint.h>

#include "m68kdeb-translate-68030.h"
#include "m68kdeb-68030-physread-tt.h"
#include "m68kdeb-68030-physread-tt-bounded.h"

static volatile uint32_t probe_word = 0x4d36384bu;

static void dump_bytes(const unsigned char *p, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; ++i)
        printf("%02lx", (unsigned long)p[i]);
    putchar('\n');
}

int main(void)
{
    APTR oldsp;
    unsigned short target_psr = 0;
    unsigned short desc_psr = 0;
    unsigned long target_desc_phys = 0;
    unsigned long desc_desc_phys = 0;
    struct m68kdeb_68030_tt_read_proof proof;
    unsigned char raw[4];
    int rc;

    /* Runtime evidence must survive a trap/hang at a privileged boundary. */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("M68KDEB_5N_START\n");
    printf("target_logical=0x%08lx\n", (unsigned long)(uintptr_t)&probe_word);
    printf("M68KDEB_5N_BEFORE_SUPERSTATE_TARGET\n");

    oldsp = SuperState();
    if (!oldsp) {
        printf("M68KDEB_5N_SUPERSTATE_FAIL\n");
        return 10;
    }

    /* No stdio or Exec/library calls are allowed while supervisor state is active. */
    rc = m68kdeb_translate_68030_ptest((unsigned long)(uintptr_t)&probe_word,
                                       &target_psr,
                                       &target_desc_phys);
    UserState(oldsp);
    printf("M68KDEB_5N_AFTER_TARGET_PTEST rc=%ld\n", (long)rc);
    printf("target_psr=0x%04lx\n", (unsigned long)target_psr);
    printf("target_last_descriptor_phys=0x%08lx\n", target_desc_phys);

    if (rc != 0) {
        printf("M68KDEB_5N_TARGET_PTEST_FAIL rc=%ld\n", (long)rc);
        return 11;
    }
    if (target_desc_phys == 0) {
        /* Zero is meaningful evidence: for example, a transparent/no-table
         * result has no terminal table descriptor to bind. Fail closed, but
         * preserve PSR so CI can classify the actual runtime translation path. */
        printf("M68KDEB_5N_NO_TARGET_DESCRIPTOR psr=0x%04lx\n",
               (unsigned long)target_psr);
        return 14;
    }

    /*
     * The target PTEST status does not prove that target_desc_phys itself is
     * logically readable. Re-probe that physical value as a logical address
     * and require a transparent-translation identity result before reading.
     */
    printf("M68KDEB_5N_BEFORE_SUPERSTATE_DESCRIPTOR\n");
    oldsp = SuperState();
    if (!oldsp) {
        printf("M68KDEB_5N_SUPERSTATE_FAIL_SECOND\n");
        return 12;
    }

    /* Again, PTEST is the only operation performed in supervisor state. */
    rc = m68kdeb_translate_68030_ptest(target_desc_phys,
                                       &desc_psr,
                                       &desc_desc_phys);
    UserState(oldsp);
    printf("M68KDEB_5N_AFTER_DESCRIPTOR_PTEST rc=%ld\n", (long)rc);

    if (rc != 0) {
        printf("M68KDEB_5N_DESCRIPTOR_PTEST_FAIL rc=%ld\n", (long)rc);
        return 13;
    }

    printf("descriptor_probe_psr=0x%04lx\n", (unsigned long)desc_psr);
    printf("descriptor_probe_last_descriptor_phys=0x%08lx\n", desc_desc_phys);

    rc = m68kdeb_68030_tt_read_admit((uint32_t)target_desc_phys,
                                     (uint32_t)target_desc_phys,
                                     4u,
                                     (uint16_t)desc_psr,
                                     &proof);
    if (rc != M68KDEB_68030_TTREAD_OK) {
        printf("M68KDEB_5N_NOT_ADMITTED rc=%ld\n", (long)rc);
        return 20;
    }

    printf("M68KDEB_5N_TT_ADMITTED\n");
    printf("M68KDEB_5N_BEFORE_BOUNDED_READ\n");

    rc = m68kdeb_68030_tt_bounded_read(&proof,
                                       (uint32_t)target_desc_phys,
                                       raw,
                                       4u);
    if (rc != M68KDEB_68030_TTBOUNDED_OK) {
        printf("M68KDEB_5N_BOUNDED_READ_FAIL rc=%ld\n", (long)rc);
        return 21;
    }

    printf("descriptor_bytes=");
    dump_bytes(raw, 4u);
    printf("M68KDEB_5N_BOUNDED_READ_OK\n");
    printf("M68KDEB_5N_RETURNED\n");
    return 0;
}
