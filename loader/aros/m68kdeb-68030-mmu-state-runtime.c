#include <exec/types.h>
#include <proto/exec.h>
#include <stdio.h>

#include "m68kdeb-68030-mmu-state.h"

int main(void)
{
    APTR oldsp;
    unsigned long tc = 0;
    int rc;

    printf("M68KDEB_5O_START\n");
    printf("M68KDEB_5O_BEFORE_SUPERSTATE\n");

    oldsp = SuperState();
    if (!oldsp) {
        printf("M68KDEB_5O_SUPERSTATE_FAIL\n");
        return 10;
    }

    rc = m68kdeb_68030_read_tc(&tc);
    UserState(oldsp);

    printf("M68KDEB_5O_AFTER_TC_READ rc=%ld\n", (long)rc);
    if (rc != 0) {
        printf("M68KDEB_5O_TC_READ_FAIL\n");
        return 11;
    }

    printf("tc=0x%08lx\n", tc);
    printf("tc_enable=%lu\n", (tc & M68KDEB_68030_TC_ENABLE) ? 1UL : 0UL);

    if (tc & M68KDEB_68030_TC_ENABLE)
        printf("M68KDEB_5O_TRANSLATION_ENABLED\n");
    else
        printf("M68KDEB_5O_TRANSLATION_DISABLED\n");

    printf("M68KDEB_5O_RETURNED\n");
    return 0;
}
