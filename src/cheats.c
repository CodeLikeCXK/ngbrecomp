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
 * file <dir>/dump.now exists, the guest's 64 MB go to <dir>/mem_<n>.bin and
 * the contiguous window (0x80000000, 128 MB: the item descriptors, GPU
 * memory) to <dir>/contig_<n>.bin (the file is removed after).
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
    snprintf(path, sizeof path, "%s/contig_%d.bin", dir, n - 1);
    if ((f = fopen(path, "wb"))) {
        fwrite((void *)XBOX_PTR(0x80000000u), 1, 128u << 20, f);
        fclose(f);
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

#define N_CHEATS 6
/* infinite health, infinite ninpo, infinite essence, golden scarabs, infinite items, infinite projectiles */
static int s_cheat[N_CHEATS];

int ngb_cheat(int which) { return which >= 0 && which < N_CHEATS ? s_cheat[which] : 0; }
void ngb_cheat_set(int which, int on)
{
    static const char *const name[N_CHEATS] = { "infinite health", "infinite ninpo", "infinite essence", "golden scarabs",
                                                "infinite items", "infinite projectiles" };
    if (which < 0 || which >= N_CHEATS)
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

/* Infinite Items and Infinite Projectiles: the game takes an item away through one function,
 * sub_00154300 (eax the item, ebx the change, signed -- all 32 bits are read for its sign; a count
 * reaching 0 empties the slot) -- using a restorative from the inventory or by the game itself (the Talisman of Rebirth),
 * throwing or shooting. With the cheat on, a removal of such an item is made as a change of 0, so
 * the game's own path runs and its caller sees it done. What an item is: its descriptor (the table
 * at [0xDB98E4], [0xDB5E24] of them): +0x0C 1 or 3 ammunition (shuriken, incendiary shuriken, smoke
 * bombs; arrows, explosive arrows, spears), +0x0D its class (5 an item), +0x1D its effect (0x12
 * health, 0x13 ninpo, 0x16 the Jewel of the Demon Seal (a Ninpo upgraded, #52), 0x22 the Talisman of
 * Rebirth; not the jewels that raise the limits for good -- 0x14 Life of the Gods, 0x17 Lives of the
 * Thousand Gods (health), 0x15 Spirit of the Devils (Ki): Infinite Health and Ninpo are the cheats for those). */
static uint32_t item_record(uint16_t item)
{
    uint32_t base = MEM32(0xDB98E4), count = MEM32(0xDB5E24);
    if (!base || count > 1024 || item >= count)
        return 0;
    uint32_t o = MEM32(base + item * 4u);
    return o && o < 0x800000 - 0x40 ? base + o : 0;
}

static int item_kept(uint16_t item)
{
    uint32_t r = item_record(item);
    if (!r)
        return 0;
    uint8_t ammo = MEM8(r + 0x0C), cls = MEM8(r + 0x0D), effect = MEM8(r + 0x1D);
    if (s_cheat[5] && (ammo == 1 || ammo == 3))
        return 1;
    return s_cheat[4] && cls == 5 &&
           (effect == 0x12 || effect == 0x13 || effect == 0x16 || effect == 0x22);
}

static void item_count_change(void);

void sub_00154300(void)
{
    if (ENV_ONCE("NGB_ITEM_LOG"))                     /* NGB_ITEM_LOG=1: every change of an item's count */
        fprintf(stderr, "[ITEM] %03X %+d\n", (unsigned)(eax & 0xFFFF), (int)ebx);
    if ((s_cheat[4] || s_cheat[5]) && (int32_t)ebx < 0 && item_kept((uint16_t)eax)) {
        uint32_t keep = ebx;
        if (getenv("NGB_CHEAT_LOG"))
            fprintf(stderr, "[CHEAT] item %03X kept (%d)\n", (unsigned)(eax & 0xFFFF), (int)ebx);
        ebx = 0;
        item_count_change();
        ebx = keep;
        return;
    }
    item_count_change();
}

/* The game's sub_00154300 (generated, src/manual_functions.json) */
static void item_count_change(void)
{
    RECOMP_ENTRY(0x00154300u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_00154300: ;
    PUSH32(esp, ecx);
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    edx = edx | 0xFFFFFFFFu;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(esp + 8) = 0;
    esi = 0; /* xor self */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ecx = 0x5891D0;

loc_00154315: ;
    _fa = (uint32_t)(MEM8(ecx + 2)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(ecx + 2), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00154320; /* je: equal / zero */

loc_0015431B: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(MEM16(ecx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), MEM16(ecx) (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0015432E; /* je: equal / zero */

loc_00154320: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x5893D0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x5893D0 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_00154315; /* jl: less (signed <) */

loc_0015432C: ;
    goto loc_00154330;

loc_0015432E: ;
    edx = esi;

loc_00154330: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0 (32-bit) */
    ebp = edx;
    if (CMP_GE(_fas, _fbs)) goto loc_0015437E; /* jge: greater or equal (signed >=) */

loc_00154336: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0xFFFFFFFFu (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0015442B; /* je: equal / zero */

loc_0015433F: ;
    edx = edx << 2;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    SET_LO8(ecx, MEM8(edx + 0x5891D2));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0xFF (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00154414; /* je: equal / zero */

loc_00154351: ;
    eax = ZX8(LO8(ecx));
    esi = ebx;
    esi = (uint32_t)(-(int32_t)esi);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, eax (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_00154369; /* jge: greater or equal (signed >=) */

loc_0015435C: ;
    _fb = (uint32_t)(LO8(ebx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    SET_LO8(ecx, LO8(ecx) + LO8(ebx));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    MEM8(edx + 0x5891D2) = LO8(ecx);
    goto loc_00154414;

loc_00154369: ;
    MEM8(edx + 0x5891D2) = 0;
    MEM16(edx + 0x5891D0) = 0xFFFF;
    goto loc_00154414;

loc_0015437E: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0xFFFFFFFFu (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001543C9; /* jne: not equal / not zero */

loc_00154383: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ecx = 0x5891D0;
    /* nop */

loc_00154390: ;
    _fa = (uint32_t)(MEM8(ecx + 2)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(ecx + 2), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001543AF; /* je: equal / zero */

loc_00154396: ;
    _fa = (uint32_t)(MEM16(ecx)) & 0xFFFFu; _fb = (uint32_t)(0xFFFF) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(ecx), 0xFFFF (16-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_001543AF; /* jae: above or equal (unsigned >=) */

loc_0015439D: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x5893D0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x5893D0 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_00154390; /* jl: less (signed <) */

loc_001543A9: ;
    POP32(esp, esi);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    POP32(esp, ebp);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

loc_001543AF: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x80) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0x80 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_001543A9; /* jge: greater or equal (signed >=) */

loc_001543B7: ;
    ebp = edx;
    MEM8(edx * 4 + 0x5891D2) = 0;
    MEM16(edx * 4 + 0x5891D0) = LO16(eax);

loc_001543C9: ;
    edx = ebp * 4;
    SET_LO8(ecx, MEM8(edx + 0x5891D2));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0xFF (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00154414; /* je: equal / zero */

loc_001543DB: ;
    esi = MEM32(0xDB5E24);
    eax = ZX16(LO16(eax));
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, eax (32-bit) */
    PUSH32(esp, edi);
    if (CMP_LE(_fas, _fbs)) goto loc_001543FA; /* jle: less or equal (signed <=) */

loc_001543E9: ;
    esi = MEM32(0xDB98E4);
    edi = MEM32(esi + eax * 4);
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001543FA; /* je: equal / zero */

loc_001543F6: ;
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + edi;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    if ((_fa != 0)) goto loc_001543FE; /* jne: not equal / not zero */

loc_001543FA: ;
    SET_LO8(eax, 0); /* xor self */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    goto loc_00154401;

loc_001543FE: ;
    SET_LO8(eax, MEM8(esi + 0x24));

loc_00154401: ;
    _fb = (uint32_t)(LO8(ebx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    SET_LO8(ecx, LO8(ecx) + LO8(ebx));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ecx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ecx) (8-bit) */
    MEM8(edx + 0x5891D2) = LO8(ecx);
    POP32(esp, edi);
    if (CMP_AE(_fa, _fb)) goto loc_00154414; /* jae: above or equal (unsigned >=) */

loc_0015440E: ;
    MEM8(edx + 0x5891D2) = LO8(eax);

loc_00154414: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0xFFFFFFFFu (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0015442B; /* je: equal / zero */

loc_00154419: ;
    SET_LO8(eax, MEM8(edx + 0x5891D2));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0015442B; /* je: equal / zero */

loc_00154423: ;
    MEM32(esp + 8) = 1;

loc_0015442B: ;
    eax = MEM32(esp + 8);
    POP32(esp, esi);
    MEM8(0x7F1A94) = 1;
    POP32(esp, ebp);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

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
        for (int i = 0; e && i < N_CHEATS && e[i]; i++)
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
    { extern void ngb_camera_frame(void); ngb_camera_frame(); }   /* src/camera.c: which view the stick turns */
    { extern void ngb_quick_tick(void); ngb_quick_tick(); }
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
