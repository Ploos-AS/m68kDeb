/*
 * m68kDeb AROS/AmigaOS-compatible Linux/m68k loader
 *
 * M1.3b.1: native executable + Exec memory-map probe.
 * M1.3b.2: load an uncompressed Linux/m68k kernel, validate its advertised
 * Amiga bootinfo ABI, and construct a bootinfo record list in memory.
 *
 * Deliberately NO privileged takeover or kernel jump yet.
 */

#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <proto/dos.h>
#include <proto/exec.h>

static const char version[] = "$VER: m68kdeb-loader 0.2 (09.09.2026)";

#define BOOTINFOV_MAGIC       0x4249561aUL
#define AMIGA_BOOTI_VERSION   0x00020000UL
#define MACH_AMIGA            1UL

#define BI_LAST               0x0000
#define BI_MACHTYPE           0x0001
#define BI_CPUTYPE            0x0002
#define BI_FPUTYPE            0x0003
#define BI_MMUTYPE            0x0004
#define BI_MEMCHUNK           0x0005
#define BI_COMMAND_LINE       0x0007
#define BI_AMIGA_MODEL        0x8000
#define BI_AMIGA_CHIP_SIZE    0x8002

#define CPU_68020             (1UL << 0)
#define CPU_68030             (1UL << 1)
#define CPU_68040             (1UL << 2)
#define CPU_68060             (1UL << 3)
#define FPU_68881             (1UL << 0)
#define FPU_68882             (1UL << 1)
#define FPU_68040             (1UL << 2)
#define FPU_68060             (1UL << 3)
#define MMU_68851             (1UL << 0)
#define MMU_68030             (1UL << 1)
#define MMU_68040             (1UL << 2)
#define MMU_68060             (1UL << 3)

#define BOOTINFO_CAPACITY     4096UL
#define DEFAULT_AMIGA_MODEL   5UL /* AMI_1200 for the M1.3b CI profile */

#define EI_CLASS              4
#define EI_DATA               5
#define EI_VERSION            6
#define ELFCLASS32            1
#define ELFDATA2MSB           2
#define EV_CURRENT            1
#define EM_68K                4
#define PT_LOAD               1UL
#define ELF32_EHDR_SIZE       52UL
#define ELF32_PHDR_SIZE       32UL

struct bootinfo_builder {
    UBYTE *base;
    ULONG used;
    ULONG capacity;
    ULONG records;
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
    store_be16(b->base + b->used + 2, (UWORD)size);
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

static int add_memchunk(struct bootinfo_builder *b, ULONG addr, ULONG size)
{
    UBYTE p[8];
    store_be32(p, addr);
    store_be32(p + 4, size);
    return add_record(b, BI_MEMCHUNK, p, sizeof(p));
}

static int add_string(struct bootinfo_builder *b, UWORD tag, const char *s)
{
    ULONG n = 0;
    while (s[n] != '\0')
        n++;
    return add_record(b, tag, (const UBYTE *)s, n + 1UL);
}

static int validate_bootinfo(const UBYTE *b, ULONG used)
{
    ULONG off = 0;
    int saw_last = 0;

    while (off + 4UL <= used) {
        UWORD tag = load_be16(b + off);
        UWORD size = load_be16(b + off + 2);
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

/*
 * vmlinux is an ELF32 big-endian m68k executable.  Linux links ENTRY(_start),
 * while the bootinfo version table starts earlier at _stext in the same
 * loadable text segment.  Resolve the entry through PT_LOAD for a sanity
 * check, then search only the file-backed bytes preceding _start in that
 * segment for BOOTINFOV_MAGIC and validate the machine/version pairs.
 */
static int kernel_entry_file_offset(const UBYTE *kernel, ULONG size,
                                    ULONG *entry, ULONG *entry_offset)
{
    ULONG phoff;
    UWORD phentsize;
    UWORD phnum;
    UWORD i;

    if (size < ELF32_EHDR_SIZE)
        return 0;
    if (kernel[0] != 0x7f || kernel[1] != 'E' ||
        kernel[2] != 'L' || kernel[3] != 'F')
        return 0;
    if (kernel[EI_CLASS] != ELFCLASS32 ||
        kernel[EI_DATA] != ELFDATA2MSB ||
        kernel[EI_VERSION] != EV_CURRENT)
        return 0;
    if (load_be16(kernel + 18) != EM_68K)
        return 0;

    *entry = load_be32(kernel + 24);
    phoff = load_be32(kernel + 28);
    phentsize = load_be16(kernel + 42);
    phnum = load_be16(kernel + 44);

    if (phentsize < ELF32_PHDR_SIZE || phnum == 0)
        return 0;
    if (phoff > size || (ULONG)phnum > (size - phoff) / (ULONG)phentsize)
        return 0;

    for (i = 0; i < phnum; i++) {
        ULONG p = phoff + (ULONG)i * (ULONG)phentsize;
        ULONG file_off;
        ULONG vaddr;
        ULONG filesz;
        ULONG delta;

        if (load_be32(kernel + p) != PT_LOAD)
            continue;
        file_off = load_be32(kernel + p + 4);
        vaddr = load_be32(kernel + p + 8);
        filesz = load_be32(kernel + p + 16);
        if (*entry < vaddr)
            continue;
        delta = *entry - vaddr;
        if (delta >= filesz)
            continue;
        if (file_off > size || filesz > size - file_off)
            return 0;
        if (delta > filesz)
            return 0;
        *entry_offset = file_off + delta;
        if (*entry_offset >= size)
            return 0;
        return 1;
    }
    return 0;
}

static int kernel_supports_amiga_bootinfo(const UBYTE *kernel, ULONG size,
                                          ULONG *entry,
                                          ULONG *entry_offset,
                                          ULONG *magic_offset,
                                          ULONG *advertised_version)
{
    ULONG phoff;
    UWORD phentsize;
    UWORD phnum;
    UWORD i;

    if (!kernel_entry_file_offset(kernel, size, entry, entry_offset))
        return 0;

    phoff = load_be32(kernel + 28);
    phentsize = load_be16(kernel + 42);
    phnum = load_be16(kernel + 44);

    for (i = 0; i < phnum; i++) {
        ULONG p = phoff + (ULONG)i * (ULONG)phentsize;
        ULONG file_off;
        ULONG vaddr;
        ULONG filesz;
        ULONG delta;
        ULONG scan;
        ULONG scan_end;

        if (load_be32(kernel + p) != PT_LOAD)
            continue;

        file_off = load_be32(kernel + p + 4);
        vaddr = load_be32(kernel + p + 8);
        filesz = load_be32(kernel + p + 16);
        if (*entry < vaddr)
            continue;
        delta = *entry - vaddr;
        if (delta >= filesz)
            continue;
        if (file_off > size || filesz > size - file_off)
            return 0;

        scan_end = file_off + delta;
        if (scan_end > size)
            return 0;

        for (scan = file_off; scan + 12UL <= scan_end; scan += 2UL) {
            ULONG q;
            ULONG pairs;

            if (load_be32(kernel + scan) != BOOTINFOV_MAGIC)
                continue;

            q = scan + 4UL;
            pairs = 0;
            while (q <= size - 8UL && pairs < 32UL) {
                ULONG mach = load_be32(kernel + q);
                ULONG ver = load_be32(kernel + q + 4UL);
                if (mach == 0UL)
                    break;
                if (mach == MACH_AMIGA) {
                    *magic_offset = scan;
                    *advertised_version = ver;
                    return ver == AMIGA_BOOTI_VERSION;
                }
                q += 8UL;
                pairs++;
            }
        }
        return 0;
    }
    return 0;
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

static int probe_only(struct ExecBase *SysBase)
{
    struct MemHeader *mh;
    ULONG regions = 0;

    Printf("M68KDEB_LOADER_START\n");
    Printf("exec_version=%lu.%lu\n",
           (ULONG)SysBase->LibNode.lib_Version,
           (ULONG)SysBase->LibNode.lib_Revision);
    Printf("attn_flags=0x%04lx\n", (ULONG)SysBase->AttnFlags);

    for (mh = (struct MemHeader *)SysBase->MemList.lh_Head;
         mh->mh_Node.ln_Succ != NULL;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
        ULONG lower = (ULONG)mh->mh_Lower;
        ULONG upper = (ULONG)mh->mh_Upper;
        ULONG free_bytes = (ULONG)mh->mh_Free;
        ULONG attrs = (ULONG)mh->mh_Attributes;
        const char *name = mh->mh_Node.ln_Name ? mh->mh_Node.ln_Name : "(unnamed)";

        Printf("mem[%lu] lower=0x%08lx upper=0x%08lx bytes=%lu free=%lu attrs=0x%08lx name=%s\n",
               regions, lower, upper, upper - lower, free_bytes, attrs, name);
        regions++;
    }

    Printf("memory_regions=%lu\n", regions);
    Printf("M68KDEB_LOADER_PROBE_OK\n");
    return 0;
}

static int load_and_build(struct ExecBase *SysBase, const char *path, ULONG model)
{
    BPTR fh;
    LONG size_long;
    LONG got;
    UBYTE *kernel = NULL;
    UBYTE *bootinfo = NULL;
    struct bootinfo_builder bb;
    struct MemHeader *mh;
    ULONG kernel_entry = 0;
    ULONG kernel_entry_offset = 0;
    ULONG magic_offset = 0;
    ULONG advertised_version = 0;
    ULONG cpu, mmu, fpu;
    ULONG regions = 0;
    ULONG chip_size = 0;
    static const char cmdline[] = "root=/dev/ram video=pal console=ttyS0,9600n8";
    int rc = 20;

    fh = Open((STRPTR)path, MODE_OLDFILE);
    if (!fh) {
        Printf("M68KDEB_LOADER_ERROR kernel_open path=%s\n", path);
        return 20;
    }

    if (Seek(fh, 0, OFFSET_END) < 0 ||
        (size_long = Seek(fh, 0, OFFSET_CURRENT)) <= 0 ||
        Seek(fh, 0, OFFSET_BEGINNING) < 0) {
        Printf("M68KDEB_LOADER_ERROR kernel_seek\n");
        Close(fh);
        return 20;
    }

    kernel = (UBYTE *)AllocMem((ULONG)size_long, MEMF_PUBLIC);
    if (!kernel) {
        Printf("M68KDEB_LOADER_ERROR kernel_alloc bytes=%lu\n", (ULONG)size_long);
        Close(fh);
        return 20;
    }

    got = Read(fh, kernel, size_long);
    Close(fh);
    if (got != size_long) {
        Printf("M68KDEB_LOADER_ERROR kernel_read expected=%lu got=%lu\n",
               (ULONG)size_long, (ULONG)got);
        goto out;
    }

    Printf("M68KDEB_KERNEL_LOADED path=%s addr=0x%08lx bytes=%lu\n",
           path, (ULONG)kernel, (ULONG)size_long);

    if (!kernel_supports_amiga_bootinfo(kernel, (ULONG)size_long,
                                        &kernel_entry, &kernel_entry_offset,
                                        &magic_offset, &advertised_version)) {
        Printf("M68KDEB_LOADER_ERROR bootinfo_abi entry=0x%08lx entry_file_offset=%lu magic_offset=%lu advertised=0x%08lx expected=0x%08lx\n",
               kernel_entry, kernel_entry_offset, magic_offset,
               advertised_version, AMIGA_BOOTI_VERSION);
        goto out;
    }

    Printf("kernel_format=ELF32-BE-m68k\n");
    Printf("kernel_entry=0x%08lx\n", kernel_entry);
    Printf("kernel_entry_file_offset=%lu\n", kernel_entry_offset);
    Printf("kernel_bootinfo_magic_offset=%lu\n", magic_offset);
    Printf("kernel_amiga_bootinfo_version=0x%08lx\n", advertised_version);

    cpu = detect_cpu(SysBase->AttnFlags);
    mmu = detect_mmu(cpu);
    fpu = detect_fpu(SysBase->AttnFlags, cpu);
    if (!cpu || !mmu) {
        Printf("M68KDEB_LOADER_ERROR cpu_mmu_detect cpu=0x%08lx mmu=0x%08lx attn=0x%04lx\n",
               cpu, mmu, (ULONG)SysBase->AttnFlags);
        goto out;
    }

    bootinfo = (UBYTE *)AllocMem(BOOTINFO_CAPACITY, MEMF_PUBLIC | MEMF_CLEAR);
    if (!bootinfo) {
        Printf("M68KDEB_LOADER_ERROR bootinfo_alloc\n");
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
        !add_u32(&bb, BI_AMIGA_MODEL, model)) {
        Printf("M68KDEB_LOADER_ERROR bootinfo_core_records\n");
        goto out;
    }

    for (mh = (struct MemHeader *)SysBase->MemList.lh_Head;
         mh->mh_Node.ln_Succ != NULL;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
        ULONG lower = (ULONG)mh->mh_Lower;
        ULONG upper = (ULONG)mh->mh_Upper;
        ULONG bytes = upper - lower;
        if (bytes == 0)
            continue;
        if (!add_memchunk(&bb, lower, bytes)) {
            Printf("M68KDEB_LOADER_ERROR bootinfo_memchunk index=%lu\n", regions);
            goto out;
        }
#ifdef MEMF_CHIP
        if ((mh->mh_Attributes & MEMF_CHIP) != 0)
            chip_size += bytes;
#endif
        Printf("boot_mem[%lu] addr=0x%08lx bytes=%lu attrs=0x%08lx\n",
               regions, lower, bytes, (ULONG)mh->mh_Attributes);
        regions++;
    }

    if (!add_u32(&bb, BI_AMIGA_CHIP_SIZE, chip_size) ||
        !add_string(&bb, BI_COMMAND_LINE, cmdline) ||
        !add_record(&bb, BI_LAST, NULL, 0)) {
        Printf("M68KDEB_LOADER_ERROR bootinfo_tail_records\n");
        goto out;
    }

    if (!validate_bootinfo(bootinfo, bb.used)) {
        Printf("M68KDEB_LOADER_ERROR bootinfo_selfcheck\n");
        goto out;
    }

    Printf("bootinfo_addr=0x%08lx\n", (ULONG)bootinfo);
    Printf("bootinfo_bytes=%lu\n", bb.used);
    Printf("bootinfo_records=%lu\n", bb.records);
    Printf("bootinfo_memchunks=%lu\n", regions);
    Printf("bootinfo_machtype=%lu\n", MACH_AMIGA);
    Printf("bootinfo_cpu=0x%08lx\n", cpu);
    Printf("bootinfo_fpu=0x%08lx\n", fpu);
    Printf("bootinfo_mmu=0x%08lx\n", mmu);
    Printf("bootinfo_amiga_model=%lu\n", model);
    Printf("bootinfo_chip_size=%lu\n", chip_size);
    Printf("bootinfo_cmdline=%s\n", cmdline);
    Printf("handoff=NOT_ATTEMPTED\n");
    Printf("M68KDEB_BOOTINFO_BUILD_OK\n");
    rc = 0;

out:
    if (bootinfo)
        FreeMem(bootinfo, BOOTINFO_CAPACITY);
    if (kernel)
        FreeMem(kernel, (ULONG)size_long);
    return rc;
}

int main(int argc, char **argv)
{
    struct ExecBase *SysBase = *((struct ExecBase **)4UL);
    ULONG model = DEFAULT_AMIGA_MODEL;

    (void)version;

    if (!SysBase) {
        PutStr("M68KDEB_LOADER_ERROR sysbase=null\n");
        return 20;
    }

    if (argc < 2)
        return probe_only(SysBase);

    if (argc >= 3) {
        LONG parsed = 0;
        if (StrToLong((STRPTR)argv[2], &parsed) > 0 && parsed > 0)
            model = (ULONG)parsed;
    }

    Printf("M68KDEB_LOADER_START mode=bootinfo-build\n");
    Printf("exec_version=%lu.%lu\n",
           (ULONG)SysBase->LibNode.lib_Version,
           (ULONG)SysBase->LibNode.lib_Revision);
    Printf("attn_flags=0x%04lx\n", (ULONG)SysBase->AttnFlags);
    return load_and_build(SysBase, argv[1], model);
}
