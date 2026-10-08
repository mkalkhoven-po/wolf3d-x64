#include <stdio.h>
#include <string.h>

#include "id_ca.h"
#include "wl_data.h"

static int g_failures;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
            g_failures++;                                                    \
        }                                                                    \
    } while (0)

static void test_carmack_literals(void)
{
    const byte in[] = { 0x34, 0x12, 0x78, 0x56 };
    uint16_t out[2] = { 0, 0 };

    CHECK(CAL_CarmackExpand(in, sizeof(in), out, sizeof(out)));
    CHECK(out[0] == 0x1234 && out[1] == 0x5678);
}

static void test_carmack_near_copy(void)
{
    /* two literals, then NEARTAG count=2 offset=2 words back */
    const byte in[] = { 0x01, 0x00, 0x02, 0x00, 0x02, 0xA7, 0x02 };
    uint16_t out[4] = { 0 };

    CHECK(CAL_CarmackExpand(in, sizeof(in), out, sizeof(out)));
    CHECK(out[0] == 1 && out[1] == 2 && out[2] == 1 && out[3] == 2);
}

static void test_carmack_far_copy(void)
{
    /* two literals, then FARTAG count=2 absolute word offset 0 */
    const byte in[] = { 0x01, 0x00, 0x02, 0x00, 0x02, 0xA8, 0x00, 0x00 };
    uint16_t out[4] = { 0 };

    CHECK(CAL_CarmackExpand(in, sizeof(in), out, sizeof(out)));
    CHECK(out[0] == 1 && out[1] == 2 && out[2] == 1 && out[3] == 2);
}

static void test_carmack_escaped_literal(void)
{
    /* count 0 means the low byte of a tag-looking word follows in the stream */
    const byte in[] = { 0x00, 0xA7, 0x34, 0x00, 0xA8, 0x56 };
    uint16_t out[2] = { 0 };

    CHECK(CAL_CarmackExpand(in, sizeof(in), out, sizeof(out)));
    CHECK(out[0] == 0xA734 && out[1] == 0xA856);
}

static void test_carmack_rejects_corrupt(void)
{
    uint16_t out[4] = { 0 };
    const byte truncated[] = { 0x01, 0x00 };
    const byte bad_near_offset[] = { 0x02, 0xA7, 0x05 };
    const byte count_overflow[] = { 0x01, 0x00, 0x09, 0xA7, 0x01 };
    const byte far_forward[] = { 0x01, 0x00, 0x02, 0xA8, 0x03, 0x00 };

    CHECK(!CAL_CarmackExpand(truncated, sizeof(truncated), out, sizeof(out)));
    CHECK(!CAL_CarmackExpand(bad_near_offset, sizeof(bad_near_offset), out, sizeof(out)));
    CHECK(!CAL_CarmackExpand(count_overflow, sizeof(count_overflow), out, sizeof(out)));
    CHECK(!CAL_CarmackExpand(far_forward, sizeof(far_forward), out, sizeof(out)));
}

static void test_rlew(void)
{
    const uint16_t tag = 0xABCD;
    const uint16_t in[] = { 1, tag, 3, 7, 2 };
    uint16_t out[5] = { 0 };

    CHECK(CA_RLEWexpand(in, 5, out, sizeof(out), tag));
    CHECK(out[0] == 1 && out[1] == 7 && out[2] == 7 && out[3] == 7 && out[4] == 2);
}

static void test_rlew_rejects_corrupt(void)
{
    const uint16_t tag = 0xABCD;
    const uint16_t overflow[] = { tag, 9, 1 };
    const uint16_t truncated_run[] = { tag, 2 };
    const uint16_t short_stream[] = { 1, 2 };
    uint16_t out[4] = { 0 };

    CHECK(!CA_RLEWexpand(overflow, 3, out, sizeof(out), tag));
    CHECK(!CA_RLEWexpand(truncated_run, 2, out, sizeof(out), tag));
    CHECK(!CA_RLEWexpand(short_stream, 2, out, sizeof(out), tag));
}

static void test_data_paths(void)
{
    wolf_data_dir_t dir;
    char path[WOLF_PATH_MAX];

    memset(&dir, 0, sizeof(dir));
    snprintf(dir.path, sizeof(dir.path), "%s", "packdir");
    snprintf(dir.extension, sizeof(dir.extension), "%s", ".WL1");

    Wolf_MakeDataPath(&dir, "VSWAP", path, sizeof(path));
    CHECK(strstr(path, "packdir") == path);
    CHECK(strstr(path, "VSWAP.WL1") != NULL);

    snprintf(dir.path, sizeof(dir.path), "%s", "this_directory_does_not_exist");
    CHECK(!Wolf_CheckRequiredFiles(&dir));
}

static void check_layout(const gfx_layout_t *l)
{
    CHECK(l->numchunks <= NUMCHUNKS_MAX);
    CHECK(l->numpics == l->getpsychedpic - 2);
    CHECK(l->starttile8 == l->getpsychedpic + 1);
    CHECK(l->startexterns > l->starttile8 && l->startexterns < l->numchunks);
    CHECK(l->gunpic == l->knifepic + 1 && l->machinegunpic == l->knifepic + 2 &&
          l->gatlinggunpic == l->knifepic + 3);
    CHECK(l->face8apic == l->face1apic + 21);
    CHECK(l->gotgatlingpic == l->face8apic + 1 && l->getpsychedpic == l->pausedpic + 1);
    CHECK(l->t_demo0 >= l->startexterns && l->t_demo0 + NUMDEMOS <= l->numchunks);
}

static void test_gfx_layouts(void)
{
    check_layout(&gfx_layout_wl6);
    check_layout(&gfx_layout_wl1);
    CHECK(gfx_layout_wl6.numchunks == 149 && gfx_layout_wl6.numpics == 132);
    CHECK(gfx_layout_wl1.numchunks == 156 && gfx_layout_wl1.numpics == 144);
    CHECK(gfx_layout_wl6.t_demo0 == 139 && gfx_layout_wl1.t_demo0 == 151);
}

int main(void)
{
    test_carmack_literals();
    test_carmack_near_copy();
    test_carmack_far_copy();
    test_carmack_escaped_literal();
    test_carmack_rejects_corrupt();
    test_rlew();
    test_rlew_rejects_corrupt();
    test_data_paths();
    test_gfx_layouts();

    if (g_failures) {
        fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("test_decoders: all checks passed\n");
    return 0;
}
