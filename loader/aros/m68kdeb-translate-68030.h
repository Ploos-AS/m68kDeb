#ifndef M68KDEB_TRANSLATE_68030_H
#define M68KDEB_TRANSLATE_68030_H

int m68kdeb_translate_68030_ptest(unsigned long logical,
                                  unsigned short *psr_out,
                                  unsigned long *last_descriptor_phys_out);

#endif
