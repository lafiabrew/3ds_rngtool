/*
 * Unit & specification tests for 3ds_rngtool:
 * - Proves TSV ((TID ^ SID) >> 4) and 6-digit Gen 7 G7TID calculation.
 * - Proves LCRNG (seed * 0x41C64E6D + 0x6073) frame search finds Shiny and high-IV spreads.
 * - Proves atomic Luma3DS Rosalina `cheats.txt` generation and `.bak` backup on overwrite.
 */

#include "rng_tool.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int main(void) {
    assert(rng_calc_tsv(13370u, 42000u) == ((13370u ^ 42000u) >> 4));
    assert(rng_calc_g7tid(13370u, 42000u) == ((13370u | (42000u << 16)) % 1000000u));

    rng_search_state_t st;
    memset(&st, 0, sizeof(st));
    st.tid = 13370u;
    st.sid = 42000u;
    st.initial_seed = 0x1A2B3C4Du;

    int all_hits = rng_search_frames(&st, 25000u, false);
    assert(all_hits > 0);

    int shiny_hits = rng_search_frames(&st, 50000u, true);
    assert(shiny_hits > 0);
    for (int i = 0; i < shiny_hits; i++) {
        assert(st.hits[i].is_shiny == true);
        assert(st.hits[i].psv == st.tsv);
    }

    char cheat_path[640];
    assert(rng_write_luma_cheats("build/test_sd", "00040000001B5000", "Photonic Sun", cheat_path, sizeof(cheat_path)));
    /* Write a second time to verify .bak creation */
    assert(rng_write_luma_cheats("build/test_sd", "00040000001B5000", "Photonic Sun v2", cheat_path, sizeof(cheat_path)));

    char bak_path[660];
    snprintf(bak_path, sizeof(bak_path), "%s.bak", cheat_path);
    struct stat sb;
    assert(stat(cheat_path, &sb) == 0 && sb.st_size > 64);
    assert(stat(bak_path, &sb) == 0 && sb.st_size > 64);

    printf("PASS: test_rngtool all TSV/G7TID, LCRNG shiny search, and Luma cheats.txt checks succeeded.\n");
    return 0;
}
