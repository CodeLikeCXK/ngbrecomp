/* D-pad quick selection. Gameplay inventory changes run on the guest thread
 * through Black's own equip, effect, and inventory-debit routines. */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include "quick_select.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void native_pause_check(void);
int ngb_pc_setting(int which);
int ngb_movie_active(void);
extern uint64_t SDL_GetTicks64(void);
static int s_in_quick;
static int s_pause;
static uint16_t s_last_heal = 0xFFFF;
static unsigned s_recovery_sound;
static uint64_t s_recovery_due;

typedef struct
{
    uint32_t r[9];
    RecompXmm x[8];
    double fp[8];
    int top;
} GuestContext;
static GuestContext save_context(void)
{
    GuestContext c = {{g_eax, g_ecx, g_edx, g_esp, g_ebx, g_esi, g_edi, g_ebp, g_seh_ebp},
                      {g_xmm0, g_xmm1, g_xmm2, g_xmm3, g_xmm4, g_xmm5, g_xmm6, g_xmm7},
                      {0},
                      g_fp_top};
    memcpy(c.fp, g_fp_stack, sizeof c.fp);
    return c;
}
static void restore_context(const GuestContext *c)
{
    g_eax = c->r[0];
    g_ecx = c->r[1];
    g_edx = c->r[2];
    g_esp = c->r[3];
    g_ebx = c->r[4];
    g_esi = c->r[5];
    g_edi = c->r[6];
    g_ebp = c->r[7];
    g_seh_ebp = c->r[8];
    g_xmm0 = c->x[0];
    g_xmm1 = c->x[1];
    g_xmm2 = c->x[2];
    g_xmm3 = c->x[3];
    g_xmm4 = c->x[4];
    g_xmm5 = c->x[5];
    g_xmm6 = c->x[6];
    g_xmm7 = c->x[7];
    memcpy(g_fp_stack, c->fp, sizeof c->fp);
    g_fp_top = c->top;
}
static uint32_t call(void (*fn)(void))
{
    PUSH32(esp, 0x001669EEu);
    fn();
    return eax;
}
static void play_sound(unsigned sound, const char *event)
{
    PUSH32(esp, 0x40);
    PUSH32(esp, sound);
    int voice = (int)call(sub_001E3400);
    esp += 8;
    fprintf(stderr, "[QUICK] %s sound=%X voice=%d ms=%llu\n", event, sound, voice,
            (unsigned long long)SDL_GetTicks64());
}
static void finish_recovery(void)
{
    unsigned sound = s_recovery_sound;
    s_recovery_sound = 0;
    if (sound) play_sound(sound, "recovery");
}
static uint32_t record(uint16_t item)
{
    uint32_t base = MEM32(0xDB98E4), count = MEM32(0xDB5E24);
    /* Descriptor storage can live in the runtime's high contiguous-memory
     * arena. Do not mistake its Xbox alias for an out-of-range RAM pointer. */
    if (!base || count > 1024 || item >= count)
        return 0;
    uint32_t o = MEM32(base + item * 4u);
    return o && o < 0x800000 - 0x40 && base + o >= base ? base + o : 0;
}
static int supported_layout(void)
{
    /* These task lists belong to the supported retail game, rather than
     * the host executable's addresses. A different game layout must never
     * receive our inventory writes or pause-counter changes. The installer's
     * full XBE section check remains the primary game-version check. */
    return MEM32(0x428210) == 4 && MEM32(0x428214) == 0x320E0 &&
           MEM32(0x428218) == 6 && MEM32(0x428248) == 4 &&
           MEM32(0x428250) == 5 && record(1) && MEM8(record(1) + 0xD) == 1;
}
static unsigned stock(uint16_t item)
{
    for (uint32_t p = 0x5891D0; p < 0x5893D0; p += 4)
        if (MEM16(p) == item)
            return MEM8(p + 2);
    return 0;
}
static uint32_t refill_effect(uint16_t item, int *kind)
{
    uint32_t r = record(item), base = MEM32(0xDB98E4);
    if (!r || !MEM8(r + 0x1D))
        return 0;
    uint32_t o = MEM32(r + 0x10);
    if (!o || o > 0x800000 - 32)
        return 0;
    int found = 0;
    for (unsigned i = 0; i < 4; i++)
    {
        uint32_t effect = MEM32(base + o + i * 8);
        if (!effect)
            break;
        if (effect != 0x12 && effect != 0x13)
            return 0;
        if (found && found != (int)effect)
            return 0;
        found = (int)effect;
    }
    if (kind)
        *kind = found;
    return found ? base + o : 0;
}
static int regular_play(void)
{
    return !ngb_movie_active() && (!MEM8(0xDB9AD5) || s_pause) && MEM8(0x7F1B12) == 1 && !MEM8(0x7F1B11) &&
           (MEM8(0xDB9894) == 2 || MEM8(0xDB9894) == 3) && MEM8(0x4B7640) < 0x20 &&
           MEM16(0x58A566 + MEM8(0x4B7640) * 0x210u) != 0;
}
static int can_pause(void) { return regular_play() && (s_pause || (call(sub_00166BD0) & 255)); }
static void pause_menu(int pause)
{
    if (pause == s_pause) return;
    /* The main game loop observes this counter and suspends its gameplay
     * task group. Rendering, input polling and music continue on their usual
     * schedule. Never wait inside the gameplay task to hold the picture. */
    if (pause) MEM8(0xDB5A68)++;
    else
    {
        /* Native input histories keep action transitions as well as the
         * current pad state. Flush them as the ordinary pause exit does. */
        GuestContext c = save_context();
        for (unsigned i = 0; i < 5; i++)
        {
            MEM32(0xD219A0 + i * 0x78u) = 0;
            MEM32(0xD219A8 + i * 0x78u) = 0;
            MEM8(0xD21A16 + i * 0x78u) = 1;
        }
        call(sub_00032460);
        restore_context(&c);
        if (MEM8(0xDB5A68)) MEM8(0xDB5A68)--;
    }
    s_pause = pause;
    fprintf(stderr, "[QUICK] pause=%d depth=%u\n", pause, MEM8(0xDB5A68));
}
static void picture(NgbQuickEntry *e)
{
    const char *art = "", *name = "Weapon";
    unsigned lv = e->count;
    uint16_t art_id = e->id;
    /* Mission and costume variants use the same Black pictures. */
    switch (art_id)
    {
    case 0x14C:
        art_id = 1;
        break;
    case 0x14E:
        art_id = 0;
        break;
    case 0xF:
    case 0x9C:
    case 0x16D:
    case 0x16E:
    case 0x16F:
        art_id = 0x9B;
        break;
    case 0x9F:
    case 0xA0:
        art_id = 0x9E;
        break;
    case 0xAD:
    case 0xAE:
        art_id = 0xAC;
        break;
    case 0x160:
        art_id = 0x9A;
        break;
    case 0x14F:
        art_id = 0x26;
        break;
    case 0x150:
    case 0x161:
        art_id = 0x44;
        break;
    case 0x151:
    case 0x162:
    case 0x163:
        art_id = 0x95;
        break;
    case 0x152:
        art_id = 0xC9;
        break;
    case 0x153:
    case 0x164:
        art_id = 0xCA;
        break;
    case 0x154:
        art_id = 0xCB;
        break;
    case 0x155:
        art_id = 0xCC;
        break;
    case 0x156:
        art_id = 0xCD;
        break;
    case 0x157:
        art_id = 0xCE;
        break;
    case 0x159:
        art_id = 0xE1;
        break;
    case 0x15C:
        art_id = 0xF7;
        break;
    case 0x15D:
    case 0x15E:
    case 0x15F:
        art_id = 0x12B;
        break;
    case 0x170:
        art_id = 0x98;
        break;
    default:
        break;
    }
    uint32_t rec = record(e->id);
    if (rec)
        lv += MEM8(rec + 0x1E);
    if (rec && (MEM8(rec + 0xD) == 1 || MEM8(rec + 0xD) == 3) && art_id != 0x98)
        e->level = (uint8_t)lv;
    switch (art_id)
    {
    case 1:
        art = lv >= 3 ? "kata3" : lv == 2 ? "kata2" : "katana";
        name = "Dragon Sword";
        break;
    case 0xC9:
        art = "skata";
        name = "True Dragon Sword";
        break;
    case 0x26:
        art = "nchak";
        name = "Nunchaku";
        break;
    case 0x12B:
    case 0x133:
    case 0x134:
        art = lv >= 3 ? "rod3" : lv == 2 ? "rod2" : "rod";
        name = "Lunar";
        break;
    case 0x44:
        art = lv >= 2 ? "ham2" : "hammer";
        name = "War Hammer";
        break;
    case 0x95:
        art = lv >= 3 ? "zan3" : lv == 2 ? "zan2" : "zanba";
        name = "Dabilahro";
        break;
    case 0xCA:
        art = lv >= 2 ? "fl2" : "flail";
        name = "Vigoorian Flail";
        break;
    case 0xCB:
        art = "boku";
        name = "Wooden Sword";
        break;
    case 0xE1:
        art = "bokux";
        name = "Unlabored Flawlessness";
        break;
    case 0xCC:
    case 0x182:
    case 0x183:
        art = "yoto";                 /* every level: the game's yoto2.spr is its "No Data" picture */
        name = "Kitetsu";
        break;
    case 0xCD:
    case 0xFA:
        art = "koku";
        name = "Dark Dragon Blade";
        break;
    case 0xCE:
        art = lv >= 3 ? "sabe3" : lv == 2 ? "sabe2" : "saber";
        name = "Plasma Saber";
        break;
    case 0xF7:
        art = "ssabe";
        name = "Plasma Saber Mk.II";
        break;
    case 0:
        art = "syuri";
        name = "Shuriken";
        break;
    case 8:
        art = "ex_syuri";
        name = "Incendiary Shuriken";
        break;
    case 0x30:
        art = "fu_syuri";
        name = "Windmill Shuriken";
        break;
    case 0x165:                       /* its own picture, not the incendiary shuriken's */
        art = "smokebom";
        name = "Smoke Bomb";
        break;
    case 0x27:
    case 0x29:
        art = "bow01";
        name = "Bow";
        break;
    case 0x8D:
        art = "bow02";
        name = "Bow";
        break;
    case 0xE2:
        art = "bow03";
        name = "Bow";
        break;
    case 0xF8:
        art = "bow04";
        name = "Strongbow";
        break;
    case 0x98:
        art = "sgun";
        name = "Spear Gun";
        break;
    case 0x9A:
        art = "scr_4";
        name = "The Art of the Fire Wheels";
        break;
    case 0x9B:
        art = lv >= 3 ? "scr_1_3" : lv == 2 ? "scr_1_2" : "scr_1";
        name = "The Art of the Inferno";
        break;
    case 0x9E:
        art = lv >= 3 ? "scr_2_3" : lv == 2 ? "scr_2_2" : "scr_2";
        name = "The Art of the Ice Storm";
        break;
    case 0xAC:
        art = lv >= 3 ? "scr_3_3" : lv == 2 ? "scr_3_2" : "scr_3";
        name = "The Art of the Inazuma";
        break;
    case 0xBE:
        art = "re_hp_s";
        name = "Elixir of Spiritual Life";
        break;
    case 0xBF:
        art = "re_hp_l";
        name = "Great Spirit Elixir";
        break;
    case 0xC0:
        art = "re_mp_s";
        name = "Elixir of the Devil Way";
        break;
    case 0xC1:
        art = "re_mp_l";
        name = "Great Devil Elixir";
        break;
    case 0x181:
        art = "re_mp_ex";
        name = "Fragrance of Dayflower";
        break;
    case 0x17E:
        art = "re_mp_ex_s";
        name = "Fragrance of Hydrangea";
        break;
    default:
        break;
    }
    if (*art)
        snprintf(e->art, sizeof e->art, "%s.spr", art);
    snprintf(e->name, sizeof e->name, "%s", name);
}
static int entry_category(uint16_t item)
{
    uint32_t r = record(item);
    if (!r) return 0;
    /* Keep the spear gun with Black's melee weapons, including its mission
     * variant. The loader still uses each descriptor's native equip slot. */
    if (item == 0x98 || item == 0x170) return NGB_QUICK_MELEE;
    unsigned cls = MEM8(r + 0xD);
    if (cls >= 1 && cls <= 3) return (int)cls;
    return refill_effect(item, NULL) ? NGB_QUICK_ITEMS : 0;
}
static void make_view(NgbQuickView *v, int category)
{
    memset(v, 0, sizeof *v);
    v->category = category;
    v->freeze = ngb_pc_setting(19);
    uint32_t player = MEM8(0x4B7640) * 0x210u;
    for (uint32_t p = 0x5891D0; p < 0x5893D0 && v->count < NGB_QUICK_CAP; p += 4)
    {
        uint16_t item = MEM16(p);
        unsigned count = MEM8(p + 2);
        uint32_t r = record(item);
        int kind = 0;
        if (!count || item == 0xFFFF || !r)
            continue;
        if (entry_category(item) != category)
            continue;
        if (category == NGB_QUICK_ITEMS) refill_effect(item, &kind);
        int duplicate = 0;
        for (int i = 0; i < v->count; i++)
            if (v->entries[i].id == item)
                duplicate = 1;
        if (duplicate)
            continue;
        NgbQuickEntry *e = &v->entries[v->count];
        e->id = item;
        e->count = (uint8_t)count;
        e->usable = category != NGB_QUICK_ITEMS || (kind == 0x12 ? MEM16(0x58A566 + player) < MEM16(0x58A564 + player)
                                                   : MEM16(0x58A56Eu + player) < MEM16(0x58A56C + player));
        picture(e);
        /* Unsupported artwork cannot enter an invisible carousel cell. */
        if (!e->art[0])
            continue;
        e->quantity = category == NGB_QUICK_ITEMS;
        if (category == NGB_QUICK_PROJECTILES)
        {
            /* A bow's inventory count is ownership, not its ammunition.
             * Its descriptor names the arrows required by that bow variant. */
            uint32_t components = MEM32(r + 4), base = MEM32(0xDB98E4);
            if (components && components < 0x800000 - 4 && MEM16(base + components) != 0xFFFF)
            {
                e->count = (uint8_t)stock(MEM16(base + components));
                e->quantity = 1;
            }
            else if (item == 8 || item == 0x165)
                e->quantity = 1;
        }
        unsigned cls = MEM8(r + 0xD);
        if ((category != NGB_QUICK_ITEMS && item == MEM16(0x58A570 + player + (cls - 1) * 2)) ||
            (category == NGB_QUICK_ITEMS && item == s_last_heal))
            v->index = v->count;
        v->count++;
    }
}
static void make_menu(NgbQuickMenu *menu, int category, int ready)
{
    memset(menu, 0, sizeof *menu);
    menu->category = category;
    menu->ready = ready;
    for (int i = 0; i < NGB_QUICK_CATEGORIES; i++) make_view(&menu->views[i], i + 1);
}
static void apply(uint16_t item, int category)
{
    if (!can_pause() || !stock(item))
    {
        fprintf(stderr, "[QUICK] refused stale selection %04X\n", item);
        return;
    }
    uint32_t r = record(item), player = MEM8(0x4B7640) * 0x210u;
    if (!r)
        return;
    if (entry_category(item) != category)
    {
        fprintf(stderr, "[QUICK] refused %04X: not in category %d\n", item, category);
        return;
    }
    if (category != NGB_QUICK_ITEMS)
    {
        unsigned cls = MEM8(r + 0xD);
        if (cls < 1 || cls > 3)
            return;
        /* The inventory handler checks ownership of the weapon before
         * passing it to the resource loader. */
        esi = item;
        if (!call(sub_001542C0))
        {
            fprintf(stderr, "[QUICK] refused %04X: the game does not count it as owned\n", item);
            return;
        }
        esp -= 64;
        uint32_t scratch = esp;
        ecx = item;
        eax = scratch;
        unsigned n = call(sub_001550F0);
        if (n > 16)
        {
            fprintf(stderr, "[QUICK] refused %04X: %u required parts\n", item, n);
            return;
        }
        /* A part the game does not count as owned does not stop it: its own Weapon menu equips such a
         * weapon (the Kitetsu of a chapter 13 save, part 0x0EA; the Spear Gun without spears, 0x099). */
        for (unsigned i = 0; i < n; i++)
        {
            esi = MEM16(scratch + i * 2);
            if (!call(sub_001542C0))
                fprintf(stderr, "[QUICK] %04X: its part %04X is not owned\n", item, (unsigned)MEM16(scratch + i * 2));
        }
        if (MEM16(0x58A570 + player + (cls - 1) * 2) == item)
        {
            play_sound(5, "confirmation");
            return;
        }
        /* Weapon assets share memory with the world. Use Black's complete
         * inventory transition, including the world/player restoration on
         * exit, even when selection itself is configured to remain live. */
        s_in_quick = 1;
        call(sub_002FD820);
        eax = cls - 1;
        PUSH32(esp, item);
        unsigned accepted = call(sub_002EAA20) & 255;
        esp += 4;
        PUSH32(esp, 1);
        call(sub_00166B90);
        esp += 4;
        s_in_quick = 0;
        fprintf(stderr, "[QUICK] equipped accepted=%u slot=%u item=%04X actual=%04X\n", accepted, cls - 1, item,
                MEM16(0x58A570 + player + (cls - 1) * 2));
        /* The native resource transition pauses active voices. Confirm only
         * after it has restored the world and resumed audio. */
        if (accepted && MEM16(0x58A570 + player + (cls - 1) * 2) == item)
            play_sound(5, "confirmation");
    }
    else
    {
        int kind = 0;
        uint32_t effect = refill_effect(item, &kind);
        if (!effect)
            return;
        edi = effect;
        if (!call(sub_00183EC0))
            return;
        unsigned before = stock(item);
        uint16_t hp = MEM16(0x58A566 + player), mp = MEM16(0x58A56Eu + player);
        esp -= 64;
        uint32_t state = esp;
        memset((void *)XBOX_PTR(state), 0, 48);
        ebx = effect;
        PUSH32(esp, state);
        unsigned result = call(sub_00183C60);
        esp += 4;
        if (result == 0x80 && (hp != MEM16(0x58A566 + player) || mp != MEM16(0x58A56Eu + player)))
        {
            uint16_t old_item = MEM16(0xDB5E20), old_count = MEM16(0xDB5E22);
            MEM16(0xDB5E20) = item;
            MEM16(0xDB5E22) = 1;
            call(sub_00155D10);
            MEM16(0xDB5E20) = old_item;
            MEM16(0xDB5E22) = old_count;
            s_last_heal = item;
            fprintf(stderr, "[QUICK] used %04X stock=%u->%u health=%u->%u ninpo=%u->%u\n", item, before,
                    stock(item), hp, MEM16(0x58A566 + player), mp, MEM16(0x58A56Eu + player));
            play_sound(5, "confirmation");
            /* Give confirmation a short lead before the inventory gauge's
             * recovery effect. Service it on later guest frames; never wait
             * here or delay the item effect, gameplay or music. */
            s_recovery_sound = kind == 0x12 ? 0xA1 : 0xA6;
            s_recovery_due = SDL_GetTicks64() + 100;
        }
        else
            fprintf(stderr, "[QUICK] item refused %04X result=%X stock=%u\n", item, result, stock(item));
    }
}
static void dispatch_command(uint16_t item, int category)
{
    if (category > 0)
    {
        /* A fast second command must not overwrite an earlier item's sound
         * or carry it into the weapon loader's audio pause. */
        finish_recovery();
        apply(item, category);
    }
}
static uint64_t s_closed_since;   /* the gate closed on an open menu, since (SDL ms) */
static void quick_step(void)
{
    if (!regular_play()) s_recovery_sound = 0;
    else if (s_recovery_sound && SDL_GetTicks64() >= s_recovery_due)
        finish_recovery();
    if (!ngb_pc_setting(20))
    {
        ngb_quick_close();
        ngb_quick_take_open();
        pause_menu(0);
        return;
    }
    int steps = ngb_quick_take_navigation();
    if (regular_play())
        while (steps-- > 0)
        {
            /* Native pause-menu cursor movement. Host input only queues the
             * event; sound dispatch belongs on the guest gameplay thread. */
            play_sound(1, "navigation");
        }
    uint16_t item;
    int category;
    /* The native inventory gate includes the player's current action and
     * resource transitions. Opening is independent of this gate; pausing,
     * category changes and applying a selection must wait for it. Recheck
     * queued host input here: live gameplay may have begun another action. */
    int ready = can_pause();
    if (ngb_quick_take_command(&item, &category) && ready)
    {
        ngb_quick_close();
        dispatch_command(item, category);
    }
    if (!ngb_quick_wanted()) pause_menu(0);
    if (!regular_play())
    {
        ngb_quick_close();
        pause_menu(0);
        return;
    }
    /* The game allows no menu while the gate is closed -- an attack finishing, but also states that last:
     * aiming the bow in first person, its own Start ignored there too. A request waits for the gate (it
     * lapses after a quarter second, src/quick_ui.c), and a menu the gate stays closed on for a second
     * closes: open there, it showed the cards with nothing working but Left and Right (#76). */
    int requested = ready ? ngb_quick_take_open() : 0;
    category = ngb_quick_category();
    if (category && !ready && !s_pause)
    {
        if (!s_closed_since)
            s_closed_since = SDL_GetTicks64();
        else if (SDL_GetTicks64() - s_closed_since > 1000)
        {
            fprintf(stderr, "[QUICK] closed: the game allows no menu now\n");
            ngb_quick_close();
            s_closed_since = 0;
            return;
        }
    }
    else
        s_closed_since = 0;
    if (category)
    {
        NgbQuickMenu menu;
        make_menu(&menu, category, ready);
        ngb_quick_refresh(&menu);
    }
    else
    {
        const char *enabled = getenv("NGB_QUICK_SELECT");
        if (s_in_quick || (enabled && !strcmp(enabled, "0")))
            return;
        if (!requested)
            return;
        if (!ngb_quick_available() || !supported_layout())
        {
            fprintf(stderr, "[QUICK] unavailable: unsupported renderer or game layout\n");
            return;
        }
        NgbQuickMenu menu;
        make_menu(&menu, requested, ready);
        ngb_quick_open(&menu);
        /* Keep the input history of the action already in progress. In
         * particular, clearing it here could release a charged attack.
         * Host binding filters consume menu controls on subsequent polls. */
    }
    pause_menu(ngb_quick_frozen());
}
void sub_00166960(void)
{
    GuestContext c = save_context();
    quick_step();
    restore_context(&c);
    if (s_pause) { esp += 4; return; }
    native_pause_check();
}
void ngb_quick_tick(void)
{
    /* The pause-check hook is absent in cutscenes and other game states. */
    if (!s_in_quick && (!regular_play() || !ngb_pc_setting(20)))
    {
        s_recovery_sound = 0;
        ngb_quick_close();
        pause_menu(0);
    }
    if (getenv("NGB_QUICK_PROBE"))
    {
        /* SDL's clock is host time, independent of the guest pause counter. */
        static uint64_t last;
        static unsigned frames;
        uint64_t now = SDL_GetTicks64();
        frames++;
        if (now - last >= 1000)
        {
            fprintf(stderr, "[QUICK PROBE] ms=%llu frames=%u menu=%d pause=%d depth=%u/%u gate=%u state=%u task4=%X pos=%.5f,%.5f,%.5f weapon=%04X\n",
                    (unsigned long long)now, frames, ngb_quick_wanted(), s_pause,
                    MEM8(0xDB5A68), MEM8(0xDB5A62), MEM8(0xDB9AD5), MEM8(0x7F1B12),
                    MEM32(0xD21C80), MEMF(0x4B5710), MEMF(0x4B5714), MEMF(0x4B5718),
                    MEM16(0x58A570 + MEM8(0x4B7640) * 0x210u));
            last = now;
            frames = 0;
        }
    }
}
/* The original pause check follows, kept in the generated code's style.
 * Its only change is the name, so Black retains every can-pause check. */
/**
 * sub_00166960
 * Original: 0x00166960 - 0x00166A38 (216 bytes, 57 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
static void native_pause_check(void)
{
    RECOMP_ENTRY(0x00166960u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_00166960: ;
    SET_LO8(eax, MEM8(0x7F1B11));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001669A3; /* je: equal / zero */

loc_00166969: ;
    SET_LO8(eax, MEM8(0xDB5A68));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    MEM8(0x7F1B11) = 0;
    if (CMP_LE(_fas, _fbs)) goto loc_00166980; /* jle: less or equal (signed <=) */

loc_00166979: ;
    SET_LO8(eax, LO8(eax) - 1);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fb = (_fa == 0x7Fu); /* dec result/SF/OF; CF unchanged */
    MEM8(0xDB5A68) = LO8(eax);

loc_00166980: ;
    _fa = (uint32_t)(MEM8(0xDB9C6A)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0xDB9C6A), 4 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00166999; /* jne: not equal / not zero */

loc_00166989: ;
    edx = MEM32(0x4B4B70);
    PUSH32(esp, 0x00166994u); RECOMP_ABI_CALL(0x0003FA10u, sub_0003FA10); /* call 0x0003FA10 */

loc_00166994: ;
    PUSH32(esp, 0x00166999u); RECOMP_ABI_CALL(0x00032E70u, sub_00032E70); /* call 0x00032E70 */

loc_00166999: ;
    eax = 3;
    g_seh_ebp = ebp; sub_0030C930(); return; /* tail jmp 0x0030C930 */

loc_001669A3: ;
    PUSH32(esp, ebx);
    ecx = 9;
    PUSH32(esp, 0x001669AEu); RECOMP_ABI_CALL(0x000324C0u, sub_000324C0); /* call 0x000324C0 */

loc_001669AE: ;
    ecx = ZX8(MEM8(0xDB932E));
    ebx = MEM32(0xD21980);
    edx = 1;
    edx = edx << LO8(ecx);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edx, ebx (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_00166A20; /* jne: not equal / not zero */

loc_001669C6: ;
    ebx = MEM32(0x4B75CC);
    eax = 0x4B4E50;
    PUSH32(esp, 0x001669D6u); RECOMP_ABI_CALL(0x00034A10u, sub_00034A10); /* call 0x00034A10 */

loc_001669D6: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001669EE; /* je: equal / zero */

loc_001669DA: ;
    SET_LO8(eax, MEM8(eax + 6));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x73) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x73 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001669E5; /* je: equal / zero */

loc_001669E1: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x74) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x74 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001669EE; /* jne: not equal / not zero */

loc_001669E5: ;
    MEM16(0x4B75D2) = 0;

loc_001669EE: ;
    PUSH32(esp, 0x001669F3u); RECOMP_ABI_CALL(0x00166BD0u, sub_00166BD0); /* call 0x00166BD0 */

loc_001669F3: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00166A1E; /* je: equal / zero */

loc_001669F7: ;
    SET_LO8(edx, MEM8(0xDB5A68));
    SET_LO8(edx, LO8(edx) + 1);
    _fa = (uint32_t)(LO8(edx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fb = (_fa == 0x80u); /* inc result/SF/OF; CF unchanged */
    PUSH32(esp, 0x17B740);
    eax = 3;
    MEM8(0x7F1B11) = 1;
    MEM8(0xDB5A68) = LO8(edx);
    PUSH32(esp, 0x00166A1Bu); RECOMP_ABI_CALL(0x0030C8E0u, sub_0030C8E0); /* call 0x0030C8E0 */

loc_00166A1B: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_00166A1E: ;
    POP32(esp, ebx);
    esp += 4; return; /* ret */

loc_00166A20: ;
    ecx = (uint32_t)((int32_t)ecx * (int32_t)0x78);
    ecx = MEM32(ecx + 0xD219A8);
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 0x10 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_001669EE; /* jne: not equal / not zero */

loc_00166A2E: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, ecx (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_00166A1E; /* je: equal / zero */

loc_00166A32: ;
    POP32(esp, ebx);
    g_seh_ebp = ebp; sub_00166A40(); return; /* tail jmp 0x00166A40 */

}
