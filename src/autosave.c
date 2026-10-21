/*
 * autosave.c -- Dedicated autosave subsystem for Ninja Gaiden Black (recomp).
 *
 * Automatically snapshots the player's live state (0x572F40, 0x17460 bytes)
 * upon door interactions and scene/room transitions, displays an EB Garamond
 * on-screen HUD banner (" [ AUTOSAVED ] "), and allows resuming from this
 * checkpoint upon death (Game Over -> Continue) or through the Load Game menu.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include "autosave.h"
#include "port.h"
#include "launcher.h"
#include "xbox_memory_layout.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#endif

/* Guest memory map references */
#define GUEST_STATE_ADDR      0x572F40u
#define GUEST_STATE_SIZE      0x17460u
#define GUEST_HP_ADDR         0x58A566u
#define GUEST_MAX_HP_ADDR     0x58A564u
#define GUEST_NINPO_ADDR      0x58A56Eu
#define GUEST_MAX_NINPO_ADDR  0x58A56Cu
#define GUEST_ESSENCE_ADDR    0x589570u
#define GUEST_CHAPTER_ADDR    0x589755u
#define GUEST_STAGE_ADDR      0x589761u
#define GUEST_ROOM_ADDR       0x589758u
#define GUEST_CHECKPOINT_ADDR 0x58A38Cu
#define GUEST_DOOR_ARRAY_ADDR 0x5840D0u
#define GUEST_DOOR_COUNT      48
#define GUEST_DOOR_STRIDE     0x104u
#define GUEST_MODE_ADDR       0xDB5A60u
#define GUEST_DEATH_ADDR      0x7F1B11u
#define GUEST_CHECKSUM_FLAGS  0x7F1A94u
#define GUEST_SLOT_COUNT_ADDR 0xDB9546u
#define GUEST_SLOT_MAP_ADDR   0xDB9547u
#define GUEST_METADATA_BASE   0x8F8630u
#define GUEST_LOAD_SLOT_ADDR  0xDA3E90u
#define GUEST_MENU_TYPE_ADDR  0xDB9897u
#define GUEST_SLOT_ARRAY_ADDR 0xC08DC0u

#define AUTOSAVE_SLOT_INDEX   29
#define BANNER_DURATION_MS    2500
#define AUTOSAVE_COOLDOWN_MS  3000

#define NGB_TITLE_ID_STR      "5443000d"
#define NGB_AUTOSAVE_MAGIC    "NGBAUTOSAVE_V1"

/* Autosave file header */
#pragma pack(push, 1)
struct autosave_header {
    char     magic[16];
    uint32_t version;
    uint32_t payload_size;
    uint32_t timestamp;
    uint16_t chapter;
    uint16_t stage;
    uint16_t room;
    uint16_t checkpoint;
    uint16_t hp;
    uint16_t max_hp;
    uint16_t ninpo;
    uint8_t  difficulty;
    uint8_t  reserved[17];
    uint32_t checksum;
};
#pragma pack(pop)

/* External renderer controls */
extern void nv2a_present_set_show_fps(bool show);
extern int ngb_pc_setting(int which);

/* Internal state */
static char s_save_path[1024];
static char s_xbox_save_path[1024];
static char s_xbox_dir_path[1024];
static bool s_initialized;
static uint64_t s_banner_until;
static uint64_t s_last_save_ms;

static uint8_t  s_last_mode;
static uint8_t  s_last_chapter;
static uint8_t  s_last_stage;
static uint8_t  s_last_room;
static uint16_t s_last_checkpoint;
static uint32_t s_last_door_hash;
static bool     s_player_died;

static uint64_t now_ms(void)
{
#if defined(_WIN32)
    return (uint64_t)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
#endif
}

static uint32_t calc_checksum(const uint8_t *data, size_t len)
{
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

static uint32_t compute_door_hash(void)
{
    uint32_t hash = 0x811c9dc5;
    for (int i = 0; i < GUEST_DOOR_COUNT; i++) {
        uint32_t addr = GUEST_DOOR_ARRAY_ADDR + (uint32_t)i * GUEST_DOOR_STRIDE;
        uint8_t state = MEM8(addr + 2);
        hash ^= (uint32_t)state << ((i % 4) * 8);
        hash *= 0x01000193;
    }
    return hash;
}

static void ensure_dir_exists(const char *file_path)
{
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s", file_path);
    char *p = tmp;
#if defined(_WIN32)
    if (p[0] && p[1] == ':') p += 2;
#endif
    for (; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char c = *p;
            *p = '\0';
            if (tmp[0] && !(tmp[1] == ':' && tmp[2] == '\0'))
                ngb_mkdir(tmp);
            *p = c;
        }
    }
}

static void write_save_meta(const char *dir_path)
{
    char meta_path[1100];
    snprintf(meta_path, sizeof(meta_path), "%s/SaveMeta.xbx", dir_path);
    FILE *f = fopen(meta_path, "wb");
    if (f) {
        /* UTF-16LE text: Name=AUTOSAVE\r\n */
        static const uint8_t bom_and_name[] = {
            0xFF, 0xFE,
            'N', 0, 'a', 0, 'm', 0, 'e', 0, '=', 0,
            'A', 0, 'U', 0, 'T', 0, 'O', 0, 'S', 0, 'A', 0, 'V', 0, 'E', 0,
            '\r', 0, '\n', 0
        };
        fwrite(bom_and_name, 1, sizeof(bom_and_name), f);
        fclose(f);
    }

    /* Also copy SaveImage.xbx from parent title directory if available */
    char src_img[1100], dst_img[1100];
    snprintf(src_img, sizeof(src_img), "%s/../SaveImage.xbx", dir_path);
    snprintf(dst_img, sizeof(dst_img), "%s/SaveImage.xbx", dir_path);
    FILE *fsrc = fopen(src_img, "rb");
    if (fsrc) {
        FILE *fdst = fopen(dst_img, "wb");
        if (fdst) {
            char buf[4096];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), fsrc)) > 0)
                fwrite(buf, 1, n, fdst);
            fclose(fdst);
        }
        fclose(fsrc);
    }
}

static void resolve_autosave_path(void)
{
    const char *dir = getenv("NGB_SAVE_DIR");
    if (dir && *dir) {
        snprintf(s_save_path, sizeof(s_save_path), "%s/UserData/" NGB_TITLE_ID_STR "/save_auto.dat", dir);
        snprintf(s_xbox_dir_path, sizeof(s_xbox_dir_path), "%s/UserData/" NGB_TITLE_ID_STR "/autosave", dir);
        snprintf(s_xbox_save_path, sizeof(s_xbox_save_path), "%s/UserData/" NGB_TITLE_ID_STR "/autosave/savegame.dat", dir);
        return;
    }
#if defined(_WIN32)
    const char *base = getenv("LOCALAPPDATA");
    const char *prefix = base && *base ? base : ".";
    snprintf(s_save_path, sizeof(s_save_path), "%s/NinjaGaidenBlack/UserData/" NGB_TITLE_ID_STR "/save_auto.dat", prefix);
    snprintf(s_xbox_dir_path, sizeof(s_xbox_dir_path), "%s/NinjaGaidenBlack/UserData/" NGB_TITLE_ID_STR "/autosave", prefix);
    snprintf(s_xbox_save_path, sizeof(s_xbox_save_path), "%s/NinjaGaidenBlack/UserData/" NGB_TITLE_ID_STR "/autosave/savegame.dat", prefix);
#else
    const char *xdg = getenv("XDG_DATA_HOME"), *home = getenv("HOME");
    if (xdg && *xdg) {
        snprintf(s_save_path, sizeof(s_save_path), "%s/NinjaGaidenBlack/UserData/" NGB_TITLE_ID_STR "/save_auto.dat", xdg);
        snprintf(s_xbox_dir_path, sizeof(s_xbox_dir_path), "%s/NinjaGaidenBlack/UserData/" NGB_TITLE_ID_STR "/autosave", xdg);
        snprintf(s_xbox_save_path, sizeof(s_xbox_save_path), "%s/NinjaGaidenBlack/UserData/" NGB_TITLE_ID_STR "/autosave/savegame.dat", xdg);
    } else {
        const char *h = home && *home ? home : ".";
        snprintf(s_save_path, sizeof(s_save_path), "%s/.local/share/NinjaGaidenBlack/UserData/" NGB_TITLE_ID_STR "/save_auto.dat", h);
        snprintf(s_xbox_dir_path, sizeof(s_xbox_dir_path), "%s/.local/share/NinjaGaidenBlack/UserData/" NGB_TITLE_ID_STR "/autosave", h);
        snprintf(s_xbox_save_path, sizeof(s_xbox_save_path), "%s/.local/share/NinjaGaidenBlack/UserData/" NGB_TITLE_ID_STR "/autosave/savegame.dat", h);
    }
#endif
}

const char *ngb_autosave_get_path(void)
{
    if (!s_initialized)
        ngb_autosave_init();
    return s_save_path;
}

void ngb_autosave_init(void)
{
    if (s_initialized)
        return;
    resolve_autosave_path();
    ensure_dir_exists(s_save_path);
    ensure_dir_exists(s_xbox_save_path);
    write_save_meta(s_xbox_dir_path);
    s_initialized = true;
    fprintf(stderr, "[AUTOSAVE] Initialized, target path: %s\n", s_save_path);
}

bool ngb_autosave_banner_active(void)
{
    return s_banner_until > now_ms();
}

static void trigger_ui_banner(void)
{
    s_banner_until = now_ms() + BANNER_DURATION_MS;
    nv2a_present_set_show_fps(true);
}

bool ngb_autosave_save(void)
{
    if (!s_initialized)
        ngb_autosave_init();

    uint16_t hp = MEM16(GUEST_HP_ADDR);
    uint16_t max_hp = MEM16(GUEST_MAX_HP_ADDR);
    if (!hp || hp >= 0x8000)
        return false;

    /* Synchronize combat HP and stats into state payload before snapshot */
    MEM16(GUEST_STATE_ADDR + 0x1664Au) = hp;
    MEM16(GUEST_STATE_ADDR + 0x16648u) = max_hp;

    for (uint32_t i = 0; i < 8; i += 2) {
        MEM16(0x58959Cu + i) = MEM16(0x58A534u + i);
        MEM16(0x58958Cu + i) = MEM16(0x58A524u + i);
    }

    const uint8_t *payload = (const uint8_t *)XBOX_PTR(GUEST_STATE_ADDR);
    if (!payload)
        return false;

    struct autosave_header hdr;
    memset(&hdr, 0, sizeof(hdr));
    memcpy(hdr.magic, NGB_AUTOSAVE_MAGIC, sizeof(hdr.magic));
    hdr.version = 1;
    hdr.payload_size = GUEST_STATE_SIZE;
    hdr.timestamp = (uint32_t)time(NULL);
    hdr.chapter = MEM8(GUEST_CHAPTER_ADDR);
    hdr.stage = MEM8(GUEST_STAGE_ADDR);
    hdr.room = MEM8(GUEST_ROOM_ADDR);
    hdr.checkpoint = MEM16(GUEST_CHECKPOINT_ADDR);
    hdr.hp = hp;
    hdr.max_hp = max_hp;
    hdr.ninpo = MEM16(GUEST_NINPO_ADDR);
    hdr.difficulty = MEM8(GUEST_STATE_ADDR + 0x16817u);
    hdr.checksum = calc_checksum(payload, GUEST_STATE_SIZE);

    /* 1. Write structured save_auto.dat atomically */
    char tmp_path[1100];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", s_save_path);
    ensure_dir_exists(tmp_path);

    FILE *f = fopen(tmp_path, "wb");
    if (!f) {
        fprintf(stderr, "[AUTOSAVE] Failed to open %s for writing\n", tmp_path);
        return false;
    }

    if (fwrite(&hdr, 1, sizeof(hdr), f) != sizeof(hdr) ||
        fwrite(payload, 1, GUEST_STATE_SIZE, f) != GUEST_STATE_SIZE) {
        fclose(f);
        remove(tmp_path);
        fprintf(stderr, "[AUTOSAVE] Write failed to %s\n", tmp_path);
        return false;
    }

    fflush(f);
    fclose(f);

    remove(s_save_path);
    if (rename(tmp_path, s_save_path) != 0) {
        fprintf(stderr, "[AUTOSAVE] Rename %s -> %s failed\n", tmp_path, s_save_path);
        return false;
    }

    /* 2. Write raw Xbox save payload into autosave/savegame.dat for native menu loading */
    char tmp_xbox[1100];
    snprintf(tmp_xbox, sizeof(tmp_xbox), "%s.tmp", s_xbox_save_path);
    ensure_dir_exists(tmp_xbox);

    FILE *fx = fopen(tmp_xbox, "wb");
    if (fx) {
        if (fwrite(payload, 1, GUEST_STATE_SIZE, fx) == GUEST_STATE_SIZE) {
            fflush(fx);
            fclose(fx);
            remove(s_xbox_save_path);
            rename(tmp_xbox, s_xbox_save_path);
        } else {
            fclose(fx);
            remove(tmp_xbox);
        }
    }
    write_save_meta(s_xbox_dir_path);

    s_last_save_ms = now_ms();
    trigger_ui_banner();
    fprintf(stderr, "[AUTOSAVE] State saved successfully (Chapter %u, Room %u, HP %u/%u)\n",
            hdr.chapter, hdr.room, hdr.hp, hdr.max_hp);
    return true;
}

bool ngb_autosave_load(void)
{
    if (!s_initialized)
        ngb_autosave_init();

    FILE *f = fopen(s_save_path, "rb");
    if (!f)
        return false;

    struct autosave_header hdr;
    if (fread(&hdr, 1, sizeof(hdr), f) != sizeof(hdr)) {
        fclose(f);
        return false;
    }

    if (memcmp(hdr.magic, NGB_AUTOSAVE_MAGIC, 12) != 0 ||
        hdr.payload_size != GUEST_STATE_SIZE) {
        fclose(f);
        fprintf(stderr, "[AUTOSAVE] Invalid header in %s\n", s_save_path);
        return false;
    }

    uint8_t *dst = (uint8_t *)XBOX_PTR(GUEST_STATE_ADDR);
    if (!dst || fread(dst, 1, GUEST_STATE_SIZE, f) != GUEST_STATE_SIZE) {
        fclose(f);
        return false;
    }
    fclose(f);

    uint32_t cs = calc_checksum(dst, GUEST_STATE_SIZE);
    if (cs != hdr.checksum) {
        fprintf(stderr, "[AUTOSAVE] Checksum mismatch (calc %08X != hdr %08X)\n", cs, hdr.checksum);
        return false;
    }

    /* Restore player health */
    uint16_t restored_hp = hdr.hp > 0 ? hdr.hp : hdr.max_hp;
    if (restored_hp == 0) restored_hp = 1000;
    MEM16(GUEST_STATE_ADDR + 0x1664Au) = restored_hp;
    MEM16(GUEST_STATE_ADDR + 0x16648u) = hdr.max_hp ? hdr.max_hp : 1000;
    MEM16(GUEST_HP_ADDR) = restored_hp;
    MEM16(GUEST_MAX_HP_ADDR) = hdr.max_hp ? hdr.max_hp : 1000;

    /* Acknowledge all anti-tamper checksum blocks */
    for (uint32_t a = GUEST_CHECKSUM_FLAGS; a < GUEST_CHECKSUM_FLAGS + 24; a++)
        MEM8(a) = 1;

    trigger_ui_banner();
    fprintf(stderr, "[AUTOSAVE] State loaded successfully (Chapter %u, Room %u, HP %u)\n",
            hdr.chapter, hdr.room, restored_hp);
    return true;
}

static bool autosave_file_exists(struct autosave_header *out_hdr)
{
    if (!s_initialized)
        ngb_autosave_init();
    FILE *f = fopen(s_save_path, "rb");
    if (!f)
        return false;
    struct autosave_header hdr;
    bool ok = fread(&hdr, 1, sizeof(hdr), f) == sizeof(hdr) &&
              memcmp(hdr.magic, NGB_AUTOSAVE_MAGIC, 12) == 0;
    fclose(f);
    if (ok && out_hdr)
        *out_hdr = hdr;
    return ok;
}

static void handle_load_menu_injection(void)
{
    /* Mode 3 is save/load menu. Byte at 0xDB9897 == 1 when in Load menu (0 when in Dragon Save menu). */
    uint8_t mode = MEM8(GUEST_MODE_ADDR);
    if (mode != 3)
        return;

    uint8_t menu_type = MEM8(GUEST_MENU_TYPE_ADDR);
    if (menu_type != 1) {
        /* Not the Load Game menu (e.g. Dragon Statue Save menu) -- do not inject */
        return;
    }

    struct autosave_header hdr;
    if (!autosave_file_exists(&hdr))
        return;

    uint8_t count = MEM8(GUEST_SLOT_COUNT_ADDR);
    if (count > 29) count = 29;

    /* Avoid duplicate injection if slot 29 is already in the slot map */
    for (int i = 0; i < count; i++) {
        if (MEM8(GUEST_SLOT_MAP_ADDR + i) == AUTOSAVE_SLOT_INDEX)
            return;
    }

    /* Inject Autosave slot into metadata array at slot index 29 */
    uint32_t meta = GUEST_METADATA_BASE + AUTOSAVE_SLOT_INDEX * 0x78u;
    MEM32(meta + 0x00) = hdr.timestamp;
    MEM16(meta + 0x0C) = hdr.max_hp;
    MEM16(meta + 0x0E) = hdr.hp;
    MEM16(meta + 0x10) = hdr.ninpo;
    MEM8(meta + 0x1A)  = (uint8_t)hdr.chapter;
    MEM8(meta + 0x1B)  = hdr.difficulty;
    MEM8(meta + 0x1C)  = 1;  /* Valid flag */
    memcpy((void *)XBOX_PTR(meta + 0x25), "  [AUTOSAVE]    ", 16);

    /* Inject slot path into slot array at 0xC08DC0 for native sub_001DA1B0 loader */
    uint32_t slot_entry = GUEST_SLOT_ARRAY_ADDR + (uint32_t)AUTOSAVE_SLOT_INDEX * 0x208u;
    MEM32(slot_entry) = 0;  /* Valid entry (not 0xFFFFFFFF) */
    char *slot_path1 = (char *)XBOX_PTR(slot_entry + 4);
    char *slot_path2 = (char *)XBOX_PTR(slot_entry + 4 + 0x104);
    if (slot_path1)
        snprintf(slot_path1, 0x104, "U:\\" NGB_TITLE_ID_STR "\\autosave");
    if (slot_path2)
        snprintf(slot_path2, 0x104, "U:\\" NGB_TITLE_ID_STR "\\autosave");
    MEM32(slot_entry + 4 + 0x204) = 0;

    /* Insert AUTOSAVE_SLOT_INDEX at top of menu slots list (row 0) */
    for (int i = (int)count; i > 0; i--)
        MEM8(GUEST_SLOT_MAP_ADDR + i) = MEM8(GUEST_SLOT_MAP_ADDR + (i - 1));
    MEM8(GUEST_SLOT_MAP_ADDR) = AUTOSAVE_SLOT_INDEX;
    MEM8(GUEST_SLOT_COUNT_ADDR) = count + 1;

    fprintf(stderr, "[AUTOSAVE] Injected [AUTOSAVE] slot into Load Menu (total %u slots)\n", count + 1);
}

void ngb_autosave_tick(void)
{
    if (!s_initialized)
        ngb_autosave_init();

    /* Manage UI banner timer */
    if (s_banner_until) {
        if (now_ms() >= s_banner_until) {
            s_banner_until = 0;
            if (!ngb_pc_setting(5)) /* Revert overlay if user's Show FPS setting is off */
                nv2a_present_set_show_fps(false);
        }
    }

    uint8_t mode = MEM8(GUEST_MODE_ADDR);
    uint16_t hp = MEM16(GUEST_HP_ADDR);
    uint8_t death = MEM8(GUEST_DEATH_ADDR);

    /* Track player death */
    if (hp == 0 || death != 0) {
        s_player_died = true;
    }

    /* Returning to Title/Demo resets death tracking so New Game won't trigger restore */
    if (mode == 1 || mode == 2 || mode == 4) {
        s_player_died = false;
    }

    /* Death -> Continue respawn detection:
     * When player died and mode transitions to 5 (scene reload from Continue),
     * restore from autosave snapshot so player restarts at the autosave point. */
    if (s_player_died && mode == 5) {
        if (autosave_file_exists(NULL)) {
            ngb_autosave_load();
            s_player_died = false;
            s_last_save_ms = now_ms();
            fprintf(stderr, "[AUTOSAVE] Resumed from autosave checkpoint after death\n");
        }
    }

    /* Handle Load Game menu slot injection */
    handle_load_menu_injection();

    /* Autosave trigger detection during active gameplay (Mode 6) */
    if (mode == 6 && hp > 0 && death == 0) {
        uint8_t  chapter    = MEM8(GUEST_CHAPTER_ADDR);
        uint8_t  stage      = MEM8(GUEST_STAGE_ADDR);
        uint8_t  room       = MEM8(GUEST_ROOM_ADDR);
        uint16_t checkpoint = MEM16(GUEST_CHECKPOINT_ADDR);
        uint32_t door_hash  = compute_door_hash();
        uint64_t now        = now_ms();

        bool scene_changed = (s_last_mode == 5);
        bool room_changed  = (chapter != s_last_chapter || stage != s_last_stage ||
                              room != s_last_room || checkpoint != s_last_checkpoint);
        bool door_changed  = (s_last_door_hash != 0 && door_hash != s_last_door_hash);

        bool ready = (now - s_last_save_ms >= AUTOSAVE_COOLDOWN_MS);

        if (ready && (scene_changed || room_changed || door_changed)) {
            ngb_autosave_save();
        }

        s_last_chapter    = chapter;
        s_last_stage      = stage;
        s_last_room       = room;
        s_last_checkpoint = checkpoint;
        s_last_door_hash  = door_hash;
    }

    s_last_mode = mode;
}
