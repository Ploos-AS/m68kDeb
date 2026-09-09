/*
 * m68kDeb AROS-native loader probe
 *
 * M1.3b.1 is deliberately read-only: prove that our own executable can run
 * under AROS and inspect the Exec memory map before implementing Linux bootinfo
 * construction or privileged handoff code.
 */

#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <proto/dos.h>

static const char version[] = "$VER: m68kdeb-loader 0.1 (09.09.2026)";

int main(void)
{
    struct ExecBase *SysBase = *((struct ExecBase **)4UL);
    struct MemHeader *mh;
    unsigned long regions = 0;

    (void)version;

    if (!SysBase) {
        PutStr("M68KDEB_LOADER_ERROR sysbase=null\n");
        return 20;
    }

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
