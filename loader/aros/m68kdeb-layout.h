#ifndef M68KDEB_LAYOUT_H
#define M68KDEB_LAYOUT_H

#include <exec/types.h>

#define M68KDEB_PAGE_SIZE 4096UL

struct m68kdeb_elf_layout {
    ULONG phoff;
    UWORD phentsize;
    UWORD phnum;
    ULONG entry;
    ULONG min_vaddr;
    ULONG max_vaddr;
    ULONG load_segments;
};

struct m68kdeb_final_layout {
    UBYTE *raw;
    ULONG raw_bytes;
    UBYTE *base;
    ULONG image_bytes;
    ULONG entry_addr;
    UBYTE *bootinfo;
    ULONG bootinfo_capacity;
};

int m68kdeb_layout_inspect_elf(const UBYTE *kernel, ULONG kernel_bytes,
                               struct m68kdeb_elf_layout *layout);

int m68kdeb_layout_materialize_elf(const UBYTE *kernel, ULONG kernel_bytes,
                                   const struct m68kdeb_elf_layout *layout,
                                   UBYTE *base, ULONG image_bytes);

int m68kdeb_layout_allocate(const struct m68kdeb_elf_layout *layout,
                            ULONG bootinfo_capacity,
                            struct m68kdeb_final_layout *final_layout);

void m68kdeb_layout_release(struct m68kdeb_final_layout *final_layout);

#endif
