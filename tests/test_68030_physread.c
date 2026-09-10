#include <stdio.h>
#include <string.h>
#include "../loader/aros/m68kdeb-68030-physread.h"

struct fake_reader_ctx {
    uint32_t expected_phys;
    uint32_t expected_bytes;
    unsigned char data[8];
    int fail;
    int calls;
};

static int fake_reader(void *opaque, uint32_t physical, void *dst, uint32_t bytes)
{
    struct fake_reader_ctx *ctx = (struct fake_reader_ctx *)opaque;
    ctx->calls++;
    if (ctx->fail || physical != ctx->expected_phys || bytes != ctx->expected_bytes)
        return 1;
    memcpy(dst, ctx->data, bytes);
    return 0;
}

static int expect(int cond, const char *name)
{
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    struct fake_reader_ctx ctx;
    struct m68kdeb_68030_translation_evidence out;
    int rc;
    int failed = 0;

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_phys = 0x00102000u;
    ctx.expected_bytes = 4u;
    ctx.data[0] = 0x00; ctx.data[1] = 0x40; ctx.data[2] = 0x00; ctx.data[3] = 0x01;

    rc = m68kdeb_68030_bind_terminal_descriptor(0x00000123u, 0x0001u,
                                                 ctx.expected_phys, 12u,
                                                 M68KDEB_68030_PAGE_SHORT,
                                                 fake_reader, &ctx, &out);
    failed |= expect(rc == M68KDEB_68030_PHYSREAD_OK, "short descriptor accepted");
    failed |= expect(out.physical == 0x00400123u, "physical address decoded");
    failed |= expect(ctx.calls == 1, "reader called once");

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_phys = 0x00200000u;
    ctx.expected_bytes = 8u;
    ctx.data[0] = 0x00; ctx.data[1] = 0x00; ctx.data[2] = 0x00; ctx.data[3] = 0x01;
    ctx.data[4] = 0x00; ctx.data[5] = 0x80; ctx.data[6] = 0x00; ctx.data[7] = 0x00;
    rc = m68kdeb_68030_bind_terminal_descriptor(0x00000abcu, 0x0001u,
                                                 ctx.expected_phys, 12u,
                                                 M68KDEB_68030_PAGE_LONG,
                                                 fake_reader, &ctx, &out);
    failed |= expect(rc == M68KDEB_68030_PHYSREAD_OK, "long descriptor accepted");
    failed |= expect(out.physical == 0x00800abcu, "long descriptor physical decoded");

    rc = m68kdeb_68030_bind_terminal_descriptor(0, 1, 0x1003u, 12u,
                                                 M68KDEB_68030_PAGE_SHORT,
                                                 fake_reader, &ctx, &out);
    failed |= expect(rc == M68KDEB_68030_PHYSREAD_UNALIGNED, "unaligned descriptor rejected");

    ctx.fail = 1;
    ctx.expected_phys = 0x00300000u;
    ctx.expected_bytes = 4u;
    rc = m68kdeb_68030_bind_terminal_descriptor(0, 1, ctx.expected_phys, 12u,
                                                 M68KDEB_68030_PAGE_SHORT,
                                                 fake_reader, &ctx, &out);
    failed |= expect(rc == M68KDEB_68030_PHYSREAD_READER_FAILED, "reader failure propagated");

    if (failed)
        return 1;

    puts("M68KDEB_68030_PHYSREAD_TEST_PASS");
    return 0;
}
