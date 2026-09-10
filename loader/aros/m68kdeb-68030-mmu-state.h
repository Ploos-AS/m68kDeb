#ifndef M68KDEB_68030_MMU_STATE_H
#define M68KDEB_68030_MMU_STATE_H

#define M68KDEB_68030_TC_ENABLE 0x80000000UL

int m68kdeb_68030_read_tc(unsigned long *tc_out);

#endif
