#include "pk_platform.h"
#include "rng_tool.h"

#include <stdio.h>
#include <string.h>

static const struct {
    const char *tid;
    const char *label;
} k_cheat_titles[4] = {
    { "00040000001B5000", "POKEMON ULTRA SUN / PHOTONIC SUN" },
    { "00040000001B5100", "POKEMON ULTRA MOON / PRISMATIC MOON" },
    { "000400000011C400", "POKEMON OMEGA RUBY / RUTILE RUBY" },
    { "0004000000055D00", "POKEMON X / ETERNAL X" }
};

typedef struct {
    char sd_root[RNG_PATH_MAX];
    rng_search_state_t rng;
    int selected_hit;
    int scroll_hit;
    int selected_title;
    bool shiny_only;
    char status_msg[128];
} rng_app_t;

static void sync_rng_scroll(rng_app_t *app) {
    int vis = 7;
    if (app->rng.hit_count <= vis) {
        app->scroll_hit = 0;
        return;
    }
    if (app->selected_hit < app->scroll_hit) app->scroll_hit = app->selected_hit;
    else if (app->selected_hit >= app->scroll_hit + vis) app->scroll_hit = app->selected_hit - vis + 1;
}

static void rng_app_init(rng_app_t *app, const char *sd_root) {
    memset(app, 0, sizeof(*app));
    snprintf(app->sd_root, sizeof(app->sd_root), "%s",
             sd_root != NULL ? sd_root : pk_platform_default_sd_root());
    app->rng.tid = 13370u;
    app->rng.sid = 42000u;
    app->rng.initial_seed = 0x1A2B3C4Du;
    app->shiny_only = false;
    int hits = rng_search_frames(&app->rng, 25000u, app->shiny_only);
    snprintf(app->status_msg, sizeof(app->status_msg),
             "TSV:%04u G7TID:%06u | %d RNG FRAMES FOUND",
             (unsigned)app->rng.tsv, (unsigned)app->rng.g7tid, hits);
    pk_log_event(app->sd_root, "rngtool", "INFO", app->status_msg);
}

static void rng_app_step(rng_app_t *app, const pk_input_t *in) {
    if ((in->down & PK_BTN_DUP) && app->rng.hit_count > 0) {
        app->selected_hit = (app->selected_hit > 0) ? (app->selected_hit - 1) : (app->rng.hit_count - 1);
    }
    if ((in->down & PK_BTN_DDOWN) && app->rng.hit_count > 0) {
        app->selected_hit = (app->selected_hit + 1 < app->rng.hit_count) ? (app->selected_hit + 1) : 0;
    }
    sync_rng_scroll(app);
    if (in->down & PK_BTN_DLEFT) {
        app->rng.initial_seed -= 0x100u;
        rng_search_frames(&app->rng, 25000u, app->shiny_only);
    }
    if (in->down & PK_BTN_DRIGHT) {
        app->rng.initial_seed += 0x100u;
        rng_search_frames(&app->rng, 25000u, app->shiny_only);
    }
    if (in->down & PK_BTN_Y) {
        app->shiny_only = !app->shiny_only;
        int hits = rng_search_frames(&app->rng, 50000u, app->shiny_only);
        app->selected_hit = 0;
        snprintf(app->status_msg, sizeof(app->status_msg),
                 "FILTER [%s]: %d FRAMES FOUND",
                 app->shiny_only ? "SHINY ONLY" : "SHINY + HIGH IV", hits);
    }
    if (in->down & PK_BTN_L) {
        app->selected_title = (app->selected_title + 3) % 4;
    }
    if (in->down & PK_BTN_R) {
        app->selected_title = (app->selected_title + 1) % 4;
    }
    bool write_cheats = (in->down & PK_BTN_A || in->down & PK_BTN_X) != 0;
    if (in->touch_just_pressed && in->touch_x >= 16 && in->touch_x <= 304 &&
        in->touch_y >= 48 && in->touch_y < 176) {
        int row = (in->touch_y - 48) / 32;
        if (row >= 0 && row < 4) {
            if (app->selected_title == row) write_cheats = true;
            else app->selected_title = row;
        }
    }
    if (write_cheats) {
        char out_path[RNG_PATH_MAX + 96];
        if (rng_write_luma_cheats(app->sd_root,
                                  k_cheat_titles[app->selected_title].tid,
                                  k_cheat_titles[app->selected_title].label,
                                  out_path, sizeof(out_path))) {
            snprintf(app->status_msg, sizeof(app->status_msg),
                     "WROTE LUMA CHEATS: %s", k_cheat_titles[app->selected_title].tid);
            pk_log_event(app->sd_root, "rngtool", "INFO", app->status_msg);
        }
    }
}

static void rng_app_render(const rng_app_t *app) {
    pk_render_begin_frame();
    pk_render_begin_target(PK_TARGET_TOP_LEFT, PK_RGBA8(22, 14, 34, 255));
    pk_draw_rect(0, 0, 400, 26, PK_RGBA8(52, 28, 82, 255));
    pk_draw_text(8, 7, 0.55f, PK_RGBA8(255, 215, 95, 255),
                 "3DS POKERNG - SHINY FRAME FINDER & LUMA CHEATS");

    char line[160];
    snprintf(line, sizeof(line), "SEED: 0x%08X | TID:%05u SID:%05u | TSV:%04u | G7ID:%06u",
             (unsigned)app->rng.initial_seed, (unsigned)app->rng.tid,
             (unsigned)app->rng.sid, (unsigned)app->rng.tsv, (unsigned)app->rng.g7tid);
    pk_draw_text(8, 34, 0.48f, PK_RGBA8(210, 190, 255, 255), line);

    pk_draw_rect(8, 52, 384, 152, PK_RGBA8(32, 22, 48, 255));
    pk_draw_rect_outline(8, 52, 384, 152, 1.0f, PK_RGBA8(140, 95, 215, 255));

    for (int row = 0; row < 7; row++) {
        int idx = app->scroll_hit + row;
        if (idx >= app->rng.hit_count) break;
        const rng_frame_hit_t *h = &app->rng.hits[idx];
        float y = 58.0f + (float)row * 20.0f;
        if (idx == app->selected_hit) {
            pk_draw_rect(12.0f, y - 2.0f, 368.0f, 18.0f, PK_RGBA8(78, 48, 122, 255));
        }
        snprintf(line, sizeof(line),
                 "%02d.F:%5u %s PID:%08X IV:%2u/%2u/%2u/%2u/%2u/%2u NAT:%2u",
                 idx + 1, (unsigned)h->frame,
                 h->is_square_shiny ? "SQ*" : (h->is_shiny ? "SH*" : "   "),
                 (unsigned)h->pid,
                 (unsigned)h->ivs[0], (unsigned)h->ivs[1], (unsigned)h->ivs[2],
                 (unsigned)h->ivs[3], (unsigned)h->ivs[4], (unsigned)h->ivs[5],
                 (unsigned)h->nature);
        pk_draw_text(16.0f, y + 2.0f, 0.43f,
                     h->is_shiny ? PK_RGBA8(255, 230, 90, 255) : PK_RGBA8(225, 220, 240, 255),
                     line);
    }
    pk_draw_scrollbar(384.0f, 56.0f, 144.0f, app->rng.hit_count, 7, app->scroll_hit,
                      PK_RGBA8(42, 28, 64, 255), PK_RGBA8(255, 215, 95, 255));

    pk_draw_rect(0, 216, 400, 24, PK_RGBA8(36, 20, 56, 255));
    pk_draw_text(8, 222, 0.48f, PK_RGBA8(130, 250, 180, 255), app->status_msg);

    /* Bottom Screen: Luma3DS Rosalina Cheats Generator */
    pk_render_begin_target(PK_TARGET_BOTTOM, PK_RGBA8(16, 12, 26, 255));
    pk_draw_rect(8, 12, 304, 216, PK_RGBA8(28, 20, 44, 255));
    pk_draw_text(16, 22, 0.5f, PK_RGBA8(255, 220, 110, 255),
                 "LUMA3DS ROSALINA CHEATS GENERATOR:");
    for (int t = 0; t < 4; t++) {
        float y = 48.0f + (float)t * 32.0f;
        bool sel = (t == app->selected_title);
        pk_draw_rect(16.0f, y, 288.0f, 26.0f,
                     sel ? PK_RGBA8(92, 54, 148, 255) : PK_RGBA8(38, 28, 58, 255));
        pk_draw_text(22.0f, y + 7.0f, 0.44f, PK_RGBA8(240, 235, 255, 255),
                     k_cheat_titles[t].label);
    }
    pk_draw_text(16, 188, 0.46f, PK_RGBA8(180, 225, 255, 255),
                 "L/R: SELECT GAME | A: WRITE CHEATS.TXT");
    pk_draw_text(16, 206, 0.46f, PK_RGBA8(180, 225, 255, 255),
                 "Y: TOGGLE SHINY FILTER | D-PAD: SEED");
    pk_render_end_frame();
}

int main(int argc, char **argv) {
    const char *sd_root = NULL;
    bool one_shot = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--sd-root") == 0 && i + 1 < argc) {
            sd_root = argv[++i];
        } else if (strcmp(argv[i], "--once") == 0) {
            one_shot = true;
        }
    }

    if (!pk_platform_init()) return 1;
    rng_app_t app;
    rng_app_init(&app, sd_root);

    while (pk_platform_main_loop()) {
        pk_input_t in;
        pk_platform_poll_input(&in);
        if (in.down & PK_BTN_START) break;
        rng_app_step(&app, &in);
        rng_app_render(&app);
        if (one_shot) break;
    }

    pk_platform_shutdown();
    return 0;
}
