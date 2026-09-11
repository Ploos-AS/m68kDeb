/*
 * m68kDeb AROS/AmigaOS-compatible Linux/m68k loader
 *
 * M1.3b.4b.6c: construct Linux bootinfo with the RAM region containing the
 * final kernel layout first. Linux/m68k early MMU setup derives its initial
 * mapping from the first BI_MEMCHUNK, so AROS Exec MemList order must not leak
 * into the boot ABI.
 */

#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "m68kdeb-68030-mmu-state.h"
#include "m68kdeb-68030-takeover-identity.h"
#include "m68kdeb-handoff.h"
#include "m68kdeb-layout.h"
#include "m68kdeb-takeover.h"

static const char version[] = "$VER: m68kdeb-loader 0.5 (10.09.2026)";

#define BOOTINFOV_MAGIC       0x4249561aUL
#define AMIGA_BOOTI_VERSION   0x00020000UL
#define MACH_AMIGA            1UL
#define BI_LAST               0x0000
#define BI_MACHTYPE           0x0001
#define BI_CPUTYPE            0x0002
#define BI_FPUTYPE            0x0003
#define BI_MMUTYPE            0x0004
#define BI_MEMCHUNK           0x0005
#define BI_RAMDISK            0x0006
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
#define DEFAULT_AMIGA_MODEL   5UL

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

static int kernel_supports_amiga_bootinfo(const UBYTE *kernel, ULONG size,
                                          const struct m68kdeb_elf_layout *l,
                                          ULONG *magic_offset,
                                          ULONG *advertised_version)
{
    UWORD i;
    for (i = 0; i < l->phnum; i++) {
        ULONG p = l->phoff + (ULONG)i * (ULONG)l->phentsize;
        ULONG off, vaddr, filesz, delta, scan, scan_end;
        if (load_be32(kernel + p) != 1UL) continue;
        off = load_be32(kernel + p + 4UL);
        vaddr = load_be32(kernel + p + 8UL);
        filesz = load_be32(kernel + p + 16UL);
        if (l->entry < vaddr) continue;
        delta = l->entry - vaddr;
        if (delta >= filesz || off > size || filesz > size - off) continue;
        scan_end = off + delta;
        for (scan = off; scan + 12UL <= scan_end; scan += 2UL) {
            ULONG q, pairs = 0;
            if (load_be32(kernel + scan) != BOOTINFOV_MAGIC) continue;
            q = scan + 4UL;
            while (q <= size - 8UL && pairs < 32UL) {
                ULONG mach = load_be32(kernel + q);
                ULONG ver = load_be32(kernel + q + 4UL);
                if (mach == 0UL) break;
                if (mach == MACH_AMIGA) {
                    *magic_offset = scan;
                    *advertised_version = ver;
                    return ver == AMIGA_BOOTI_VERSION;
                }
                q += 8UL;
                pairs++;
            }
        }
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

static struct MemHeader *find_owner(struct ExecBase *SysBase,
                                    ULONG base, ULONG bytes)
{
    struct MemHeader *mh;
    ULONG end;
    if (!bytes || base > 0xffffffffUL - bytes) return NULL;
    end = base + bytes;
    for (mh = (struct MemHeader *)SysBase->MemList.lh_Head;
         mh->mh_Node.ln_Succ != NULL;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
        ULONG lower = (ULONG)mh->mh_Lower;
        ULONG upper = (ULONG)mh->mh_Upper;
        if (base >= lower && end <= upper) return mh;
    }
    return NULL;
}

static int probe_only(struct ExecBase *SysBase)
{
    struct MemHeader *mh;
    ULONG regions = 0;
    Printf("M68KDEB_LOADER_START\n");
    Printf("exec_version=%lu.%lu\n", (ULONG)SysBase->LibNode.lib_Version,
           (ULONG)SysBase->LibNode.lib_Revision);
    Printf("attn_flags=0x%04lx\n", (ULONG)SysBase->AttnFlags);
    for (mh = (struct MemHeader *)SysBase->MemList.lh_Head;
         mh->mh_Node.ln_Succ != NULL;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
        Printf("mem[%lu] lower=0x%08lx upper=0x%08lx bytes=%lu free=%lu attrs=0x%08lx name=%s\n",
               regions, (ULONG)mh->mh_Lower, (ULONG)mh->mh_Upper,
               (ULONG)mh->mh_Upper - (ULONG)mh->mh_Lower,
               (ULONG)mh->mh_Free, (ULONG)mh->mh_Attributes,
               mh->mh_Node.ln_Name ? mh->mh_Node.ln_Name : "(unnamed)");
        regions++;
    }
    Printf("memory_regions=%lu\n", regions);
    Printf("M68KDEB_LOADER_PROBE_OK\n");
    return 0;
}

static UBYTE *load_public_file(const char *path, ULONG *size_out,
                               const char *what)
{
    BPTR fh;
    LONG size_long, got;
    UBYTE *data;
    fh = Open((STRPTR)path, MODE_OLDFILE);
    if (!fh) {
        Printf("M68KDEB_LOADER_ERROR %s_open path=%s\n", what, path);
        return NULL;
    }
    if (Seek(fh, 0, OFFSET_END) < 0 ||
        (size_long = Seek(fh, 0, OFFSET_CURRENT)) <= 0 ||
        Seek(fh, 0, OFFSET_BEGINNING) < 0) {
        Printf("M68KDEB_LOADER_ERROR %s_seek\n", what);
        Close(fh);
        return NULL;
    }
    data = (UBYTE *)AllocMem((ULONG)size_long, MEMF_PUBLIC);
    if (!data) { Close(fh); return NULL; }
    got = Read(fh, data, size_long);
    Close(fh);
    if (got != size_long) { FreeMem(data, (ULONG)size_long); return NULL; }
    *size_out = (ULONG)size_long;
    return data;
}

static int load_and_build(struct ExecBase *SysBase, const char *path,
                          const char *initramfs_path, ULONG model)
{
    UBYTE *kernel = NULL, *initramfs = NULL;
    ULONG kernel_size = 0, initramfs_size = 0;
    ULONG cpu, mmu, fpu, chip_size = 0, regions = 0;
    ULONG magic_offset = 0, advertised_version = 0;
    struct m68kdeb_elf_layout elf;
    struct m68kdeb_final_layout final_layout;
    struct m68kdeb_takeover_request takeover;
    struct m68kdeb_68030_takeover_identity_proof identity;
    struct bootinfo_builder bb;
    struct MemHeader *mh, *layout_owner, *init_owner, *boot_first;
    static const char cmdline[] = "root=/dev/ram video=pal console=ttyS0,9600n8";
    unsigned long tc = 0;
    APTR oldsp;
    int preflight_rc = M68KDEB_TAKEOVER_BAD_INITRAMFS;
    int identity_rc, tc_rc;
    int rc = 20;

    final_layout.raw = NULL;
    kernel = load_public_file(path, &kernel_size, "kernel");
    if (!kernel) return 20;
    Printf("M68KDEB_KERNEL_LOADED path=%s addr=0x%08lx bytes=%lu\n",
           path, (ULONG)kernel, kernel_size);

    if (!m68kdeb_layout_inspect_elf(kernel, kernel_size, &elf)) {
        Printf("M68KDEB_LOADER_ERROR elf_contract\n"); goto out;
    }
    if (!kernel_supports_amiga_bootinfo(kernel, kernel_size, &elf,
                                        &magic_offset, &advertised_version)) {
        Printf("M68KDEB_LOADER_ERROR bootinfo_abi entry=0x%08lx advertised=0x%08lx expected=0x%08lx\n",
               elf.entry, advertised_version, AMIGA_BOOTI_VERSION); goto out;
    }
    if (!m68kdeb_layout_allocate(&elf, BOOTINFO_CAPACITY, &final_layout) ||
        !m68kdeb_layout_materialize_elf(kernel, kernel_size, &elf,
                                        final_layout.base,
                                        final_layout.image_bytes)) {
        Printf("M68KDEB_LOADER_ERROR final_layout\n"); goto out;
    }

    if (initramfs_path) {
        initramfs = load_public_file(initramfs_path, &initramfs_size, "initramfs");
        if (!initramfs) goto out;
        Printf("M68KDEB_INITRAMFS_LOADED path=%s addr=0x%08lx bytes=%lu\n",
               initramfs_path, (ULONG)initramfs, initramfs_size);
    }

    cpu = detect_cpu(SysBase->AttnFlags);
    mmu = detect_mmu(cpu);
    fpu = detect_fpu(SysBase->AttnFlags, cpu);
    if (!cpu || !mmu) { Printf("M68KDEB_LOADER_ERROR cpu_mmu_detect cpu=0x%08lx mmu=0x%08lx\n", cpu, mmu); goto out; }

    bb.base = final_layout.bootinfo;
    bb.used = 0;
    bb.capacity = final_layout.bootinfo_capacity;
    bb.records = 0;
    if (!add_u32(&bb, BI_MACHTYPE, MACH_AMIGA) ||
        !add_u32(&bb, BI_CPUTYPE, cpu) || !add_u32(&bb, BI_FPUTYPE, fpu) ||
        !add_u32(&bb, BI_MMUTYPE, mmu) || !add_u32(&bb, BI_AMIGA_MODEL, model)) goto out;

    boot_first = find_owner(SysBase, (ULONG)final_layout.base,
                            final_layout.image_bytes + final_layout.bootinfo_capacity);
    if (!boot_first) {
        Printf("M68KDEB_LOADER_ERROR boot_memchunk_owner\n");
        goto out;
    }

    /* Linux/m68k head.S treats the first BI_MEMCHUNK as the boot memory bank. */
    {
        ULONG lower = (ULONG)boot_first->mh_Lower;
        ULONG upper = (ULONG)boot_first->mh_Upper;
        ULONG bytes = upper - lower;
        if (!bytes || !add_addr_size(&bb, BI_MEMCHUNK, lower, bytes)) goto out;
#ifdef MEMF_CHIP
        if ((boot_first->mh_Attributes & MEMF_CHIP) != 0) chip_size += bytes;
#endif
        Printf("bootinfo_memchunk[0] lower=0x%08lx upper=0x%08lx bytes=%lu kernel_owner=1\n",
               lower, upper, bytes);
        regions++;
    }

    for (mh = (struct MemHeader *)SysBase->MemList.lh_Head;
         mh->mh_Node.ln_Succ != NULL;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
        ULONG lower, upper, bytes;
        if (mh == boot_first) continue;
        lower = (ULONG)mh->mh_Lower;
        upper = (ULONG)mh->mh_Upper;
        bytes = upper - lower;
        if (!bytes) continue;
        if (!add_addr_size(&bb, BI_MEMCHUNK, lower, bytes)) goto out;
#ifdef MEMF_CHIP
        if ((mh->mh_Attributes & MEMF_CHIP) != 0) chip_size += bytes;
#endif
        Printf("bootinfo_memchunk[%lu] lower=0x%08lx upper=0x%08lx bytes=%lu kernel_owner=0\n",
               regions, lower, upper, bytes);
        regions++;
    }
    if (!add_u32(&bb, BI_AMIGA_CHIP_SIZE, chip_size)) goto out;
    if (initramfs && !add_addr_size(&bb, BI_RAMDISK, (ULONG)initramfs, initramfs_size)) goto out;
    if (!add_string(&bb, BI_COMMAND_LINE, cmdline) ||
        !add_record(&bb, BI_LAST, NULL, 0) ||
        !validate_bootinfo(final_layout.bootinfo, bb.used)) goto out;

    Printf("layout_base=0x%08lx\n", (ULONG)final_layout.base);
    Printf("layout_bytes=%lu\n", final_layout.image_bytes);
    Printf("kernel_entry=0x%08lx\n", final_layout.entry_addr);
    Printf("bootinfo_addr=0x%08lx\n", (ULONG)final_layout.bootinfo);
    Printf("bootinfo_bytes=%lu\n", bb.used);
    Printf("bootinfo_records=%lu\n", bb.records);
    Printf("bootinfo_memchunks=%lu\n", regions);
    Printf("kernel_bootinfo_magic_offset=%lu\n", magic_offset);
    Printf("kernel_amiga_bootinfo_version=0x%08lx\n", advertised_version);

    if (initramfs) {
        preflight_rc = m68kdeb_build_takeover_request(&final_layout, bb.used,
                           (ULONG)initramfs, initramfs_size, cpu, mmu, &takeover);
        Printf("takeover_layout_base=0x%08lx\n", takeover.layout_base);
        Printf("takeover_entry=0x%08lx\n", takeover.entry_addr);
        Printf("takeover_bootinfo=0x%08lx\n", takeover.bootinfo_addr);
        Printf("takeover_initramfs=0x%08lx\n", takeover.initramfs_addr);
        Printf("takeover_preflight_rc=%ld\n", (LONG)preflight_rc);
        if (preflight_rc != M68KDEB_TAKEOVER_OK) { Printf("M68KDEB_LOADER_ERROR takeover_preflight rc=%ld\n", (LONG)preflight_rc); goto out; }
        Printf("M68KDEB_TAKEOVER_PREFLIGHT_OK\n");

        if (cpu == M68KDEB_CPU_68030 && mmu == M68KDEB_MMU_68030) {
            if (takeover.layout_bytes > 0xffffffffUL - takeover.bootinfo_bytes) goto out;
            layout_owner = find_owner(SysBase, takeover.layout_base,
                                      takeover.layout_bytes + takeover.bootinfo_bytes);
            init_owner = find_owner(SysBase, takeover.initramfs_addr,
                                    takeover.initramfs_bytes);
            if (!layout_owner || !init_owner) goto out;
            oldsp = SuperState();
            tc_rc = m68kdeb_68030_read_tc(&tc);
            if (oldsp) UserState(oldsp);
            Printf("takeover_tc_rc=%ld\n", (LONG)tc_rc);
            Printf("takeover_tc=0x%08lx\n", tc);
            if (tc_rc != 0) goto out;
            identity_rc = m68kdeb_68030_takeover_identity_from_tc(
                (uint32_t)tc, &takeover,
                (uint32_t)(ULONG)layout_owner->mh_Lower,
                (uint32_t)((ULONG)layout_owner->mh_Upper - (ULONG)layout_owner->mh_Lower),
                (uint32_t)(ULONG)init_owner->mh_Lower,
                (uint32_t)((ULONG)init_owner->mh_Upper - (ULONG)init_owner->mh_Lower),
                &identity);
            Printf("takeover_identity_rc=%ld\n", (LONG)identity_rc);
            if (identity_rc != M68KDEB_68030_TAKEID_OK) goto out;
            Printf("M68KDEB_TAKEOVER_IDENTITY_OK\n");
        }
        Flush(Output());
        rc = m68kdeb_takeover_commit(&takeover);
        Printf("M68KDEB_LOADER_ERROR takeover_commit_returned rc=%ld\n", (LONG)rc);
        rc = 20;
        goto out;
    }
    rc = 0;
out:
    if (final_layout.raw) m68kdeb_layout_free(&final_layout);
    if (initramfs) FreeMem(initramfs, initramfs_size);
    if (kernel) FreeMem(kernel, kernel_size);
    return rc;
}

int main(int argc, char **argv)
{
    struct ExecBase *SysBase = *((struct ExecBase **)4UL);
    if (argc == 1) return probe_only(SysBase);
    if (argc == 2) return load_and_build(SysBase, argv[1], NULL, DEFAULT_AMIGA_MODEL);
    if (argc == 3) return load_and_build(SysBase, argv[1], argv[2], DEFAULT_AMIGA_MODEL);
    Printf("usage: %s [kernel [initramfs]]\n", argv[0]);
    return 20;
}
