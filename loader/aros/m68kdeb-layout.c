#include <exec/memory.h>
#include <proto/exec.h>

#include "m68kdeb-layout.h"

#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define ELFCLASS32 1
#define ELFDATA2MSB 2
#define EV_CURRENT 1
#define EM_68K 4
#define PT_LOAD 1UL
#define ELF32_EHDR_SIZE 52UL
#define ELF32_PHDR_SIZE 32UL

static UWORD load_be16(const UBYTE *p)
{
    return (UWORD)(((UWORD)p[0] << 8) | (UWORD)p[1]);
}

static ULONG load_be32(const UBYTE *p)
{
    return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) |
           ((ULONG)p[2] << 8) | (ULONG)p[3];
}

int m68kdeb_layout_inspect_elf(const UBYTE *kernel, ULONG size,
                               struct m68kdeb_elf_layout *l)
{
    UWORD i;
    int entry_seen = 0;

    if (!kernel || !l || size < ELF32_EHDR_SIZE ||
        kernel[0] != 0x7f || kernel[1] != 'E' ||
        kernel[2] != 'L' || kernel[3] != 'F')
        return 0;
    if (kernel[EI_CLASS] != ELFCLASS32 || kernel[EI_DATA] != ELFDATA2MSB ||
        kernel[EI_VERSION] != EV_CURRENT || load_be16(kernel + 18) != EM_68K)
        return 0;

    l->entry = load_be32(kernel + 24);
    l->phoff = load_be32(kernel + 28);
    l->phentsize = load_be16(kernel + 42);
    l->phnum = load_be16(kernel + 44);
    l->min_vaddr = 0xffffffffUL;
    l->max_vaddr = 0;
    l->load_segments = 0;

    if (l->phentsize < ELF32_PHDR_SIZE || l->phnum == 0 ||
        l->phoff > size ||
        (ULONG)l->phnum > (size - l->phoff) / (ULONG)l->phentsize)
        return 0;

    for (i = 0; i < l->phnum; i++) {
        ULONG p = l->phoff + (ULONG)i * (ULONG)l->phentsize;
        ULONG off, vaddr, filesz, memsz, end;

        if (load_be32(kernel + p) != PT_LOAD)
            continue;
        off = load_be32(kernel + p + 4UL);
        vaddr = load_be32(kernel + p + 8UL);
        filesz = load_be32(kernel + p + 16UL);
        memsz = load_be32(kernel + p + 20UL);
        if (memsz < filesz || off > size || filesz > size - off)
            return 0;
        if (memsz > 0xffffffffUL - vaddr)
            return 0;
        end = vaddr + memsz;
        if (vaddr < l->min_vaddr)
            l->min_vaddr = vaddr;
        if (end > l->max_vaddr)
            l->max_vaddr = end;
        if (l->entry >= vaddr && l->entry < end)
            entry_seen = 1;
        l->load_segments++;
    }

    return l->load_segments && entry_seen && l->max_vaddr > l->min_vaddr;
}

int m68kdeb_layout_materialize_elf(const UBYTE *kernel, ULONG kernel_size,
                                   const struct m68kdeb_elf_layout *l,
                                   UBYTE *base, ULONG span)
{
    UWORD i;

    if (!kernel || !l || !base || !span)
        return 0;

    for (i = 0; i < l->phnum; i++) {
        ULONG p = l->phoff + (ULONG)i * (ULONG)l->phentsize;
        ULONG off, vaddr, filesz, memsz, dest_off;

        if (load_be32(kernel + p) != PT_LOAD)
            continue;
        off = load_be32(kernel + p + 4UL);
        vaddr = load_be32(kernel + p + 8UL);
        filesz = load_be32(kernel + p + 16UL);
        memsz = load_be32(kernel + p + 20UL);
        if (vaddr < l->min_vaddr)
            return 0;
        dest_off = vaddr - l->min_vaddr;
        if (dest_off > span || memsz > span - dest_off ||
            off > kernel_size || filesz > kernel_size - off)
            return 0;
        if (memsz)
            SetMem(base + dest_off, 0, memsz);
        if (filesz)
            CopyMem((APTR)(kernel + off), (APTR)(base + dest_off), filesz);
    }
    return 1;
}

int m68kdeb_layout_allocate(const struct m68kdeb_elf_layout *l,
                            ULONG bootinfo_capacity,
                            struct m68kdeb_final_layout *f)
{
    ULONG span, alloc_size;
    UBYTE *raw, *base;

    if (!l || !f || !bootinfo_capacity || l->max_vaddr <= l->min_vaddr)
        return 0;

    span = l->max_vaddr - l->min_vaddr;
    if ((span & 3UL) != 0 ||
        span > 0xffffffffUL - bootinfo_capacity - (M68KDEB_PAGE_SIZE - 1UL))
        return 0;

    alloc_size = span + bootinfo_capacity + (M68KDEB_PAGE_SIZE - 1UL);
    raw = (UBYTE *)AllocMem(alloc_size, MEMF_PUBLIC | MEMF_CLEAR);
    if (!raw)
        return 0;

    base = (UBYTE *)(((ULONG)raw + M68KDEB_PAGE_SIZE - 1UL) &
                     ~(M68KDEB_PAGE_SIZE - 1UL));

    f->raw = raw;
    f->raw_bytes = alloc_size;
    f->base = base;
    f->image_bytes = span;
    f->entry_addr = (ULONG)base + (l->entry - l->min_vaddr);
    f->bootinfo = base + span;
    f->bootinfo_capacity = bootinfo_capacity;
    return 1;
}

void m68kdeb_layout_release(struct m68kdeb_final_layout *f)
{
    if (!f)
        return;
    if (f->raw)
        FreeMem(f->raw, f->raw_bytes);
    f->raw = NULL;
    f->raw_bytes = 0;
    f->base = NULL;
    f->image_bytes = 0;
    f->entry_addr = 0;
    f->bootinfo = NULL;
    f->bootinfo_capacity = 0;
}
