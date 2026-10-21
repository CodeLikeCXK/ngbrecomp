/*
 * cheats.c -- the RECOMP SETTINGS cheats, applied once a frame on the
 * game's thread (from Direct3D's Swap, src/d3d/d3d.c). They are for this
 * run only: not saved, off at the next start.
 *
 * Ryu's values (the player's block at 0x58A520, 0x210 bytes a player):
 * 0x58A564 the most health, 0x58A566 the health (16 bits; the HUD bar
 * follows it), 0x58A56C the most ninpo, 0x58A56E the ninpo. 0x589570 is
 * the essence (32 bits, the game keeps it at most 99999).
 *
 * The game guards these: 24 checksums (sub_00164E50, each over a part of the
 * state -- the inventory, health, ninpo, essence, ...) taken each frame, and
 * one changed behind its back (without the flag its own code sets with the
 * change, 0x7F1A94 + n) sends it back to the title screen. A frame a cheat
 * writes in, every flag is set: the checksums are taken anew.
 *
 * NGB_MEMDUMP=<dir>: debugging, for finding the game's values -- while the
 * file <dir>/dump.now exists, the guest's 64 MB go to <dir>/mem_<n>.bin
 * (the file is removed after).
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbox_memory_layout.h"

/* A debugging option from the environment, read once (these are asked per sprite or per frame). */
#define ENV_ONCE(name) (__extension__({ static const char *v_ = (const char *)1; \
    if (v_ == (const char *)1) v_ = getenv(name); v_; }))


static void memdump_tick(void)
{
    static const char *dir = (const char *)1;
    static int n;
    if (dir == (const char *)1)
        dir = getenv("NGB_MEMDUMP");
    if (!dir)
        return;
    char trig[1024], path[1024];
    snprintf(trig, sizeof trig, "%s/dump.now", dir);
    FILE *t = fopen(trig, "r");
    if (!t)
        return;
    fclose(t);
    remove(trig);
    snprintf(path, sizeof path, "%s/mem_%d.bin", dir, n++);
    FILE *f = fopen(path, "wb");
    if (f) {
        fwrite((void *)XBOX_PTR(0), 1, 64u << 20, f);
        fclose(f);
        fprintf(stderr, "[CHEAT] memory dumped to %s\n", path);
    }
}

/* NGB_POKE=addr:bytes:value,...: debugging, written every frame (hex). */
static void poke_tick(void)
{
    static const char *e = (const char *)1;
    if (e == (const char *)1)
        e = getenv("NGB_POKE");
    if (!e)
        return;
    static const char *gate = (const char *)1;   /* NGB_POKE_GATE=<file>: only while it exists */
    if (gate == (const char *)1)
        gate = getenv("NGB_POKE_GATE");
    if (gate) {
        FILE *g = fopen(gate, "r");
        if (!g)
            return;
        fclose(g);
    }
    const char *p = e;
    while (*p) {
        unsigned a, n, v;
        int used = 0;
        if (sscanf(p, "%x:%x:%x%n", &a, &n, &v, &used) != 3)
            break;
        if (n == 1) MEM8(a) = (uint8_t)v;
        else if (n == 2) MEM16(a) = (uint16_t)v;
        else MEM32(a) = v;
        p += used;
        if (*p == ',') p++;
    }
}

static int s_cheat[4];   /* infinite health, infinite ninpo, infinite essence, golden scarabs */

int ngb_cheat(int which) { return which >= 0 && which < 4 ? s_cheat[which] : 0; }
void ngb_cheat_set(int which, int on)
{
    static const char *const name[4] = { "infinite health", "infinite ninpo", "infinite essence", "golden scarabs" };
    if (which < 0 || which >= 4)
        return;
    s_cheat[which] = !!on;
    fprintf(stderr, "[CHEAT] %s %s\n", name[which], on ? "on" : "off");
}

/* A cheat changed the game's state: its checksums taken anew, not compared (see above). */
static void cheat_wrote(void)
{
    for (uint32_t a = 0x7F1A94; a < 0x7F1A94 + 24; a++)
        MEM8(a) = 1;
}

/* Golden Scarabs: enough held that, with those already brought to Muramasa (0x589574, at most 50),
 * they make 50 -- each of his rewards at the next visit. The inventory: 0x80 slots of 4 bytes at
 * 0x5891D0 (item, count, -; a count of 0 or item 0xFFFF an empty slot); the scarab is item 0x59.
 * In play only (Ryu has health). */
static void scarab_tick(void)
{
    uint32_t brought = MEM32(0x589574);
    if (brought >= 50 || !MEM16(0x58A564))
        return;
    uint32_t want = 50 - brought, slot = 0, empty = 0;
    for (uint32_t a = 0x5891D0; a < 0x5891D0 + 4 * 0x80; a += 4) {
        if (MEM16(a) == 0x59 && MEM8(a + 2))
            slot = a;
        else if (!empty && (!MEM8(a + 2) || MEM16(a) == 0xFFFF))
            empty = a;
    }
    if (slot && MEM8(slot + 2) >= want)
        return;
    if (!slot && !(slot = empty))
        return;                                        /* the bag is full */
    MEM16(slot) = 0x59;
    MEM8(slot + 2) = (uint8_t)want;
    cheat_wrote();
    if (getenv("NGB_CHEAT_LOG"))
        fprintf(stderr, "[CHEAT] golden scarabs: %u held (%u brought)\n", want, brought);
}

/* Ryu's health filled (Infinite Health): the segments (0x58A534 + 2i, at most 0x58A524 + 2i; 0xFFFF a
 * segment not had) -- damage lowers them -- and 0x58A566, their sum, which the game makes again
 * from them. True when anything was filled. Each frame: a blow taking all of it at once still kills
 * (the game sees 0 before the next fill). */
static int health_fill(void)
{
    uint16_t most = MEM16(0x58A564);
    if (!s_cheat[0] || !most || most >= 0x8000)
        return 0;
    int filled = 0;
    for (int i = 0; i < 8; i++) {
        uint16_t m = MEM16(0x58A524 + 2 * i), c = MEM16(0x58A534 + 2 * i);
        if (m != 0xFFFF && m && c != m) {
            MEM16(0x58A534 + 2 * i) = m;
            filled = 1;
        }
    }
    if (MEM16(0x58A566) != most) {
        MEM16(0x58A566) = most;
        filled = 1;
    }
    if (filled)
        cheat_wrote();
    return filled;
}

static void cheat_tick(void)
{
    static int env_read;
    if (!env_read) {                                   /* NGB_CHEATS=111: debugging, on at start */
        const char *e = getenv("NGB_CHEATS");
        for (int i = 0; e && i < 4 && e[i]; i++)
            s_cheat[i] = e[i] == '1';
        env_read = 1;
    }
    if (health_fill()) {                               /* Infinite Health */
        static int told;
        if (!told++ && getenv("NGB_CHEAT_LOG"))
            fprintf(stderr, "[CHEAT] health filled\n");
    }
    uint16_t most = MEM16(0x58A56C);
    if (s_cheat[1] && most && most < 0x8000 && MEM16(0x58A56E) < most) {
        MEM16(0x58A56E) = most;
        cheat_wrote();
    }
    if (s_cheat[2] && MEM32(0x589570) < 99999) {
        MEM32(0x589570) = 99999;
        cheat_wrote();
    }
    if (s_cheat[3])
        scarab_tick();
}

void ngb_aspect_tick(void);   /* src/pc_menu.c */

/* ---- debugging: where Ryu is, and moving him ----
 *
 * Ryu's position is the vector at 0x4B5710 (x y z 1; copies of it, for the frames before, at
 * 0x4B5720/30, 0x4B5F70/A0, 0x4B6730/60, 0x4B6EF0/F20). F8 appends it to ngb_positions.txt in the
 * save directory (NGB_SAVE_DIR or ~/.local/share/NinjaGaidenBlack). NGB_TELEPORT=<file>: when the
 * file appears holding "x y z", Ryu is put there (every copy of his position in his block) and the
 * file removed. */
volatile int ngb_pos_request;

static void pos_tick(void)
{
    if (ngb_pos_request) {
        ngb_pos_request = 0;
        char path[1024];
        const char *dir = getenv("NGB_SAVE_DIR"), *home = getenv("HOME");
        if (dir) snprintf(path, sizeof path, "%s/ngb_positions.txt", dir);
        else snprintf(path, sizeof path, "%s/.local/share/NinjaGaidenBlack/ngb_positions.txt", home ? home : ".");
        FILE *f = fopen(path, "a");
        if (f) {
            fprintf(f, "%.3f %.3f %.3f\n", MEMF(0x4B5710), MEMF(0x4B5714), MEMF(0x4B5718));
            fclose(f);
        }
        fprintf(stderr, "[POS] %.3f %.3f %.3f -> %s\n", MEMF(0x4B5710), MEMF(0x4B5714), MEMF(0x4B5718), path);
    }
    static const char *tp = (const char *)1;
    if (tp == (const char *)1)
        tp = getenv("NGB_TELEPORT");
    if (!tp)
        return;
    FILE *f = fopen(tp, "r");
    if (!f)
        return;
    float x, y, z;
    int ok = fscanf(f, "%f %f %f", &x, &y, &z) == 3;
    fclose(f);
    remove(tp);
    if (!ok)
        return;
    float ox = MEMF(0x4B5710), oy = MEMF(0x4B5714), oz = MEMF(0x4B5718);
    int n = 0;
    for (uint32_t a = 0x4B5000; a < 0x4B8000; a += 16)
        if (MEMF(a) == ox && MEMF(a + 4) == oy && MEMF(a + 8) == oz) {
            MEMF(a) = x; MEMF(a + 4) = y; MEMF(a + 8) = z;
            n++;
        }
    fprintf(stderr, "[POS] teleported from %.3f %.3f %.3f to %.3f %.3f %.3f (%d copies)\n", ox, oy, oz, x, y, z, n);
}

void ngb_aspect_frame_end(void);   /* src/pc_menu.c */
void ngb_autosave_tick(void);       /* src/autosave.c */

void ngb_frame_tick(void)
{
    pos_tick();
    ngb_aspect_frame_end();
    ngb_aspect_tick();
    cheat_tick();
    memdump_tick();
    poke_tick();
    ngb_autosave_tick();
}

/* ---- every difficulty from the start ----
 *
 * The game's flags for its menus (sub_001DB520, ecx the flag, eax 1 when
 * set): 6 Normal (always), 7-9 Hard, Very Hard and Master Ninja (bytes
 * 0xDB9472-0xDB9474 of the system data: each difficulty cleared unlocks the
 * next), 5 Ninja Dog (never here: the game offers it after deaths in chapter
 * 1), 1-4 the costumes. NEW GAME shows the difficulty selection when more
 * than Normal is set. With the setting on Ninja Dog through Master Ninja read
 * as set -- nothing is written,
 * so the player's own record (system.dat) is untouched and turning it off
 * puts back what they have earned. */
static int s_all_difficulties, s_all_costumes;
void ngb_set_all_difficulties(int on) { s_all_difficulties = !!on; }
/* Every costume (flags 1-4: the game's Costume Selection at New Game, as after clearing each
 * difficulty), the same way -- and the three it has but never offers (src/costume_menu.c). */
void ngb_set_all_costumes(int on) { s_all_costumes = !!on; }

/**
 * sub_001DB520
 * Original: 0x001DB520 - 0x001DB596 (118 bytes, 33 insns)
 * Category: game_vtable
 * CC: thiscall, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
static void game_flag(void)
{
    RECOMP_ENTRY(0x001DB520u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_001DB520: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xE) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xE (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_001DB595; /* jge: greater or equal (signed >=) */

loc_001DB527: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xD) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xD (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_001DB595; /* ja: above (unsigned >) */

loc_001DB52C: ;
    { uint32_t _jt = MEM32(ecx * 4 + 0x1DB598); /* switch: 14 entries, 13 targets */
    if (_jt == 0x001DB533u) goto loc_001DB533;
    if (_jt == 0x001DB53Bu) goto loc_001DB53B;
    if (_jt == 0x001DB543u) goto loc_001DB543;
    if (_jt == 0x001DB54Bu) goto loc_001DB54B;
    if (_jt == 0x001DB553u) goto loc_001DB553;
    if (_jt == 0x001DB556u) goto loc_001DB556;
    if (_jt == 0x001DB55Eu) goto loc_001DB55E;
    if (_jt == 0x001DB566u) goto loc_001DB566;
    if (_jt == 0x001DB56Eu) goto loc_001DB56E;
    if (_jt == 0x001DB576u) goto loc_001DB576;
    if (_jt == 0x001DB57Eu) goto loc_001DB57E;
    if (_jt == 0x001DB586u) goto loc_001DB586;
    if (_jt == 0x001DB590u) goto loc_001DB590;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_001DB533: ;
    SET_LO8(ecx, MEM8(0xDB941C));
    goto loc_001DB58C;

loc_001DB53B: ;
    SET_LO8(ecx, MEM8(0xDB941D));
    goto loc_001DB58C;

loc_001DB543: ;
    SET_LO8(ecx, MEM8(0xDB941E));
    goto loc_001DB58C;

loc_001DB54B: ;
    SET_LO8(ecx, MEM8(0xDB941F));
    goto loc_001DB58C;

loc_001DB553: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    esp += 4; return; /* ret */

loc_001DB556: ;
    SET_LO8(ecx, MEM8(0xDB9472));
    goto loc_001DB58C;

loc_001DB55E: ;
    SET_LO8(ecx, MEM8(0xDB9473));
    goto loc_001DB58C;

loc_001DB566: ;
    SET_LO8(ecx, MEM8(0xDB9474));
    goto loc_001DB58C;

loc_001DB56E: ;
    SET_LO8(ecx, MEM8(0xDB956A));
    goto loc_001DB58C;

loc_001DB576: ;
    SET_LO8(ecx, MEM8(0xDB9417));
    goto loc_001DB58C;

loc_001DB57E: ;
    SET_LO8(ecx, MEM8(0xDB9416));
    goto loc_001DB58C;

loc_001DB586: ;
    SET_LO8(ecx, MEM8(0xDB9418));

loc_001DB58C: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001DB595; /* je: equal / zero */

loc_001DB590: ;
    eax = 1;

loc_001DB595: ;
    esp += 4; return; /* ret */

}

void sub_001DB520(void)
{
    if (ENV_ONCE("NGB_FLAG_LOG"))
        fprintf(stderr, "[FLAG] %u (all difficulties %d)\n", ecx, s_all_difficulties);
    if ((s_all_difficulties && ecx >= 5 && ecx <= 9) || (s_all_costumes && ecx >= 1 && ecx <= 4)) {
        eax = 1;
        esp += 4;   /* ret */
        return;
    }
    game_flag();
}
