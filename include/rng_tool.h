#ifndef RNG_TOOL_H
#define RNG_TOOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RNG_MAX_RESULTS 16
#define RNG_PATH_MAX 512

typedef struct {
    uint32_t frame;
    uint32_t seed;
    uint32_t pid;
    uint16_t psv;         /* 0..4095 */
    uint8_t nature;       /* 0..24 */
    uint8_t ability_slot; /* 0 or 1 */
    uint8_t ivs[6];       /* HP, Atk, Def, Spe, SpA, SpD (0..31) */
    bool is_shiny;
    bool is_square_shiny;
} rng_frame_hit_t;

typedef struct {
    uint16_t tid;
    uint16_t sid;
    uint16_t tsv;         /* (tid ^ sid) >> 4 */
    uint32_t g7tid;       /* (tid | (sid << 16)) % 1000000 */
    uint32_t initial_seed;
    int hit_count;
    rng_frame_hit_t hits[RNG_MAX_RESULTS];
} rng_search_state_t;

/* Compute 12-bit Trainer Shiny Value (0..4095) */
uint16_t rng_calc_tsv(uint16_t tid, uint16_t sid);

/* Compute 6-digit Gen 7 Display ID (000000..999999) */
uint32_t rng_calc_g7tid(uint16_t tid, uint16_t sid);

/* Advance standard Pokémon LCRNG (seed * 0x41C64E6D + 0x6073) */
uint32_t rng_lcrng_next(uint32_t seed);

/* Evaluate a Method-1 LCRNG frame from `seed` for a given `tid`/`sid` */
void rng_eval_method1_frame(uint32_t frame_idx, uint32_t seed, uint16_t tid, uint16_t sid, rng_frame_hit_t *out);

/* Search up to `max_frames` from `initial_seed` for Shiny or high-IV (5+ IVs >= 28) frames */
int rng_search_frames(rng_search_state_t *st, uint32_t max_frames, bool shiny_only);

/* Atomically write a Luma3DS Rosalina `cheats.txt` plugin file to `<sd_root>/luma/titles/<title_id>/cheats.txt` */
bool rng_write_luma_cheats(const char *sd_root, const char *title_id, const char *game_label, char *out_path, size_t cap);

#endif /* RNG_TOOL_H */
