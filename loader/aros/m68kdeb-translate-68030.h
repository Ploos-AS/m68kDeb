#ifndef M68KDEB_TRANSLATE_68030_H
#define M68KDEB_TRANSLATE_68030_H

int m68kdeb_translate_68030_ptest(unsigned long logical,
                                  unsigned long *mmusr_out,
                                  unsigned long *last_descriptor_phys_out);

#endif
