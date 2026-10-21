/* The camera: which view the right stick turns, for the separate first- and third-person inversion
 * (src/bindings.c), and the view it starts in after a load (#23, #33, #63).
 *
 * The game keeps the right stick's mode in 0xDB9ACC: 0, the stick looks around in first person; 1, it
 * turns the third-person camera (the stick's click toggles it). Its two initialisers, run at every load
 * and new game, set 0; here they set the player's choice. The first-person routine 0x0004F800 runs while
 * the first-person view is up -- through the stick in mode 0, or the First-Person View control in mode 1 --
 * and marks the frame. Found by HammerD72 (#23). The functions are the lifted ones with only those changes.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include "bindings.h"
#include <stdio.h>
#include <stdlib.h>

static volatile unsigned s_frame;
static volatile unsigned s_fp_seen = ~0u - 8;   /* the frame the first-person routine last ran */

/* Whether the right stick looks around in first person now: the stick's mode, or the first-person routine
 * having run this frame or the last. */
int ngb_camera_first_person(void)
{
    return MEM8(0xDB9ACC) == 0 || s_frame - s_fp_seen <= 1;
}

/* Each frame (src/cheats.c). NGB_CAMERA_LOG=1: the stick's mode and the view, as they change. */
void ngb_camera_frame(void)
{
    s_frame++;
    static int log = -1, last = -1;
    if (log < 0)
        log = getenv("NGB_CAMERA_LOG") != NULL;
    if (log) {
        int now = MEM8(0xDB9ACC) | ngb_camera_first_person() << 8;
        if (now != last)
            fprintf(stderr, "[CAMERA] frame %u: stick mode %d (%s), first-person view %s\n", s_frame, now & 0xFF,
                    (now & 0xFF) ? "third-person camera" : "first-person look", (now >> 8) ? "yes" : "no");
        last = now;
    }
}

/**
 * sub_0004F800
 * Original: 0x0004F800 - 0x0004FECF (1743 bytes, 430 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: standard_frame
 */
void sub_0004F800(void)
{
    RECOMP_ENTRY(0x0004F800u);
    s_fp_seen = s_frame;   /* the first-person view this frame (ngb_camera_first_person) */
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    double _fca = 0.0, _fcb = 0.0;
    (void)_fca; (void)_fcb;
    #define fp_push(v) do { double _fp_value = (v); \
        g_fp_top = (g_fp_top + 7u) & 7u; \
        g_fp_stack[g_fp_top] = _fp_value; } while (0)
    #define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
    #define fp_top() g_fp_stack[g_fp_top]
    #define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
    #define fp_st1() fp_st(1)

loc_0004F800: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    esp = esp & 0xFFFFFFF0u;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0x84) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x84;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    SET_LO8(eax, MEM8(0x4B4B75));
    xmm4 = XMM_SCALAR(MEMF(0x415208)); /* movss */
    xmm5 = XMM_SCALAR(MEMF(0x4148C4)); /* movss */
    xmm3 = XMM_SCALAR(MEMF(0x414A2C)); /* movss */
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    edi = MEM32(ebp + 8);
    ebx = MEM32(edi + 0x8E0);
    _fb = (uint32_t)(0x1220) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + 0x1220;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esi = edi + 0x40;
    MEM32(esp + 0x18) = ebx;
    MEM8(esp + 0x17) = LO8(eax);
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ecx = edi + 0x42C;
    edi = edi;

loc_0004F850: ;
    _fa = (uint32_t)(MEM8(ebx + 0x13B)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(ebx + 0x13B), 2 (8-bit) */
    eax = ZX8(MEM8(0xDB932E));
    if (CMP_EQ(_fa, _fb)) goto loc_0004F86F; /* je: equal / zero */

loc_0004F860: ;
    eax = (uint32_t)((int32_t)eax * (int32_t)0x3C);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = (uint32_t)(int32_t)SMEM16(eax * 2 + 0xD219C8);
    goto loc_0004F87C;

loc_0004F86F: ;
    eax = (uint32_t)((int32_t)eax * (int32_t)0x3C);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = (uint32_t)(int32_t)SMEM16(eax * 2 + 0xD219CC);

loc_0004F87C: ;
    xmm2 = XMM_ZERO(); /* xorps self = zero */
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    _fca = xmm2.f[0]; _fcb = xmm0.f[0]; /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((_fca == _fcb || (_fca != _fca || _fcb != _fcb))) ? 0x40u : 0u) | (((_fca != _fca || _fcb != _fcb)) ? 0x04u : 0u) | (((_fca < _fcb || (_fca != _fca || _fcb != _fcb))) ? 0x01u : 0u) | 0x02u) << 8); /* lahf (ucomiss) */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x44 (8-bit) */
    MEMF(esp + edx * 4 + 0x28) = xmm0.f[0]; /* movss */
    if ((!RECOMP_PARITY8((_fa) & (_fb)))) goto loc_0004F8B0; /* jnp: not parity */

loc_0004F892: ;
    _fca = xmm2.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0004F8A0; /* jbe: below or equal (unsigned <=) (xmm2.f[0] vs xmm0.f[0]) */

loc_0004F897: ;
    xmm0 = XMM_SCALAR(MEMF(ecx)); /* movss */
    _fca = xmm0.f[0]; _fcb = xmm2.f[0]; /* comiss */
    if ((_fca >= _fcb)) goto loc_0004F8B0; /* jae: above or equal (unsigned >=) (xmm0.f[0] vs xmm2.f[0]) */

loc_0004F8A0: ;
    xmm0 = XMM_SCALAR(MEMF(esp + edx * 4 + 0x28)); /* movss */
    _fca = xmm0.f[0]; _fcb = xmm2.f[0]; /* comiss */
    if ((_fca < _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0004F8B4; /* jb: below (unsigned <) (xmm0.f[0] vs xmm2.f[0]) */

loc_0004F8AB: ;
    _fca = xmm2.f[0]; _fcb = MEMF(ecx); /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0004F8B4; /* jbe: below or equal (unsigned <=) (xmm2.f[0] vs MEMF(ecx)) */

loc_0004F8B0: ;
    MEMF(ecx) = xmm2.f[0]; /* movss */

loc_0004F8B4: ;
    xmm0 = XMM_SCALAR(MEMF(esp + edx * 4 + 0x28)); /* movss */
    xmm0.f[0] = xmm0.f[0] * xmm4.f[0]; /* mulss */
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    MEMF(esp + edx * 4 + 0x28) = xmm0.f[0]; /* movss */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    _fca = xmm0.f[0]; _fcb = xmm2.f[0]; /* comiss */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0004F8EB; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm2.f[0]) */

loc_0004F8D8: ;
    xmm6 = xmm1; /* movaps */
    xmm6.f[0] = xmm6.f[0] * xmm5.f[0]; /* mulss */
    xmm6.f[0] = xmm6.f[0] - xmm0.f[0]; /* subss */
    MEMF(esp + edx * 4 + 0x28) = xmm6.f[0]; /* movss */
    goto loc_0004F8F9;

loc_0004F8EB: ;
    xmm1.f[0] = xmm1.f[0] * xmm5.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] + xmm1.f[0]; /* addss */
    MEMF(esp + edx * 4 + 0x28) = xmm0.f[0]; /* movss */

loc_0004F8F9: ;
    xmm0 = XMM_SCALAR(MEMF(esp + edx * 4 + 0x28)); /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(ecx); /* subss */
    _fca = xmm0.f[0]; _fcb = xmm2.f[0]; /* comiss */
    if ((_fca > _fcb)) goto loc_0004F90D; /* ja: above (unsigned >) (xmm0.f[0] vs xmm2.f[0]) */

loc_0004F908: ;
    _fca = xmm2.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0004F919; /* jbe: below or equal (unsigned <=) (xmm2.f[0] vs xmm0.f[0]) */

loc_0004F90D: ;
    xmm0.f[0] = xmm0.f[0] * xmm3.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(ecx); /* addss */
    MEMF(ecx) = xmm0.f[0]; /* movss */

loc_0004F919: ;
    eax = MEM32(ecx);
    MEM32(esp + edx * 4 + 0x28) = eax;
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 2 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_0004F850; /* jl: less (signed <) */

loc_0004F92C: ;
    ecx = 8;
    MEM8(edi + 0x8F7) = 0;
    MEM8(edi + 0x8FB) = 2;
    MEM8(edi + 0x8FC) = 0;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0004F94Bu); RECOMP_ABI_CALL(0x000324C0u, sub_000324C0); /* call 0x000324C0 */

loc_0004F94B: ;
    ecx = ZX8(MEM8(0xDB932E));
    ecx = (uint32_t)((int32_t)ecx * (int32_t)0x78);
    _fa = (uint32_t)(MEM32(ecx + 0xD219A8)) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test MEM32(ecx + 0xD219A8), eax (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_0004F973; /* je: equal / zero */

loc_0004F95D: ;
    SET_LO8(edx, MEM8(edi + 0x43E));
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(edx), 0 (8-bit) */
    SET_LO8(edx, (CMP_NE(_fa, _fb)) ? 1 : 0); /* setne */
    SET_LO8(edx, LO8(edx) - 1);
    _fa = (uint32_t)(LO8(edx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fb = (_fa == 0x7Fu); /* dec result/SF/OF; CF unchanged */
    edx = edx & 3;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM8(edi + 0x43E) = LO8(edx);

loc_0004F973: ;
    _fa = (uint32_t)(MEM16(ebx + 0x134)) & 0xFFFFu; _fb = (uint32_t)(0x73) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(ebx + 0x134), 0x73 (16-bit) */
    xmm4 = XMM_SCALAR(MEMF(0x414CD0)); /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_0004F9E6; /* jne: not equal / not zero */

loc_0004F985: ;
    SET_LO8(eax, MEM8(edi + 0x43E));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0004F9BF; /* jne: not equal / not zero */

loc_0004F98F: ;
    xmm0 = XMM_SCALAR(MEMF(0x414AA8)); /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(esi + 0x30); /* subss */
    xmm3 = XMM_ZERO(); /* xorps self = zero */
    _fca = xmm0.f[0]; _fcb = xmm3.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0004F9B8; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm3.f[0]) */

loc_0004F9A4: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4149C0); /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(esi + 0x30); /* addss */
    MEMF(esi + 0x30) = xmm0.f[0]; /* movss */
    goto loc_0004F9E9;

loc_0004F9B8: ;
    _fca = xmm3.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0004F9E9; /* jbe: below or equal (unsigned <=) (xmm3.f[0] vs xmm0.f[0]) */

loc_0004F9BD: ;
    goto loc_0004F9A4;

loc_0004F9BF: ;
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    xmm0 = xmm4; /* movaps */
    xmm0.f[0] = xmm0.f[0] - MEMF(esi + 0x30); /* subss */
    _fca = xmm0.f[0]; _fcb = xmm1.f[0]; /* comiss */
    if ((_fca > _fcb)) goto loc_0004F9D4; /* ja: above (unsigned >) (xmm0.f[0] vs xmm1.f[0]) */

loc_0004F9CF: ;
    _fca = xmm1.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0004F9E6; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm0.f[0]) */

loc_0004F9D4: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4149C0); /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(esi + 0x30); /* addss */
    MEMF(esi + 0x30) = xmm0.f[0]; /* movss */

loc_0004F9E6: ;
    xmm3 = XMM_ZERO(); /* xorps self = zero */

loc_0004F9E9: ;
    xmm0.f[0] = (float)(int32_t)MEM32(edi + 0x428); /* cvtsi2ss */
    xmm2 = XMM_SCALAR(MEMF(0x414BBC)); /* movss */
    _fca = xmm2.f[0]; _fcb = xmm0.f[0]; /* comiss */
    xmm1 = xmm5; /* movaps */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0004FA18; /* jbe: below or equal (unsigned <=) (xmm2.f[0] vs xmm0.f[0]) */

loc_0004FA01: ;
    xmm1 = xmm2; /* movaps */
    xmm1.f[0] = xmm1.f[0] - xmm0.f[0]; /* subss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x414D14); /* mulss */
    xmm1.f[0] = xmm1.f[0] * xmm2.f[0]; /* mulss */
    xmm1.f[0] = xmm1.f[0] + xmm5.f[0]; /* addss */

loc_0004FA18: ;
    eax = ZX8(MEM8(esp + 0x17));
    xmm0 = XMM_SCALAR(MEMF(esi + 0x30)); /* movss */
    xmm5 = XMM_SCALAR(MEMF(esp + 0x2C)); /* movss */
    xmm0.f[0] = xmm0.f[0] - xmm4.f[0]; /* subss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x415204); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414BF4); /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x414A5C); /* addss */
    xmm4 = XMM_SCALAR(MEMF(esp + 0x28)); /* movss */
    edx = eax + eax * 4;
    edx = edx << 0xB;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    SET_LO8(eax, MEM8(edx + 0x4B75E9));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(5) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 5 (8-bit) */
    ebx = edx + 0x4B4E50;
    xmm4.f[0] = xmm4.f[0] * xmm0.f[0]; /* mulss */
    xmm4.f[0] = xmm4.f[0] * MEMF(0x3EC068); /* mulss */
    xmm5.f[0] = xmm5.f[0] * xmm0.f[0]; /* mulss */
    xmm5.f[0] = xmm5.f[0] * xmm1.f[0]; /* mulss */
    MEMF(esp + 0x24) = xmm4.f[0]; /* movss */
    MEMF(esp + 0x20) = xmm5.f[0]; /* movss */
    MEM32(esp + 0x28) = edx;
    if (CMP_NE(_fa, _fb)) goto loc_0004FA9C; /* jne: not equal / not zero */

loc_0004FA84: ;
    xmm0 = XMM_SCALAR(MEMF(0x4149CC)); /* movss */
    MEMF(esp + 0x30) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x4149DC)); /* movss */
    goto loc_0004FAC6;

loc_0004FA9C: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 4 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0004FAB8; /* jne: not equal / not zero */

loc_0004FAA0: ;
    xmm0 = XMM_SCALAR(MEMF(0x414AC0)); /* movss */
    MEMF(esp + 0x30) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x414ABC)); /* movss */
    goto loc_0004FAC6;

loc_0004FAB8: ;
    xmm0 = XMM_SCALAR(MEMF(0x414998)); /* movss */
    MEMF(esp + 0x30) = xmm3.f[0]; /* movss */

loc_0004FAC6: ;
    edi = MEM32(0xD52688);
    MEMF(esp + 0x34) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    _fb = (uint32_t)(0x40) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edi = edi - 0x40;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEMF(esp + 0x38) = xmm3.f[0]; /* movss */
    MEMF(esp + 0x3C) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x40) = xmm3.f[0]; /* movss */
    MEMF(esp + 0x44) = xmm3.f[0]; /* movss */
    MEMF(esp + 0x48) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x4C) = xmm3.f[0]; /* movss */
    MEM32(esp + 0x1C) = edi;
    ecx = MEM32(esp + 0x1C);
    eax = 0x950180;
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    ecx = MEM32(esp + 0x18);
    edx = edx + 0x4B5710;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0004FB3Bu); RECOMP_ABI_CALL(0x003059A0u, sub_003059A0); /* call 0x003059A0 */

loc_0004FB3B: ;
    eax = esp + 0x30;
    ecx = eax;
    MEM32(esp + 0x18) = eax;
    MEM32(esp + 0x1C) = ecx;
    eax = MEM32(esp + 0x18);
    ecx = 0x950180;
    xmm2 = XMM_SCALAR(MEMF(eax)); /* movss */
    xmm2 = XMM_SHUFFLE(xmm2, xmm2, 0); /* shufps */
    xmm2 = XMM_MUL(xmm2, XMM_MEM(ecx)); /* mulps */
    xmm1 = XMM_SCALAR(MEMF(eax + 4)); /* movss */
    xmm1 = XMM_SHUFFLE(xmm1, xmm1, 0); /* shufps */
    xmm1 = XMM_MUL(xmm1, XMM_MEM(ecx + 0x10)); /* mulps */
    xmm3 = XMM_SCALAR(MEMF(eax + 8)); /* movss */
    xmm3 = XMM_SHUFFLE(xmm3, xmm3, 0); /* shufps */
    xmm3 = XMM_MUL(xmm3, XMM_MEM(ecx + 0x20)); /* mulps */
    xmm2 = XMM_ADD(xmm2, XMM_MEM(ecx + 0x30)); /* addps */
    eax = MEM32(esp + 0x1C);
    xmm2 = XMM_ADD(xmm2, xmm1); /* addps */
    xmm2 = XMM_ADD(xmm2, xmm3); /* addps */
    XMM_STORE(eax, xmm2); /* movups */
    edx = esp + 0x40;
    eax = edx;
    MEM32(esp + 0x18) = edx;
    MEM32(esp + 0x1C) = eax;
    eax = MEM32(esp + 0x18);
    xmm2 = XMM_SCALAR(MEMF(eax)); /* movss */
    ecx = 0x950180;
    xmm2 = XMM_SHUFFLE(xmm2, xmm2, 0); /* shufps */
    xmm2 = XMM_MUL(xmm2, XMM_MEM(ecx)); /* mulps */
    xmm1 = XMM_SCALAR(MEMF(eax + 4)); /* movss */
    xmm1 = XMM_SHUFFLE(xmm1, xmm1, 0); /* shufps */
    xmm1 = XMM_MUL(xmm1, XMM_MEM(ecx + 0x10)); /* mulps */
    xmm3 = XMM_SCALAR(MEMF(eax + 8)); /* movss */
    xmm3 = XMM_SHUFFLE(xmm3, xmm3, 0); /* shufps */
    xmm3 = XMM_MUL(xmm3, XMM_MEM(ecx + 0x20)); /* mulps */
    xmm2 = XMM_ADD(xmm2, xmm1); /* addps */
    eax = MEM32(esp + 0x1C);
    xmm2 = XMM_ADD(xmm2, xmm3); /* addps */
    XMM_STORE_LOW(eax, xmm2); /* movlps */
    xmm2 = XMM_MOVE_HIGH_TO_LOW(xmm2, xmm2); /* movhlps */
    MEMF(eax + 8) = xmm2.f[0]; /* movss */
    MEM32(esp + 0x1C) = edi;
    _fb = (uint32_t)(0x40) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 0x40;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(0xD52688) = edi;
    ecx = 0x950180;
    eax = MEM32(esp + 0x1C);
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x30)); /* movss */
    MEMF(esi) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x34)); /* movss */
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, edi (32-bit) */
    MEMF(esi + 4) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x38)); /* movss */
    MEMF(esi + 8) = xmm0.f[0]; /* movss */
    if (CMP_EQ(_fa, _fb)) goto loc_0004FDEE; /* je: equal / zero */

loc_0004FC37: ;
    SET_LO8(eax, MEM8(ebx + 0x278E));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0004FDEE; /* jne: not equal / not zero */

loc_0004FC45: ;
    SET_LO8(ebx, MEM8(ebx + 0x27F0));
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(0x20) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ebx), 0x20 (8-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_0004FDEE; /* jae: above or equal (unsigned >=) */

loc_0004FC54: ;
    eax = ZX8(LO8(ebx));
    eax = (uint32_t)((int32_t)eax * (int32_t)0xDD0);
    _fb = (uint32_t)(0x4968B0) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x4968B0;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    SET_LO8(ecx, MEM8(eax + 0x220));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0004FDEE; /* je: equal / zero */

loc_0004FC70: ;
    _fb = (uint32_t)(0x280) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x280;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = esp + 0x50;
    MEM32(esp + 0x1C) = eax;
    MEM32(esp + 0x18) = ecx;
    ecx = MEM32(esp + 0x18);
    eax = MEM32(esp + 0x1C);
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    edx = esp + 0x50;
    PUSH32(esp, edx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0004FCB1u); RECOMP_ABI_CALL(0x00305870u, sub_00305870); /* call 0x00305870 */

loc_0004FCB1: ;
    ebx = esi + 0x10;
    MEM32(esp + 0x1C) = ebx;
    eax = MEM32(esp + 0x1C);
    ecx = 0x950180;
    xmm2 = XMM_SCALAR(MEMF(eax)); /* movss */
    xmm2 = XMM_SHUFFLE(xmm2, xmm2, 0); /* shufps */
    xmm2 = XMM_MUL(xmm2, XMM_MEM(ecx)); /* mulps */
    xmm1 = XMM_SCALAR(MEMF(eax + 4)); /* movss */
    xmm1 = XMM_SHUFFLE(xmm1, xmm1, 0); /* shufps */
    xmm1 = XMM_MUL(xmm1, XMM_MEM(ecx + 0x10)); /* mulps */
    xmm3 = XMM_SCALAR(MEMF(eax + 8)); /* movss */
    xmm3 = XMM_SHUFFLE(xmm3, xmm3, 0); /* shufps */
    xmm3 = XMM_MUL(xmm3, XMM_MEM(ecx + 0x20)); /* mulps */
    xmm2 = XMM_ADD(xmm2, XMM_MEM(ecx + 0x30)); /* addps */
    eax = MEM32(esp + 0x1C);
    xmm2 = XMM_ADD(xmm2, xmm1); /* addps */
    xmm2 = XMM_ADD(xmm2, xmm3); /* addps */
    XMM_STORE(eax, xmm2); /* movups */
    eax = MEM32(0xD52688);
    MEM32(esp + 0x1C) = eax;
    _fb = (uint32_t)(0x40) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x40;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(0xD52688) = eax;
    ecx = 0x950180;
    eax = MEM32(esp + 0x1C);
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    xmm0 = XMM_SCALAR(MEMF(esi)); /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(ebx); /* subss */
    MEMF(esp + 0x18) = xmm0.f[0]; /* movss */
    fp_push(MEMF(esp + 0x18)); /* fld float */
    xmm0 = XMM_SCALAR(MEMF(esi + 8)); /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(esi + 0x18); /* subss */
    MEMF(esp + 0x1C) = xmm0.f[0]; /* movss */
    fp_push(MEMF(esp + 0x1C)); /* fld float */
    { fp_st1() = atan2(fp_st1(), fp_top()); fp_pop(); } /* fpatan */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x414958)); /* fmul dword ptr [0x414958] */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0004FD62u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_0004FD62: ;
    xmm1 = XMM_SCALAR(MEMF(esp + 0x18)); /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x1C)); /* movss */
    xmm2 = xmm1; /* movaps */
    xmm2.f[0] = xmm2.f[0] * xmm1.f[0]; /* mulss */
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    xmm0.f[0] = xmm0.f[0] * xmm0.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] + xmm2.f[0]; /* addss */
    _fca = xmm1.f[0]; _fcb = xmm0.f[0]; /* ucomiss */
    MEM32(esi + 0x24) = eax;
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((_fca == _fcb || (_fca != _fca || _fcb != _fcb))) ? 0x40u : 0u) | (((_fca != _fca || _fcb != _fcb)) ? 0x04u : 0u) | (((_fca < _fcb || (_fca != _fca || _fcb != _fcb))) ? 0x01u : 0u) | 0x02u) << 8); /* lahf (ucomiss) */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x44 (8-bit) */
    MEMF(esp + 0x18) = xmm0.f[0]; /* movss */
    if (RECOMP_PARITY8((_fa) & (_fb))) goto loc_0004FDA0; /* jp: parity */

loc_0004FD92: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esp + 0x18) = xmm0.f[0]; /* movss */

loc_0004FDA0: ;
    fp_push(MEMF(esi + 4)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(esi + 0x14)); /* fsub dword ptr [esi + 0x14] */
    fp_push(MEMF(esp + 0x18)); /* fld float */
    fp_top() = RECOMP_FP_PC(sqrt(fp_top())); /* fsqrt */
    { double _t = fp_top(); fp_top() = fp_st1(); fp_st1() = _t; } /* fxch st(1) */
    { double _t = fp_top(); fp_top() = fp_st1(); fp_st1() = _t; } /* fxch st(1) */
    { fp_st1() = atan2(fp_st1(), fp_top()); fp_pop(); } /* fpatan */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x414958)); /* fmul dword ptr [0x414958] */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0004FDBDu); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_0004FDBD: ;
    MEM32(esi + 0x20) = eax;
    MEM16(esi + 0x22) = LO16(edi);
    eax = MEM32(esi + 0x20);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x8000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x8000 (32-bit) */
    if (CMP_LE(_fas, _fbs)) goto loc_0004FDD6; /* jle: less or equal (signed <=) */

loc_0004FDCE: ;
    _fb = (uint32_t)(0xFFFF0000u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xFFFF0000u;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi + 0x20) = eax;

loc_0004FDD6: ;
    eax = esi;
    ecx = ebx;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0004FDDFu); RECOMP_ABI_CALL(0x00304ED0u, sub_00304ED0); /* call 0x00304ED0 */

loc_0004FDDF: ;
    MEMF(esi + 0x2C) = (float)fp_top(); fp_pop(); /* fstp */
    xmm5 = XMM_SCALAR(MEMF(esp + 0x20)); /* movss */
    xmm4 = XMM_SCALAR(MEMF(esp + 0x24)); /* movss */

loc_0004FDEE: ;
    SET_LO8(eax, MEM8(0xDB940B));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    xmm0 = XMM_SCALAR(MEMF(0x414928)); /* movss */
    xmm1 = XMM_SCALAR(MEMF(0x415200)); /* movss */
    xmm4.f[0] = xmm4.f[0] * xmm0.f[0]; /* mulss */
    if (CMP_NE(_fa, _fb)) goto loc_0004FE1D; /* jne: not equal / not zero */

loc_0004FE0B: ;
    ecx = MEM32(esi + 0x20);
    xmm4.f[0] = xmm4.f[0] * xmm1.f[0]; /* mulss */
    eax = (int32_t)recomp_fist((double)xmm4.f[0], 0x0E7Fu, 32); /* cvttss2si */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi + 0x20) = ecx;
    goto loc_0004FE31;

loc_0004FE1D: ;
    eax = MEM32(esi + 0x20);
    xmm4.f[0] = xmm4.f[0] * MEMF(0x41499C); /* mulss */
    ecx = (int32_t)recomp_fist((double)xmm4.f[0], 0x0E7Fu, 32); /* cvttss2si */
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi + 0x20) = eax;

loc_0004FE31: ;
    eax = MEM32(esi + 0x24);
    xmm5.f[0] = xmm5.f[0] * xmm0.f[0]; /* mulss */
    xmm5.f[0] = xmm5.f[0] * xmm1.f[0]; /* mulss */
    edx = (int32_t)recomp_fist((double)xmm5.f[0], 0x0E7Fu, 32); /* cvttss2si */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi + 0x24) = eax;
    eax = esp + 0x40;
    PUSH32(esp, eax);
    ecx = esp + 0x28;
    PUSH32(esp, ecx);
    edx = esp + 0x28;
    PUSH32(esp, edx);
    eax = esp + 0x28;
    PUSH32(esp, eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0004FE5Eu); RECOMP_ABI_CALL(0x00040220u, sub_00040220); /* call 0x00040220 */

loc_0004FE5E: ;
    MEM16(esi + 0x22) = LO16(edi);
    eax = MEM32(esi + 0x20);
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x10;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x8000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x8000 (32-bit) */
    if (CMP_LE(_fas, _fbs)) goto loc_0004FE77; /* jle: less or equal (signed <=) */

loc_0004FE6F: ;
    _fb = (uint32_t)(0xFFFF0000u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xFFFF0000u;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi + 0x20) = eax;

loc_0004FE77: ;
    ecx = MEM32(esp + 0x28);
    SET_LO8(eax, MEM8(ecx + 0x4B75E9));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(5) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 5 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0004FE87; /* je: equal / zero */

loc_0004FE85: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 4 (8-bit) */

loc_0004FE87: ;
    eax = MEM32(esi + 0x20);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x31C7) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x31C7 (32-bit) */
    if (CMP_LE(_fas, _fbs)) goto loc_0004FE96; /* jle: less or equal (signed <=) */

loc_0004FE91: ;
    eax = 0x31C7;

loc_0004FE96: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFC71Du) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xFFFFC71Du (32-bit) */
    MEM32(esi + 0x20) = eax;
    if (CMP_GE(_fas, _fbs)) goto loc_0004FEA5; /* jge: greater or equal (signed >=) */

loc_0004FEA0: ;
    eax = 0xFFFFC71Du;

loc_0004FEA5: ;
    MEM32(esi + 0x20) = eax;
    SET_LO8(eax, MEM8(0x4B4B6B));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 2 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0004FEC8; /* je: equal / zero */

loc_0004FEB1: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(6) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 6 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0004FEC8; /* je: equal / zero */

loc_0004FEB5: ;
    eax = MEM32(ebp + 8);
    MEM32(eax + 0x420) = edi;
    MEM32(eax + 0x424) = 1;

loc_0004FEC8: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp);
    esp += 4; return; /* ret */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}


/**
 * sub_00167290
 * Original: 0x00167290 - 0x00167642 (946 bytes, 213 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_00167290(void)
{
    RECOMP_ENTRY(0x00167290u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_00167290: ;
    PUSH32(esp, ecx);
    SET_LO8(eax, MEM8(0xDB5A60));
    PUSH32(esp, ebx);
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(0xDB9340) = ecx;
    MEM32(0xDB9344) = ecx;
    MEM32(0xDB9348) = ecx;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEM32(0xDB934C) = ecx;
    MEM32(0xDB9350) = ecx;
    MEM32(0xDB9354) = ecx;
    MEM32(0xDB9358) = ecx;
    MEM32(0xDB935C) = ecx;
    MEM32(0xDB9360) = ecx;
    MEM32(0xDB9364) = ecx;
    MEM32(0xDB9368) = ecx;
    MEM32(0xDB936C) = ecx;
    MEM32(0xDB9370) = ecx;
    MEM32(0xDB9374) = ecx;
    PUSH32(esp, esi);
    esi = MEM32(0xD52688);
    MEM8(0xDB9888) = LO8(eax);
    MEM8(0xDB9894) = LO8(ebx);
    MEM8(0xDB9AD6) = LO8(ebx);
    MEM8(0xDB9AD5) = LO8(ebx);
    MEM16(0xDB9ADC) = 0xFFFF;
    MEM8(0xDB932A) = LO8(ebx);
    MEM32(0xDB9378) = ecx;
    MEM8(0xDB9ADB) = LO8(ebx);
    MEMF(0xDB988C) = xmm0.f[0]; /* movss */
    MEM8(0xDB9ADA) = 0x3C;
    MEM8(0xDB9321) = LO8(ebx);
    MEM8(0xDB9320) = LO8(ebx);
    MEM8(0xDB9AC6) = LO8(ebx);
    MEM8(0xDB9AD4) = LO8(ebx);
    MEM16(0xDB9AC2) = LO16(ebx);
    MEM8(0xDB9C60) = LO8(ebx);
    edx = 0x4B4BC0;
    /* nop */

loc_00167370: ;
    _fb = (uint32_t)(0x40) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esi = esi - 0x40;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(esp + 8) = esi;
    ecx = MEM32(esp + 8);
    eax = 0x950180;
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    ecx = 0x950180;
    eax = 0x950140;
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    eax = edx + -64;
    MEM32(esp + 8) = eax;
    ecx = MEM32(esp + 8);
    eax = 0x950180;
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    MEM32(esp + 8) = edx;
    ecx = MEM32(esp + 8);
    eax = 0x950180;
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    ecx = edx + 0x40;
    MEM32(esp + 8) = ecx;
    ecx = MEM32(esp + 8);
    eax = 0x950180;
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    eax = edx + 0x80;
    MEM32(esp + 8) = eax;
    ecx = MEM32(esp + 8);
    eax = 0x950180;
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    ecx = edx + 0xC0;
    MEM32(esp + 8) = ecx;
    ecx = MEM32(esp + 8);
    eax = 0x950180;
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    MEM32(esp + 8) = esi;
    _fb = (uint32_t)(0x40) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x40;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = 0x950180;
    eax = MEM32(esp + 8);
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    _fb = (uint32_t)(0x140) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0x140;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x4B4E40) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0x4B4E40 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_00167370; /* jl: less (signed <) */

loc_001674EF: ;
    SET_LO8(eax, 0xFF);
    MEM32(0xD52688) = esi;
    MEM16(0xDB9C6E) = 0xFFFF;
    MEM16(0xDB9AC4) = LO16(ebx);
    MEM32(0xDB9AD0) = ebx;
    MEM32(0xDB9C64) = ebx;
    MEM16(0xDB9328) = LO16(ebx);
    MEM16(0xDB9AD8) = LO16(ebx);
    MEM16(0xDB9334) = LO16(ebx);
    MEM8(0xDB9896) = LO8(ebx);
    MEM8(0xDB9895) = LO8(ebx);
    MEM8(0xDB937C) = LO8(ebx);
    MEM16(0xDB932C) = LO16(ebx);
    MEM8(0xDB937D) = LO8(ebx);
    MEM8(0xDB9AC0) = LO8(eax);
    MEM32(0x4B4E40) = ebx;
    MEM8(0xDB9C6A) = LO8(ebx);
    MEM8(0xDB9C6B) = LO8(eax);
    MEM16(0xDB9C68) = LO16(ebx);
    MEM8(0xDB9C6C) = LO8(eax);
    esi = 0x4B4E50;
    edi = edi;

loc_00167570: ;
    PUSH32(esp, esi);
    PUSH32(esp, 0x00167576u); RECOMP_ABI_CALL(0x00168EC0u, sub_00168EC0); /* call 0x00168EC0 */

loc_00167576: ;
    _fb = (uint32_t)(0x2800) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x2800;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x554E50) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0x554E50 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_00167570; /* jl: less (signed <) */

loc_00167587: ;
    esi = 0x58A3A0;
    /* nop */

loc_00167590: ;
    PUSH32(esp, esi);
    PUSH32(esp, 0x00167596u); RECOMP_ABI_CALL(0x00169C00u, sub_00169C00); /* call 0x00169C00 */

loc_00167596: ;
    _fb = (uint32_t)(0x210) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x210;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x58E5A0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0x58E5A0 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_00167590; /* jl: less (signed <) */

loc_001675A7: ;
    eax = 0xDB92C2;
    /* nop */

loc_001675B0: ;
    MEM8(eax + -2) = LO8(ebx);
    MEM8(eax + -1) = LO8(ebx);
    MEM8(eax) = LO8(ebx);
    _fb = (uint32_t)(3) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 3;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xDB9322) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xDB9322 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_001675B0; /* jl: less (signed <) */

loc_001675C2: ;
    eax = 0xDB9AE2;
    goto loc_001675D0;

    /* nop */

loc_001675D0: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEM8(eax + -2) = LO8(ebx);
    MEM8(eax + -1) = LO8(ebx);
    MEM8(eax) = LO8(ebx);
    MEM16(eax + 2) = LO16(ebx);
    MEMF(eax + 6) = xmm0.f[0]; /* movss */
    MEM8(eax + 1) = LO8(ebx);
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xDB9C62) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xDB9C62 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_001675D0; /* jl: less (signed <) */

loc_001675F6: ;
    esi = 0x554E50;
    goto loc_00167600;

    /* nop */

loc_00167600: ;
    eax = esi;
    PUSH32(esp, 0x00167607u); RECOMP_ABI_CALL(0x00169FF0u, sub_00169FF0); /* call 0x00169FF0 */

loc_00167607: ;
    _fb = (uint32_t)(0x6030) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x6030;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x572F40) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0x572F40 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_00167600; /* jl: less (signed <) */

loc_00167615: ;
    SET_LO8(eax, 1);
    POP32(esp, esi);
    MEM8(0xDB932E) = LO8(ebx);
    MEM32(0xDB9330) = ebx;
    MEM32(0xDB9324) = ebx;
    MEM8(0xDB9ACC) = (uint8_t)ngb_bind_third_person_start();   /* the game's 0: first-person look */
    MEM8(0x7F1A9F) = LO8(eax);
    MEM8(0x7F1A9D) = LO8(eax);
    MEM8(0x7F1A9E) = LO8(eax);
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

}

/**
 * sub_002EC580
 * Original: 0x002EC580 - 0x002EC6FB (379 bytes, 84 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_002EC580(void)
{
    RECOMP_ENTRY(0x002EC580u);
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_002EC580: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    SET_LO8(ebx, 0); /* xor self */
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    SET_LO8(eax, 0); /* xor self */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    MEM8(0xDB937C) = 5;
    MEM8(0xDB9321) = LO8(ebx);
    MEM8(0xDB9320) = LO8(ebx);
    MEM8(0xDB9AC6) = LO8(ebx);
    PUSH32(esp, 0x002EC5A6u); RECOMP_ABI_CALL(0x002FCE00u, sub_002FCE00); /* call 0x002FCE00 */

loc_002EC5A6: ;
    MEM8(0xDB9AD4) = LO8(eax);
    MEM16(0xDB9AC2) = 0x200;
    MEM16(0xDB9ADC) = 0xFFFF;
    MEM8(0xDB932A) = LO8(ebx);
    MEM8(0xDB9ACC) = (uint8_t)ngb_bind_third_person_start();   /* the game's 0: first-person look */
    MEM8(0x7F1A9D) = 1;
    MEM8(0x7F1A9E) = 1;
    MEM8(0x7F1A9F) = 1;
    PUSH32(esp, 0x002EC5E3u); RECOMP_ABI_CALL(0x002ECDF0u, sub_002ECDF0); /* call 0x002ECDF0 */

loc_002EC5E3: ;
    SET_LO8(edx, 0); /* xor self */
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    PUSH32(esp, 0x002EC5EAu); RECOMP_ABI_CALL(0x002FB120u, sub_002FB120); /* call 0x002FB120 */

loc_002EC5EA: ;
    PUSH32(esp, 0x002EC5EFu); RECOMP_ABI_CALL(0x003058E0u, sub_003058E0); /* call 0x003058E0 */

loc_002EC5EF: ;
    MEM32(esp + 8) = 0x4B4CC0;
    ecx = MEM32(esp + 8);
    eax = 0x950180;
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    MEM32(esp + 8) = 0x4B4D00;
    ecx = MEM32(esp + 8);
    eax = 0x950180;
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    MEM32(esp + 8) = 0x4B4D40;
    ecx = MEM32(esp + 8);
    eax = 0x950180;
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    MEM32(esp + 8) = 0x4B4D80;
    ecx = MEM32(esp + 8);
    eax = 0x950180;
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    MEM32(esp + 8) = 0x4B4DC0;
    ecx = MEM32(esp + 8);
    eax = 0x950180;
    xmm0 = XMM_MEM(eax); /* movaps */
    xmm1 = XMM_MEM(eax + 0x10); /* movaps */
    xmm2 = XMM_MEM(eax + 0x20); /* movaps */
    xmm3 = XMM_MEM(eax + 0x30); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    XMM_STORE(ecx + 0x10, xmm1); /* movaps */
    XMM_STORE(ecx + 0x20, xmm2); /* movaps */
    XMM_STORE(ecx + 0x30, xmm3); /* movaps */
    eax = 1;
    PUSH32(esp, 0x002EC6E4u); RECOMP_ABI_CALL(0x00305950u, sub_00305950); /* call 0x00305950 */

loc_002EC6E4: ;
    MEM8(0xDB9C60) = LO8(ebx);
    MEM8(0xDB9AD6) = LO8(ebx);
    MEM8(0xDB9894) = LO8(ebx);
    POP32(esp, ebx);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

}

