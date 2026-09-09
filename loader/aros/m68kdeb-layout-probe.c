/*
 * m68kDeb M1.3b.4a final Linux/m68k image-layout probe.
 *
 * This is deliberately reversible: it materializes the ELF PT_LOAD image,
 * places bootinfo immediately after it, validates the result, then frees all
 * allocations and returns to AROS. It never disables interrupts/caches/MMU
 * and never jumps to the kernel.
 */

#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <proto/dos.h>
#include <proto/exec.h>

static const char version[] = "$VER: m68kdeb-layout-probe 0.1 (09.09.2026)";

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
#define PAGE_SIZE 4096UL
#define BOOTINFO_CAPACITY 4096UL
#define DEFAULT_AMIGA_MODEL 5UL

#define MACH_AMIGA 1UL
#define BI_LAST 0x0000
#define BI_MACHTYPE 0x0001
#define BI_CPUTYPE 0x0002
#define BI_FPUTYPE 0x0003
#define BI_MMUTYPE 0x0004
#define BI_MEMCHUNK 0x0005
#define BI_RAMDISK 0x0006
#define BI_COMMAND_LINE 0x0007
#define BI_AMIGA_MODEL 0x8000
#define BI_AMIGA_CHIP_SIZE 0x8002

#define CPU_68020 (1UL << 0)
#define CPU_68030 (1UL << 1)
#define CPU_68040 (1UL << 2)
#define CPU_68060 (1UL << 3)
#define FPU_68881 (1UL << 0)
#define FPU_68882 (1UL << 1)
#define FPU_68040 (1UL << 2)
#define FPU_68060 (1UL << 3)
#define MMU_68851 (1UL << 0)
#define MMU_68030 (1UL << 1)
#define MMU_68040 (1UL << 2)
#define MMU_68060 (1UL << 3)

struct bootinfo_builder {
    UBYTE *base;
    ULONG used;
    ULONG capacity;
    ULONG records;
};

struct elf_layout {
    ULONG phoff;
    UWORD phentsize;
    UWORD phnum;
    ULONG entry;
    ULONG min_vaddr;
    ULONG max_vaddr;
    ULONG load_segments;
};

static UWORD load_be16(const UBYTE *p)
{
    return (UWORD)(((UWORD)p[0] << 8) | (UWORD)p[1]);
}

static ULONG load_be32(const UBYTE *p)
{
    return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) |
           ((ULONG)p[2] << 8) | (ULONG)p[3];
}

static void store_be16(UBYTE *p, UWORD v)
{
    p[0] = (UBYTE)(v >> 8);
    p[1] = (UBYTE)v;
}

static void store_be32(UBYTE *p, ULONG v)
{
    p[0] = (UBYTE)(v >> 24);
    p[1] = (UBYTE)(v >> 16);
    p[2] = (UBYTE)(v >> 8);
    p[3] = (UBYTE)v;
}

static int add_record(struct bootinfo_builder *b, UWORD tag,
                      const UBYTE *payload, ULONG payload_size)
{
    ULONG raw = 4UL + payload_size;
    ULONG size = (raw + 3UL) & ~3UL;
    ULONG i;

    if (size > 0xffffUL || b->used + size > b->capacity)
        return 0;
    store_be16(b->base + b->used, tag);
    store_be16(b->base + b->used + 2UL, (UWORD)size);
    for (i = 0; i < payload_size; i++)
        b->base[b->used + 4UL + i] = payload[i];
    for (i = raw; i < size; i++)
        b->base[b->used + i] = 0;
    b->used += size;
    b->records++;
    return 1;
}

static int add_u32(struct bootinfo_builder *b, UWORD tag, ULONG value)
{
    UBYTE p[4];
    store_be32(p, value);
    return add_record(b, tag, p, sizeof(p));
}

static int add_addr_size(struct bootinfo_builder *b, UWORD tag,
                         ULONG addr, ULONG size)
{
    UBYTE p[8];
    store_be32(p, addr);
    store_be32(p + 4, size);
    return add_record(b, tag, p, sizeof(p));
}

static int add_string(struct bootinfo_builder *b, UWORD tag, const char *s)
{
    ULONG n = 0;
    while (s[n] != '\0') n++;
    return add_record(b, tag, (const UBYTE *)s, n + 1UL);
}

static int validate_bootinfo(const UBYTE *b, ULONG used)
{
    ULONG off = 0;
    int saw_last = 0;
    while (off + 4UL <= used) {
        UWORD tag = load_be16(b + off);
        UWORD size = load_be16(b + off + 2UL);
        if (size < 4 || (size & 3) != 0 || off + (ULONG)size > used)
            return 0;
        if (tag == BI_LAST) {
            if (size != 4 || off + 4UL != used)
                return 0;
            saw_last = 1;
        }
        off += (ULONG)size;
    }
    return saw_last && off == used;
}

static UBYTE *load_public_file(const char *path, ULONG *size_out,
                               const char *what)
{
    BPTR fh;
    LONG size_long;
    LONG got;
    UBYTE *data;

    fh = Open((STRPTR)path, MODE_OLDFILE);
    if (!fh) {
        Printf("M68KDEB_LAYOUT_ERROR %s_open path=%s\n", what, path);
        return NULL;
    }
    if (Seek(fh, 0, OFFSET_END) < 0 ||
        (size_long = Seek(fh, 0, OFFSET_CURRENT)) <= 0 ||
        Seek(fh, 0, OFFSET_BEGINNING) < 0) {
        Printf("M68KDEB_LAYOUT_ERROR %s_seek\n", what);
        Close(fh);
        return NULL;
    }
    data = (UBYTE *)AllocMem((ULONG)size_long, MEMF_PUBLIC);
    if (!data) {
        Printf("M68KDEB_LAYOUT_ERROR %s_alloc bytes=%lu\n",
               what, (ULONG)size_long);
        Close(fh);
        return NULL;
    }
    got = Read(fh, data, size_long);
    Close(fh);
    if (got != size_long) {
        Printf("M68KDEB_LAYOUT_ERROR %s_read expected=%lu got=%lu\n",
               what, (ULONG)size_long, (ULONG)got);
        FreeMem(data, (ULONG)size_long);
        return NULL;
    }
    *size_out = (ULONG)size_long;
    return data;
}

static int inspect_elf(const UBYTE *kernel, ULONG size, struct elf_layout *l)
{
    UWORD i;
    int entry_seen = 0;

    if (size < ELF32_EHDR_SIZE || kernel[0] != 0x7f || kernel[1] != 'E' ||
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
        if (vaddr < l->min_vaddr) l->min_vaddr = vaddr;
        if (end > l->max_vaddr) l->max_vaddr = end;
        if (l->entry >= vaddr && l->entry < end)
            entry_seen = 1;
        l->load_segments++;
    }

    if (!l->load_segments || !entry_seen || l->max_vaddr <= l->min_vaddr)
        return 0;
    return 1;
}

static int materialize_elf(const UBYTE *kernel, ULONG kernel_size,
                           const struct elf_layout *l, UBYTE *base,
                           ULONG span)
{
    UWORD i;
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
        Printf("layout_segment[%lu] vaddr=0x%08lx file_bytes=%lu mem_bytes=%lu dest=0x%08lx\n",
               (ULONG)i, vaddr, filesz, memsz, (ULONG)(base + dest_off));
    }
    return 1;
}

static ULONG detect_cpu(UWORD attn)
{
#ifdef AFF_68060
    if (attn & AFF_68060) return CPU_68060;
#endif
#ifdef AFF_68040
    if (attn & AFF_68040) return CPU_68040;
#endif
#ifdef AFF_68030
    if (attn & AFF_68030) return CPU_68030;
#endif
#ifdef AFF_68020
    if (attn & AFF_68020) return CPU_68020;
#endif
    return 0;
}

static ULONG detect_mmu(ULONG cpu)
{
    if (cpu == CPU_68060) return MMU_68060;
    if (cpu == CPU_68040) return MMU_68040;
    if (cpu == CPU_68030) return MMU_68030;
    if (cpu == CPU_68020) return MMU_68851;
    return 0;
}

static ULONG detect_fpu(UWORD attn, ULONG cpu)
{
    if (cpu == CPU_68060) return FPU_68060;
    if (cpu == CPU_68040) return FPU_68040;
#ifdef AFF_68882
    if (attn & AFF_68882) return FPU_68882;
#endif
#ifdef AFF_68881
    if (attn & AFF_68881) return FPU_68881;
#endif
    return 0;
}

int main(int argc, char **argv)
{
    struct ExecBase *SysBase = *((struct ExecBase **)4UL);
    UBYTE *kernel = NULL;
    UBYTE *initramfs = NULL;
    UBYTE *raw = NULL;
    UBYTE *base;
    UBYTE *bootinfo;
    ULONG kernel_size = 0, initramfs_size = 0;
    ULONG alloc_size, span, model = DEFAULT_AMIGA_MODEL;
    ULONG cpu, mmu, fpu, chip_size = 0, regions = 0;
    ULONG entry_addr;
    LONG parsed = 0;
    struct elf_layout layout;
    struct bootinfo_builder bb;
    struct MemHeader *mh;
    static const char cmdline[] = "root=/dev/ram video=pal console=ttyS0,9600n8";
    int rc = 20;

    (void)version;
    if (!SysBase || argc < 3) {
        PutStr("usage: m68kdeb-layout-probe kernel initramfs [amiga-model]\n");
        return 20;
    }
    if (argc >= 4 && StrToLong((STRPTR)argv[3], &parsed) > 0 && parsed > 0)
        model = (ULONG)parsed;

    Printf("M68KDEB_LAYOUT_START\n");
    kernel = load_public_file(argv[1], &kernel_size, "kernel");
    if (!kernel) goto out;
    initramfs = load_public_file(argv[2], &initramfs_size, "initramfs");
    if (!initramfs) goto out;
    if (!inspect_elf(kernel, kernel_size, &layout)) {
        Printf("M68KDEB_LAYOUT_ERROR elf_contract\n");
        goto out;
    }

    span = layout.max_vaddr - layout.min_vaddr;
    if ((span & 3UL) != 0 || span > 0xffffffffUL - BOOTINFO_CAPACITY - (PAGE_SIZE - 1UL)) {
        Printf("M68KDEB_LAYOUT_ERROR span bytes=%lu\n", span);
        goto out;
    }
    alloc_size = span + BOOTINFO_CAPACITY + (PAGE_SIZE - 1UL);
    raw = (UBYTE *)AllocMem(alloc_size, MEMF_PUBLIC | MEMF_CLEAR);
    if (!raw) {
        Printf("M68KDEB_LAYOUT_ERROR image_alloc bytes=%lu\n", alloc_size);
        goto out;
    }
    base = (UBYTE *)(((ULONG)raw + PAGE_SIZE - 1UL) & ~(PAGE_SIZE - 1UL));
    bootinfo = base + span;
    entry_addr = (ULONG)base + (layout.entry - layout.min_vaddr);

    if (!materialize_elf(kernel, kernel_size, &layout, base, span)) {
        Printf("M68KDEB_LAYOUT_ERROR materialize\n");
        goto out;
    }

    cpu = detect_cpu(SysBase->AttnFlags);
    mmu = detect_mmu(cpu);
    fpu = detect_fpu(SysBase->AttnFlags, cpu);
    if (!cpu || !mmu) {
        Printf("M68KDEB_LAYOUT_ERROR cpu_mmu\n");
        goto out;
    }

    bb.base = bootinfo;
    bb.used = 0;
    bb.capacity = BOOTINFO_CAPACITY;
    bb.records = 0;
    if (!add_u32(&bb, BI_MACHTYPE, MACH_AMIGA) ||
        !add_u32(&bb, BI_CPUTYPE, cpu) ||
        !add_u32(&bb, BI_FPUTYPE, fpu) ||
        !add_u32(&bb, BI_MMUTYPE, mmu) ||
        !add_u32(&bb, BI_AMIGA_MODEL, model))
        goto bootinfo_error;

    for (mh = (struct MemHeader *)SysBase->MemList.lh_Head;
         mh->mh_Node.ln_Succ != NULL;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
        ULONG lower = (ULONG)mh->mh_Lower;
        ULONG upper = (ULONG)mh->mh_Upper;
        ULONG bytes = upper - lower;
        if (!bytes) continue;
        if (!add_addr_size(&bb, BI_MEMCHUNK, lower, bytes))
            goto bootinfo_error;
#ifdef MEMF_CHIP
        if ((mh->mh_Attributes & MEMF_CHIP) != 0)
            chip_size += bytes;
#endif
        regions++;
    }
    if (!add_u32(&bb, BI_AMIGA_CHIP_SIZE, chip_size) ||
        !add_addr_size(&bb, BI_RAMDISK, (ULONG)initramfs, initramfs_size) ||
        !add_string(&bb, BI_COMMAND_LINE, cmdline) ||
        !add_record(&bb, BI_LAST, NULL, 0))
        goto bootinfo_error;
    if (!validate_bootinfo(bootinfo, bb.used))
        goto bootinfo_error;

    Printf("kernel_format=ELF32-BE-m68k\n");
    Printf("kernel_entry=0x%08lx\n", layout.entry);
    Printf("layout_min_vaddr=0x%08lx\n", layout.min_vaddr);
    Printf("layout_max_vaddr=0x%08lx\n", layout.max_vaddr);
    Printf("layout_load_segments=%lu\n", layout.load_segments);
    Printf("layout_base=0x%08lx\n", (ULONG)base);
    Printf("layout_base_page_aligned=%lu\n", ((ULONG)base & (PAGE_SIZE - 1UL)) == 0 ? 1UL : 0UL);
    Printf("layout_image_bytes=%lu\n", span);
    Printf("layout_entry_addr=0x%08lx\n", entry_addr);
    Printf("layout_bootinfo_addr=0x%08lx\n", (ULONG)bootinfo);
    Printf("layout_bootinfo_bytes=%lu\n", bb.used);
    Printf("layout_bootinfo_records=%lu\n", bb.records);
    Printf("layout_bootinfo_memchunks=%lu\n", regions);
    Printf("layout_bootinfo_immediate=%lu\n", ((ULONG)bootinfo == (ULONG)base + span) ? 1UL : 0UL);
    Printf("layout_ramdisk_addr=0x%08lx\n", (ULONG)initramfs);
    Printf("layout_ramdisk_bytes=%lu\n", initramfs_size);
    Printf("handoff=NOT_ATTEMPTED\n");
    Printf("M68KDEB_FINAL_LAYOUT_OK\n");
    rc = 0;
    goto out;

bootinfo_error:
    Printf("M68KDEB_LAYOUT_ERROR bootinfo\n");
out:
    if (raw) FreeMem(raw, alloc_size);
    if (initramfs) FreeMem(initramfs, initramfs_size);
    if (kernel) FreeMem(kernel, kernel_size);
    return rc;
}
