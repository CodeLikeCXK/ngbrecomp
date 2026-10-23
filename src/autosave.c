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
#include <math.h>

#if defined(_WIN32)
#include <windows.h>
#endif

/* Guest memory map references */
#define GUEST_STATE_ADDR        0x572F40u
#define GUEST_STATE_SIZE        0x17460u
#define GUEST_LIVE_HP_ADDR      0x58958Au
#define GUEST_LIVE_MAX_HP_ADDR  0x589588u
#define GUEST_TRANS_HP_ADDR     0x58A566u
#define GUEST_TRANS_MAX_HP_ADDR 0x58A564u
#define GUEST_HP_ADDR           GUEST_LIVE_HP_ADDR
#define GUEST_MAX_HP_ADDR       GUEST_LIVE_MAX_HP_ADDR
#define GUEST_NINPO_ADDR        0x58A56Eu
#define GUEST_MAX_NINPO_ADDR    0x58A56Cu
#define GUEST_ESSENCE_ADDR      0x589570u
#define GUEST_CHAPTER_ADDR      0x589755u
#define GUEST_STAGE_ADDR        0x589761u
#define GUEST_ROOM_ADDR         0x589758u
#define GUEST_ROOM_TARGET       0x589756u
#define GUEST_ROOM_SUBAREA      0x58975Au
#define GUEST_ROOM_TRIG_ADDR    0x58975Bu
#define GUEST_ROOM_TRANS_FLAG   0x589753u
#define GUEST_CHECKPOINT_ADDR   0x58A38Cu
#define GUEST_DOOR_ACTIVE_ADDR  0x58A389u
#define GUEST_DOOR_TARGET_ADDR  0x58A38Au
#define GUEST_OBJ_ARRAY_ADDR    0x58A3A0u
#define GUEST_OBJ_COUNT         48
#define GUEST_OBJ_STRIDE        0x210u
#define GUEST_OBJ_DYN_ADDR      0xD52F60u
#define GUEST_INTERACTION_BUS   0xD69F20u
#define GUEST_INTERACT_FLAGS    0xDA41E8u
#define GUEST_INTERACT_STATE    0xDA420Au
#define GUEST_INTERACT_OBJ      0xDA4206u
#define GUEST_TARGET_OBJ_IDX    0x4B7640u
#define GUEST_MODE_ADDR         0xDB5A60u
#define GUEST_CHECKSUM_FLAGS    0x7F1A94u
#define GUEST_RELOAD_CMD_ADDR   0xBBBA36u
#define GUEST_INVENTORY_ADDR    0x5891D0u
#define GUEST_INVENTORY_COUNT   0x80u

#define BANNER_DURATION_MS      2500
#define AUTOSAVE_COOLDOWN_MS    1000

#define NGB_TITLE_ID_STR      "5443000d"
#define NGB_AUTOSAVE_MAGIC    "NGBAUTOSAVE_V1"

/* Autosave file header (64 bytes packed) */
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
    float    pos_x;
    float    pos_y;
    float    pos_z;
    float    pos_w;
    uint8_t  reserved[1];
    uint32_t checksum;
};
#pragma pack(pop)

/* External renderer controls */
extern bool nv2a_present_show_fps(void);
extern const char *nv2a_present_get_fps_text(void);
extern void nv2a_present_set_show_fps(bool show);
extern int ngb_pc_setting(int which);

/* Pending quick load request from F9 (processed synchronously on game thread) */
volatile int ngb_quick_load_request = 0;

/* Internal state */
static char s_save_path[1024];
static char s_xbox_dir_path[1024];
static bool s_initialized;
static uint64_t s_banner_until;
static char s_banner_text[64] = "[ AUTOSAVED ]";
static uint64_t s_last_save_ms;

static uint8_t  s_last_mode           = 0xFF;
static uint8_t  s_last_chapter        = 0xFF;
static uint8_t  s_last_stage          = 0xFF;
static uint8_t  s_last_room           = 0xFF;
static uint8_t  s_last_target_room    = 0xFF;
static uint8_t  s_last_subarea        = 0xFF;
static uint16_t s_last_checkpoint     = 0xFFFF;
static uint32_t s_last_obj_hash       = 0;
static uint32_t s_last_inv_hash       = 0;
static uint8_t  s_last_interact_state = 0;
static uint32_t s_last_interact_flags = 0;
static bool     s_last_bus_active     = false;
static bool     s_last_door_active    = false;
static float    s_last_saved_x        = 0.0f;
static float    s_last_saved_y        = 0.0f;
static float    s_last_saved_z        = 0.0f;
static bool     s_has_saved_pos       = false;
static const char *s_save_reason      = NULL;
static bool     s_save_pending        = false;
static bool     s_player_died         = false;

/* Quick load completion state */
static bool     s_quick_load_pending;
static uint8_t  s_quick_load_chapter;
static uint8_t  s_quick_load_stage;
static uint8_t  s_quick_load_room;
static uint16_t s_quick_load_hp;
static uint16_t s_quick_load_max_hp;
static float    s_quick_load_rx;
static float    s_quick_load_ry;
static float    s_quick_load_rz;
static float    s_quick_load_rw;

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

static uint16_t get_live_hp(void)
{
    uint16_t hp = MEM16(GUEST_LIVE_HP_ADDR);
    if (!hp || hp >= 0x8000)
        hp = MEM16(GUEST_TRANS_HP_ADDR);
    if (!hp || hp >= 0x8000)
        hp = MEM16(GUEST_STATE_ADDR + 0x1664Au);
    if (!hp || hp >= 0x8000)
        hp = 1000;
    return hp;
}

static uint16_t get_live_max_hp(void)
{
    uint16_t max_hp = MEM16(GUEST_LIVE_MAX_HP_ADDR);
    if (!max_hp || max_hp >= 0x8000)
        max_hp = MEM16(GUEST_TRANS_MAX_HP_ADDR);
    if (!max_hp || max_hp >= 0x8000)
        max_hp = MEM16(GUEST_STATE_ADDR + 0x16648u);
    if (!max_hp || max_hp >= 0x8000)
        max_hp = 1000;
    return max_hp;
}

static uint32_t compute_obj_table_hash(void)
{
    uint32_t hash = 0x811c9dc5u;
    for (int i = 0; i < GUEST_OBJ_COUNT; i++) {
        uint32_t addr = GUEST_OBJ_ARRAY_ADDR + (uint32_t)i * GUEST_OBJ_STRIDE;

        /* 11 bytes of status flags: active(+0x1F4), type(+0x1F5), subtype(+0x1F6), busy[8](+0x1F7..+0x1FE) */
        for (uint32_t o = 0x1F4; o <= 0x1FE; o++) {
            hash ^= (uint32_t)MEM8(addr + o);
            hash *= 0x01000193u;
        }

        /* 16 bytes of state entries: +0x1D0..+0x1DF */
        for (uint32_t o = 0x1D0; o <= 0x1DF; o += 2) {
            uint16_t s = MEM16(addr + o);
            hash ^= (uint32_t)(s & 0xFFu);
            hash *= 0x01000193u;
            hash ^= (uint32_t)((s >> 8) & 0xFFu);
            hash *= 0x01000193u;
        }

        /* 6 bytes of dynamic object state: +0x14..+0x19 in 0xD52F60 */
        uint32_t dyn_addr = GUEST_OBJ_DYN_ADDR + (uint32_t)i * 0x1Cu;
        uint16_t dyn_state = MEM16(dyn_addr + 0x14);
        uint16_t dyn_sub   = MEM16(dyn_addr + 0x16);
        uint8_t  dyn_f0    = MEM8(dyn_addr + 0x18);
        uint8_t  dyn_f1    = MEM8(dyn_addr + 0x19);

        hash ^= (uint32_t)(dyn_state & 0xFFu);
        hash *= 0x01000193u;
        hash ^= (uint32_t)((dyn_state >> 8) & 0xFFu);
        hash *= 0x01000193u;
        hash ^= (uint32_t)(dyn_sub & 0xFFu);
        hash *= 0x01000193u;
        hash ^= (uint32_t)dyn_f0;
        hash *= 0x01000193u;
        hash ^= (uint32_t)dyn_f1;
        hash *= 0x01000193u;
    }
    return hash;
}

static uint32_t compute_inventory_hash(void)
{
    uint32_t hash = 0x811c9dc5u;
    for (uint32_t a = GUEST_INVENTORY_ADDR; a < GUEST_INVENTORY_ADDR + GUEST_INVENTORY_COUNT * 4u; a += 4) {
        uint16_t item  = MEM16(a);
        uint8_t  count = MEM8(a + 2);
        hash ^= (uint32_t)item;
        hash *= 0x01000193u;
        hash ^= (uint32_t)count;
        hash *= 0x01000193u;
    }
    return hash;
}

static bool is_valid_position(float x, float y, float z)
{
    return (fabsf(x) > 0.1f || fabsf(y) > 0.1f || fabsf(z) > 0.1f);
}

static bool is_valid_header(const struct autosave_header *hdr)
{
    if (!hdr) return false;
    if (memcmp(hdr->magic, NGB_AUTOSAVE_MAGIC, 12) != 0) return false;
    if (hdr->payload_size != GUEST_STATE_SIZE) return false;
    if (hdr->chapter == 0 || hdr->chapter > 30) return false;
    if (!is_valid_position(hdr->pos_x, hdr->pos_y, hdr->pos_z)) return false;
    return true;
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

static void cleanup_legacy_autosave_dir(void)
{
    if (!s_xbox_dir_path[0])
        return;
    char file[1100];
    snprintf(file, sizeof(file), "%s/savegame.dat", s_xbox_dir_path);
    remove(file);
    snprintf(file, sizeof(file), "%s/save000.dat", s_xbox_dir_path);
    remove(file);
    snprintf(file, sizeof(file), "%s/SaveMeta.xbx", s_xbox_dir_path);
    remove(file);
    snprintf(file, sizeof(file), "%s/SaveImage.xbx", s_xbox_dir_path);
    remove(file);
#if defined(_WIN32)
    RemoveDirectoryA(s_xbox_dir_path);
#else
    rmdir(s_xbox_dir_path);
#endif
}

static void resolve_autosave_path(void)
{
    const char *dir = getenv("NGB_SAVE_DIR");
    if (dir && *dir) {
        snprintf(s_save_path, sizeof(s_save_path), "%s/UserData/" NGB_TITLE_ID_STR "/save_auto.dat", dir);
        snprintf(s_xbox_dir_path, sizeof(s_xbox_dir_path), "%s/UserData/" NGB_TITLE_ID_STR "/autosave", dir);
        return;
    }
#if defined(_WIN32)
    const char *base = getenv("LOCALAPPDATA");
    const char *prefix = base && *base ? base : ".";
    snprintf(s_save_path, sizeof(s_save_path), "%s/NinjaGaidenBlack/UserData/" NGB_TITLE_ID_STR "/save_auto.dat", prefix);
    snprintf(s_xbox_dir_path, sizeof(s_xbox_dir_path), "%s/NinjaGaidenBlack/UserData/" NGB_TITLE_ID_STR "/autosave", prefix);
#else
    const char *xdg = getenv("XDG_DATA_HOME"), *home = getenv("HOME");
    if (xdg && *xdg) {
        snprintf(s_save_path, sizeof(s_save_path), "%s/NinjaGaidenBlack/UserData/" NGB_TITLE_ID_STR "/save_auto.dat", xdg);
        snprintf(s_xbox_dir_path, sizeof(s_xbox_dir_path), "%s/NinjaGaidenBlack/UserData/" NGB_TITLE_ID_STR "/autosave", xdg);
    } else {
        const char *h = home && *home ? home : ".";
        snprintf(s_save_path, sizeof(s_save_path), "%s/.local/share/NinjaGaidenBlack/UserData/" NGB_TITLE_ID_STR "/save_auto.dat", h);
        snprintf(s_xbox_dir_path, sizeof(s_xbox_dir_path), "%s/.local/share/NinjaGaidenBlack/UserData/" NGB_TITLE_ID_STR "/autosave", h);
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
    cleanup_legacy_autosave_dir();
    s_initialized = true;
    fprintf(stderr, "[AUTOSAVE] Initialized, target path: %s\n", s_save_path);
}

bool ngb_autosave_banner_active(void)
{
    return s_banner_until > now_ms();
}

const char *ngb_autosave_banner_text(void)
{
    return s_banner_text[0] ? s_banner_text : "[ AUTOSAVED ]";
}

static void trigger_ui_banner(const char *text)
{
    if (text && *text)
        snprintf(s_banner_text, sizeof(s_banner_text), "%s", text);
    else
        snprintf(s_banner_text, sizeof(s_banner_text), "[ AUTOSAVED ]");
    s_banner_until = now_ms() + BANNER_DURATION_MS;
    fprintf(stderr, "[AUTOSAVE] UI banner triggered: %s for %d ms\n", s_banner_text, BANNER_DURATION_MS);
    fflush(stderr);
}

bool ngb_autosave_save(void)
{
    if (!s_initialized)
        ngb_autosave_init();

    uint8_t mode = MEM8(GUEST_MODE_ADDR);
    if (mode != 6) {
        return false;
    }

    uint8_t chapter = MEM8(GUEST_CHAPTER_ADDR);
    uint8_t stage = MEM8(GUEST_STAGE_ADDR);
    uint8_t room = MEM8(GUEST_ROOM_ADDR);
    if (chapter == 0 || chapter > 30) {
        return false;
    }

    /* Fetch live Ryu coordinates */
    float rx = MEMF(0x4B5710);
    float ry = MEMF(0x4B5714);
    float rz = MEMF(0x4B5718);
    float rw = MEMF(0x4B571C);
    if (!is_valid_position(rx, ry, rz)) {
        return false;
    }

    uint16_t hp = get_live_hp();
    uint16_t max_hp = get_live_max_hp();

    /* Write live coordinates into state buffer (offset 0x40) */
    MEMF(GUEST_STATE_ADDR + 0x40u) = rx;
    MEMF(GUEST_STATE_ADDR + 0x44u) = ry;
    MEMF(GUEST_STATE_ADDR + 0x48u) = rz;
    MEMF(GUEST_STATE_ADDR + 0x4Cu) = rw;

    /* Synchronize combat HP and stats into state payload before snapshot */
    MEM16(GUEST_STATE_ADDR + 0x1664Au) = hp;
    MEM16(GUEST_STATE_ADDR + 0x16648u) = max_hp;
    MEM16(GUEST_LIVE_HP_ADDR) = hp;
    MEM16(GUEST_LIVE_MAX_HP_ADDR) = max_hp;
    MEM16(GUEST_TRANS_HP_ADDR) = hp;
    MEM16(GUEST_TRANS_MAX_HP_ADDR) = max_hp;

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
    hdr.chapter = chapter;
    hdr.stage = stage;
    hdr.room = room;
    hdr.checkpoint = MEM16(GUEST_CHECKPOINT_ADDR);
    hdr.hp = hp;
    hdr.max_hp = max_hp;
    hdr.ninpo = MEM16(GUEST_NINPO_ADDR);
    hdr.difficulty = MEM8(GUEST_STATE_ADDR + 0x16817u);
    hdr.pos_x = rx;
    hdr.pos_y = ry;
    hdr.pos_z = rz;
    hdr.pos_w = rw;
    hdr.checksum = calc_checksum(payload, GUEST_STATE_SIZE);

    /* Write structured save_auto.dat atomically */
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

    s_last_save_ms = now_ms();
    s_last_saved_x = rx;
    s_last_saved_y = ry;
    s_last_saved_z = rz;
    s_has_saved_pos = true;
    trigger_ui_banner("[ AUTOSAVED ]");
    fprintf(stderr, "[AUTOSAVE] State saved successfully [%s] (Chapter %u, Stage %u, Room %u, HP %u/%u, Pos %.1f %.1f %.1f)\n",
            s_save_reason ? s_save_reason : "checkpoint",
            hdr.chapter, hdr.stage, hdr.room, hdr.hp, hdr.max_hp, rx, ry, rz);
    fflush(stderr);
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
    bool ok = (fread(&hdr, 1, sizeof(hdr), f) == sizeof(hdr)) && is_valid_header(&hdr);
    fclose(f);
    if (ok && out_hdr)
        *out_hdr = hdr;
    return ok;
}

static bool ngb_autosave_load_ex(struct autosave_header *out_hdr)
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

    if (!is_valid_header(&hdr)) {
        fclose(f);
        fprintf(stderr, "[AUTOSAVE] Invalid header or corrupted data in %s\n", s_save_path);
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
    uint16_t max_hp = hdr.max_hp > 0 ? hdr.max_hp : 1000;
    MEM16(GUEST_STATE_ADDR + 0x1664Au) = restored_hp;
    MEM16(GUEST_STATE_ADDR + 0x16648u) = max_hp;
    MEM16(GUEST_LIVE_HP_ADDR) = restored_hp;
    MEM16(GUEST_LIVE_MAX_HP_ADDR) = max_hp;
    MEM16(GUEST_TRANS_HP_ADDR) = restored_hp;
    MEM16(GUEST_TRANS_MAX_HP_ADDR) = max_hp;
    MEM16(GUEST_NINPO_ADDR) = hdr.ninpo;

    /* Restore chapter / stage / room / checkpoint */
    MEM8(GUEST_CHAPTER_ADDR) = (uint8_t)hdr.chapter;
    MEM8(GUEST_STAGE_ADDR)   = (uint8_t)hdr.stage;
    MEM8(GUEST_ROOM_ADDR)    = (uint8_t)hdr.room;
    MEM16(GUEST_CHECKPOINT_ADDR) = hdr.checkpoint;

    /* Restore Ryu's coordinates */
    float rx = hdr.pos_x;
    float ry = hdr.pos_y;
    float rz = hdr.pos_z;
    float rw = hdr.pos_w;
    MEMF(0x4B5710) = rx;
    MEMF(0x4B5714) = ry;
    MEMF(0x4B5718) = rz;
    MEMF(0x4B571C) = rw;
    MEMF(GUEST_STATE_ADDR + 0x40u) = rx;
    MEMF(GUEST_STATE_ADDR + 0x44u) = ry;
    MEMF(GUEST_STATE_ADDR + 0x48u) = rz;
    MEMF(GUEST_STATE_ADDR + 0x4Cu) = rw;
    for (uint32_t a = 0x4B5000; a < 0x4B8000; a += 16) {
        float ox = MEMF(a), oy = MEMF(a + 4), oz = MEMF(a + 8);
        if (fabsf(ox - rx) < 50.0f && fabsf(oy - ry) < 50.0f && fabsf(oz - rz) < 50.0f) {
            MEMF(a)     = rx;
            MEMF(a + 4) = ry;
            MEMF(a + 8) = rz;
        }
    }

    /* Acknowledge all anti-tamper checksum blocks */
    for (uint32_t a = GUEST_CHECKSUM_FLAGS; a < GUEST_CHECKSUM_FLAGS + 24; a++)
        MEM8(a) = 1;

    if (out_hdr)
        *out_hdr = hdr;

    fprintf(stderr, "[AUTOSAVE] State loaded successfully (Chapter %u, Stage %u, Room %u, HP %u, Pos %.1f %.1f %.1f)\n",
            hdr.chapter, hdr.stage, hdr.room, restored_hp, rx, ry, rz);
    return true;
}

bool ngb_autosave_load(void)
{
    return ngb_autosave_load_ex(NULL);
}

static bool ngb_autosave_do_quick_load(void)
{
    if (!s_initialized)
        ngb_autosave_init();

    struct autosave_header hdr;
    if (!autosave_file_exists(&hdr)) {
        trigger_ui_banner("[ NO AUTOSAVE FOUND ]");
        fprintf(stderr, "[AUTOSAVE] Quick load requested, but no valid autosave exists\n");
        return false;
    }

    if (!ngb_autosave_load_ex(&hdr)) {
        trigger_ui_banner("[ LOAD FAILED ]");
        return false;
    }

    uint8_t mode = MEM8(GUEST_MODE_ADDR);

    /* If currently in main or save/load menu, complete menu transition */
    if (mode == 1 || mode == 2 || mode == 3) {
        MEM32(0xDA3E90) = 5;
        MEM8(0xDA41DF) = 1;
    }

    /* Setup room reload target matching the saved state */
    uint8_t target_room = (uint8_t)hdr.room;
    MEM8(GUEST_ROOM_TARGET) = target_room;
    MEM8(0xDB9AC6)          = target_room;
    MEM8(GUEST_ROOM_ADDR)   = 0xFF; /* Force full reload of scene objects */
    MEM8(GUEST_STAGE_ADDR)  = (uint8_t)hdr.stage;
    MEM8(GUEST_CHAPTER_ADDR)= (uint8_t)hdr.chapter;
    MEM16(GUEST_CHECKPOINT_ADDR) = hdr.checkpoint;

    /* Transition protocol flags for task 0x2DF590 / sub_002FC110 */
    MEM8(0x589753) = 1;  /* GUEST_ROOM_TRANS_FLAG */
    MEM8(0x58A389) = 0;  /* GUEST_DOOR_ACTIVE_ADDR */
    MEM8(0x58975B) = 0;  /* GUEST_ROOM_TRIG_ADDR */
    MEM8(0x58975A) = 1;  /* subarea */
    MEM8(0xDB9AD6) = 0;
    MEM8(0xBBBA36) = 0;  /* NEVER set to 3; 3 invokes sub_0016BB50(3) which sets Mode 0 */
    MEM8(0xBBBA37) = 0;
    MEM8(0xBBBA38) = 0;
    MEM8(0xBBBA39) = 0;

    /* Transition engine to Mode 5 (scene reconstructor) */
    MEM8(GUEST_MODE_ADDR) = 5;

    /* Arm quick-load completion guard */
    s_quick_load_pending = true;
    s_quick_load_chapter = (uint8_t)hdr.chapter;
    s_quick_load_stage   = (uint8_t)hdr.stage;
    s_quick_load_room    = target_room;
    s_quick_load_hp      = hdr.hp > 0 ? hdr.hp : hdr.max_hp;
    s_quick_load_max_hp  = hdr.max_hp > 0 ? hdr.max_hp : 1000;
    s_quick_load_rx      = hdr.pos_x;
    s_quick_load_ry      = hdr.pos_y;
    s_quick_load_rz      = hdr.pos_z;
    s_quick_load_rw      = hdr.pos_w;

    s_player_died  = false;
    s_save_pending = false;
    s_save_reason  = NULL;
    return true;
}

bool ngb_autosave_quick_load(void)
{
    ngb_quick_load_request = 1;
    return true;
}

void ngb_autosave_tick(void)
{
    if (!s_initialized)
        ngb_autosave_init();

    /* Manage UI banner timer */
    if (s_banner_until && now_ms() >= s_banner_until) {
        s_banner_until = 0;
    }

    /* Process pending quick load request (e.g. from F9 key) synchronously on game thread */
    if (ngb_quick_load_request) {
        ngb_quick_load_request = 0;
        ngb_autosave_do_quick_load();
    }

    uint8_t mode = MEM8(GUEST_MODE_ADDR);
    uint8_t chapter = MEM8(GUEST_CHAPTER_ADDR);
    uint8_t stage = MEM8(GUEST_STAGE_ADDR);
    uint8_t room = MEM8(GUEST_ROOM_ADDR);

    /* Death tracking: Mode 9 is Game Over screen in NGB */
    if (mode == 9) {
        s_player_died = true;
    }

    /* Reset death tracking if outside gameplay/reload modes (Title 1, Demo 2, SaveMenu 3, PauseMenu 4) */
    if (mode == 1 || mode == 2 || mode == 3 || mode == 4) {
        s_player_died = false;
        s_quick_load_pending = false;
    }

    /* Check quick-load post-completion: when engine returns to Mode 6 and position is valid */
    float cur_rx = MEMF(0x4B5710);
    float cur_ry = MEMF(0x4B5714);
    float cur_rz = MEMF(0x4B5718);
    bool has_cur_pos = is_valid_position(cur_rx, cur_ry, cur_rz);

    if (s_quick_load_pending && mode == 6 && has_cur_pos) {
        /* Enforce player position and health upon room entry */
        MEMF(0x4B5710) = s_quick_load_rx;
        MEMF(0x4B5714) = s_quick_load_ry;
        MEMF(0x4B5718) = s_quick_load_rz;
        MEMF(0x4B571C) = s_quick_load_rw;
        MEM16(GUEST_LIVE_HP_ADDR) = s_quick_load_hp;
        MEM16(GUEST_LIVE_MAX_HP_ADDR) = s_quick_load_max_hp;
        MEM16(GUEST_TRANS_HP_ADDR) = s_quick_load_hp;
        MEM16(GUEST_TRANS_MAX_HP_ADDR) = s_quick_load_max_hp;

        for (uint32_t a = 0x4B5000; a < 0x4B8000; a += 16) {
            float ox = MEMF(a), oy = MEMF(a + 4), oz = MEMF(a + 8);
            if (fabsf(ox - s_quick_load_rx) < 50.0f &&
                fabsf(oy - s_quick_load_ry) < 50.0f &&
                fabsf(oz - s_quick_load_rz) < 50.0f) {
                MEMF(a)     = s_quick_load_rx;
                MEMF(a + 4) = s_quick_load_ry;
                MEMF(a + 8) = s_quick_load_rz;
            }
        }

        s_quick_load_pending = false;
        s_last_save_ms       = now_ms(); /* Don't immediately re-save */
        s_save_pending       = false;
        s_save_reason        = NULL;
        s_last_chapter       = chapter;
        s_last_stage         = stage;
        s_last_room          = room;
        s_last_subarea       = MEM8(GUEST_ROOM_SUBAREA);
        s_last_checkpoint    = MEM16(GUEST_CHECKPOINT_ADDR);
        s_last_obj_hash      = compute_obj_table_hash();
        s_last_inv_hash      = compute_inventory_hash();
        s_last_saved_x       = s_quick_load_rx;
        s_last_saved_y       = s_quick_load_ry;
        s_last_saved_z       = s_quick_load_rz;
        s_has_saved_pos      = true;

        trigger_ui_banner("[ AUTOSAVE LOADED ]");
        fprintf(stderr, "[AUTOSAVE] Quick load completed into Chapter %u, Stage %u, Room %u at (%.1f, %.1f, %.1f)\n",
                chapter, stage, room, s_quick_load_rx, s_quick_load_ry, s_quick_load_rz);
    }

    /* Death -> Continue respawn detection:
     * When player died and mode transitions to 5 (Continue from Game Over),
     * restore from autosave snapshot so player restarts at the autosave point. */
    if (s_player_died && mode == 5) {
        if (autosave_file_exists(NULL)) {
            ngb_autosave_do_quick_load();
            s_player_died  = false;
            s_save_pending = false;
            s_save_reason  = NULL;
            fprintf(stderr, "[AUTOSAVE] Resumed from autosave checkpoint after death\n");
        }
    }

    /* Active game state: gameplay (mode 6) or scene/door transitions (5, 0xC, 9, 0xD..0x12) */
    if (mode == 6 || mode == 5 || mode == 0xC || (mode >= 0xD && mode <= 0x12)) {
        uint8_t  target_room     = MEM8(GUEST_ROOM_TARGET);
        uint8_t  subarea         = MEM8(GUEST_ROOM_SUBAREA);
        uint16_t checkpoint      = MEM16(GUEST_CHECKPOINT_ADDR);
        uint8_t  interact_state  = MEM8(GUEST_INTERACT_STATE);
        uint32_t interact_flags  = MEM32(GUEST_INTERACT_FLAGS);
        bool     bus_active      = (MEM32(GUEST_INTERACTION_BUS) != 0 || MEM32(GUEST_INTERACTION_BUS + 4) != 0);
        bool     door_active     = (MEM8(GUEST_DOOR_ACTIVE_ADDR) != 0 || MEM8(GUEST_ROOM_TRANS_FLAG) != 0 || MEM8(GUEST_ROOM_TRIG_ADDR) != 0);
        uint32_t obj_hash        = compute_obj_table_hash();
        uint32_t inv_hash        = compute_inventory_hash();

        float rx = MEMF(0x4B5710);
        float ry = MEMF(0x4B5714);
        float rz = MEMF(0x4B5718);
        bool has_pos = is_valid_position(rx, ry, rz);

        if (s_last_obj_hash == 0 && mode == 6 && has_pos) {
            s_last_obj_hash = obj_hash;
        }
        if (s_last_inv_hash == 0 && mode == 6 && has_pos) {
            s_last_inv_hash = inv_hash;
        }

        const char *trigger = NULL;

        /* Tier 1: Direct player interaction (door/gate/chest activation via prompt or X key) */
        if (mode == 6 && ((interact_state != 0 && s_last_interact_state == 0) ||
                          ((interact_flags & 0x80000000u) && !(s_last_interact_flags & 0x80000000u)))) {
            trigger = "player door/object interaction";
            uint8_t obj_idx = MEM8(GUEST_TARGET_OBJ_IDX);
            uint16_t obj_id = MEM16(GUEST_INTERACT_OBJ);
            fprintf(stderr, "[AUTOSAVE] Player interaction detected: target_idx=%u, obj_id=0x%04X, state=0x%02X, flags=0x%08X\n",
                    obj_idx, obj_id, interact_state, interact_flags);
        }
        /* Tier 2: Interaction bus active */
        else if (mode == 6 && bus_active && !s_last_bus_active) {
            trigger = "interaction bus active";
            fprintf(stderr, "[AUTOSAVE] Interaction bus active: bus=0x%08X 0x%08X\n",
                    MEM32(GUEST_INTERACTION_BUS), MEM32(GUEST_INTERACTION_BUS + 4));
        }
        /* Tier 3: Door / gate activation signal */
        else if (mode == 6 && door_active && !s_last_door_active) {
            trigger = "door / gate transition active";
            fprintf(stderr, "[AUTOSAVE] Door transition active: door_active=%u, trans_flag=%u, trig=%u\n",
                    MEM8(GUEST_DOOR_ACTIVE_ADDR), MEM8(GUEST_ROOM_TRANS_FLAG), MEM8(GUEST_ROOM_TRIG_ADDR));
        }
        /* Tier 4: Inventory change (chest opened: key, scroll, scarab, or item acquired) */
        else if (mode == 6 && s_last_inv_hash != 0 && inv_hash != s_last_inv_hash) {
            trigger = "chest opened / item acquired";
            fprintf(stderr, "[AUTOSAVE] Inventory hash changed: 0x%08X -> 0x%08X\n",
                    s_last_inv_hash, inv_hash);
        }
        /* Tier 5: Real scene object table state change (door opened, chest opened, busy flag flipped) */
        else if (mode == 6 && s_last_obj_hash != 0 && obj_hash != s_last_obj_hash) {
            trigger = "scene door/object state change";
            fprintf(stderr, "[AUTOSAVE] Object table hash changed: 0x%08X -> 0x%08X\n",
                    s_last_obj_hash, obj_hash);
        }
        /* Tier 6: Room / stage / checkpoint / chapter transition */
        else if (chapter != s_last_chapter || stage != s_last_stage ||
                 room != s_last_room || checkpoint != s_last_checkpoint ||
                 subarea != s_last_subarea) {
            trigger = "room/stage transition";
            fprintf(stderr, "[AUTOSAVE] Room/stage transition: Ch %u->%u, St %u->%u, Rm %u->%u, Sub %u->%u, CP %u->%u\n",
                    s_last_chapter, chapter, s_last_stage, stage, s_last_room, room,
                    s_last_subarea, subarea, s_last_checkpoint, checkpoint);
        }
        /* Tier 7: Target room transition dispatcher */
        else if (target_room != room && target_room != 0 && target_room != s_last_target_room) {
            trigger = "target room transition";
            fprintf(stderr, "[AUTOSAVE] Target room transition set: Rm %u -> Target %u\n", room, target_room);
        }
        /* Tier 8: Scene returned from loading/cutscene to gameplay */
        else if ((s_last_mode != 6 && s_last_mode != 4 && s_last_mode != 3 && s_last_mode != 0xFF) && (mode == 6)) {
            trigger = "scene returned to gameplay";
            fprintf(stderr, "[AUTOSAVE] Scene returned to gameplay mode 6 from mode %u\n", s_last_mode);
        }
        /* Tier 9: Exploration distance checkpoint */
        else if (mode == 6 && has_pos && s_has_saved_pos) {
            float dx = rx - s_last_saved_x;
            float dy = ry - s_last_saved_y;
            float dz = rz - s_last_saved_z;
            float dist_sq = dx * dx + dy * dy + dz * dz;
            if (dist_sq > 2250000.0f && (now_ms() - s_last_save_ms >= 15000)) {
                trigger = "exploration distance checkpoint";
                fprintf(stderr, "[AUTOSAVE] Exploration distance checkpoint (dist=%.1f)\n", sqrtf(dist_sq));
            }
        }

        if (trigger && !s_quick_load_pending) {
            s_save_pending = true;
            s_save_reason  = trigger;
        }

        uint64_t now = now_ms();
        bool cooldown_passed = (now - s_last_save_ms >= AUTOSAVE_COOLDOWN_MS);
        bool valid_gameplay = (mode == 6) &&
                              (chapter > 0 && chapter < 30) &&
                              has_pos &&
                              !s_quick_load_pending;

        if (s_save_pending && valid_gameplay && cooldown_passed) {
            if (ngb_autosave_save()) {
                s_save_pending     = false;
                s_save_reason      = NULL;
                s_last_chapter     = chapter;
                s_last_stage       = stage;
                s_last_room        = room;
                s_last_target_room = target_room;
                s_last_subarea     = subarea;
                s_last_checkpoint  = checkpoint;
                s_last_obj_hash    = obj_hash;
                s_last_inv_hash    = inv_hash;
            }
        }

        if (valid_gameplay) {
            s_last_obj_hash = obj_hash;
            s_last_inv_hash = inv_hash;
        }
        s_last_interact_state = interact_state;
        s_last_interact_flags = interact_flags;
        s_last_bus_active     = bus_active;
        s_last_door_active    = door_active;
        s_last_target_room    = target_room;
    }

    s_last_mode = mode;
}

/* 5x7 ASCII bitmap font glyphs for DX12 overlay style */
struct glyph5x7 {
    char c;
    uint8_t rows[7];
};

static const uint8_t *get_glyph_5x7(char c)
{
    static const struct glyph5x7 table[] = {
        /* Digits */
        { '0', { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E } },
        { '1', { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E } },
        { '2', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F } },
        { '3', { 0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E } },
        { '4', { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 } },
        { '5', { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E } },
        { '6', { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E } },
        { '7', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 } },
        { '8', { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E } },
        { '9', { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C } },
        /* Uppercase Letters */
        { 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
        { 'B', { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E } },
        { 'C', { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } },
        { 'D', { 0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C } },
        { 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
        { 'F', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 } },
        { 'G', { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F } },
        { 'H', { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
        { 'I', { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E } },
        { 'J', { 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C } },
        { 'K', { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 } },
        { 'L', { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } },
        { 'M', { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 } },
        { 'N', { 0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11 } },
        { 'O', { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
        { 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
        { 'Q', { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D } },
        { 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
        { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
        { 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
        { 'U', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
        { 'V', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 } },
        { 'W', { 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A } },
        { 'X', { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 } },
        { 'Y', { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 } },
        { 'Z', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F } },
        /* Lowercase letters */
        { 'a', { 0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F } },
        { 'b', { 0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x1E } },
        { 'c', { 0x00, 0x00, 0x0E, 0x10, 0x10, 0x11, 0x0E } },
        { 'd', { 0x01, 0x01, 0x0D, 0x13, 0x11, 0x11, 0x0F } },
        { 'e', { 0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E } },
        { 'f', { 0x06, 0x09, 0x08, 0x1C, 0x08, 0x08, 0x08 } },
        { 'g', { 0x00, 0x00, 0x0F, 0x11, 0x0F, 0x01, 0x0E } },
        { 'h', { 0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x11 } },
        { 'i', { 0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E } },
        { 'j', { 0x02, 0x00, 0x06, 0x02, 0x02, 0x12, 0x0C } },
        { 'k', { 0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12 } },
        { 'l', { 0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E } },
        { 'm', { 0x00, 0x00, 0x1A, 0x15, 0x15, 0x11, 0x11 } },
        { 'n', { 0x00, 0x00, 0x16, 0x19, 0x11, 0x11, 0x11 } },
        { 'o', { 0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E } },
        { 'p', { 0x00, 0x00, 0x1E, 0x11, 0x1E, 0x10, 0x10 } },
        { 'r', { 0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10 } },
        { 's', { 0x00, 0x00, 0x0E, 0x10, 0x0E, 0x01, 0x1E } },
        { 't', { 0x08, 0x08, 0x1C, 0x08, 0x08, 0x09, 0x06 } },
        { 'u', { 0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D } },
        { 'v', { 0x00, 0x00, 0x11, 0x11, 0x11, 0x0A, 0x04 } },
        { 'w', { 0x00, 0x00, 0x11, 0x15, 0x15, 0x15, 0x0A } },
        { 'x', { 0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11 } },
        { 'y', { 0x00, 0x00, 0x11, 0x11, 0x0F, 0x01, 0x0E } },
        { 'z', { 0x00, 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F } },
        /* Symbols */
        { '[', { 0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E } },
        { ']', { 0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E } },
        { '-', { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 } },
        { '.', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C } },
        { ':', { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 } },
        { '/', { 0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00 } },
        { '!', { 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 } },
        { '%', { 0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13 } },
        { '(', { 0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02 } },
        { ')', { 0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08 } },
        { '_', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F } },
    };
    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (table[i].c == c)
            return table[i].rows;
    }
    return NULL;
}

bool ngb_autosave_get_bitmap(uint32_t *px, uint32_t max_w, uint32_t max_h,
                             uint32_t *ow, uint32_t *oh,
                             uint32_t screen_w, uint32_t screen_h)
{
    if (!ngb_autosave_banner_active())
        return false;
    if (!px || !ow || !oh || !screen_w || !screen_h)
        return false;

    const char *text = s_banner_text[0] ? s_banner_text : "[ AUTOSAVED ]";

    unsigned k = screen_h / 360;
    if (k < 2) k = 2;
    if (k > 8) k = 8;

    uint32_t w = (uint32_t)(strlen(text) * 6 + 5) * k;
    uint32_t h = 13 * k;
    if (w > max_w) w = max_w;
    if (h > max_h) h = max_h;
    if (w > screen_w) w = screen_w;
    if (h > screen_h) h = screen_h;

    for (uint32_t i = 0; i < w * h; i++)
        px[i] = 0xC0181818u;

    for (int ci = 0; text[ci]; ci++) {
        const uint8_t *g = get_glyph_5x7(text[ci]);
        if (!g)
            continue;
        for (int gy = 0; gy < 7; gy++) {
            for (int gx = 0; gx < 5; gx++) {
                if (!(g[gy] & (0x10 >> gx)))
                    continue;
                for (unsigned yy = 0; yy < k; yy++) {
                    for (unsigned xx = 0; xx < k; xx++) {
                        uint32_t x = (3 + ci * 6 + gx) * k + xx;
                        uint32_t y = (3 + gy) * k + yy;
                        if (x < w && y < h)
                            px[y * w + x] = 0xFFFFFFFFu;
                    }
                }
            }
        }
    }

    *ow = w;
    *oh = h;
    return true;
}
