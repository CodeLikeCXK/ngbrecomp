/*
 * d3d.c -- the hook every Direct3D entry point of the title passes through
 * (src/d3d/d3d_hooks.c, docs/renderer.md).
 *
 * d3d_hook(fn) returns nonzero when the call was handled here, so the
 * recompiled body is skipped. With NGB_RENDERER=d3d the Direct3D renderer
 * (src/d3d/renderer) takes the calls it implements; otherwise every call
 * goes to the recompiled body and from there to the NV2A emulation.
 *
 * NGB_D3D_TRACE=N logs the first N calls of each function with its
 * arguments as its convention passes them (registers first, then the
 * stack), and once a second how often each was called.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define D3D_FUNCS_TABLE
#include "d3d_funcs.h"

typedef int (*D3dHandler)(void);
D3dHandler d3d_handlers[D3DF_COUNT];   /* filled by the renderer when it is on */

static int s_trace = -1;
static unsigned s_calls[D3DF_COUNT];
static unsigned s_window[D3DF_COUNT];

static uint32_t reg(const char *r)
{
    if (!r) return 0;
    switch (r[1]) {
    case 'a': return eax;
    case 'b': return r[2] == 'x' ? ebx : g_ebp;
    case 'c': return ecx;
    case 'd': return r[2] == 'x' ? edx : edi;
    case 's': return esi;
    }
    return 0;
}

static void trace(int fn)
{
    const D3dFunc *f = &d3d_funcs[fn];
    if (s_calls[fn]++ < (unsigned)s_trace) {
        fprintf(stderr, "[D3D] %-46s", f->name);
        for (int i = 0; i < 4; i++)
            if (f->regs[i])
                fprintf(stderr, " a%d(%s)=%08X", i + 1, f->regs[i], reg(f->regs[i]));
        fprintf(stderr, " stack:");
        int n = f->stack >= 0 ? f->stack / 4 : 4;
        for (int i = 0; i < n && i < 6; i++)
            fprintf(stderr, " %08X", MEM32(esp + 4 + 4 * i));
        fprintf(stderr, "  (from %08X)\n", MEM32(esp));
    }
    s_window[fn]++;
    static time_t last;
    time_t now = time(NULL);
    if (now != last) {
        last = now;
        fprintf(stderr, "[D3D] calls/s:");
        for (int i = 0; i < D3DF_COUNT; i++)
            if (s_window[i]) {
                fprintf(stderr, " %s=%u", d3d_funcs[i].name, s_window[i]);
                s_window[i] = 0;
            }
        fprintf(stderr, "\n");
    }
}

/* NGB_D3D_PUSHDUMP=N: the methods of the first N push buffers run. A
 * D3DPushBuffer: +4 data (physical), +0xC size in bytes. */
static void push_dump(void)
{
    static int left = -1;
    if (left < 0) {
        const char *e = getenv("NGB_D3D_PUSHDUMP");
        left = e ? atoi(e) : 0;
    }
    if (!left)
        return;
    left--;
    uint32_t pb = MEM32(esp + 4), data = MEM32(pb + 4) | 0x80000000u, size = MEM32(pb + 0xC);
    fprintf(stderr, "[PUSH] buffer %08X: data %08X, %u bytes, fixup %08X\n", pb, data, size, MEM32(esp + 8));
    for (uint32_t o = 0; o + 4 <= size && o < 4096;) {
        uint32_t h = MEM32(data + o);
        uint32_t count = (h >> 18) & 0x7FF, method = h & 0x1FFC, sub = (h >> 13) & 7;
        int ni = (h & 0x40000000) != 0;
        if ((h & 0xE0000003) == 0x20000000) {
            fprintf(stderr, "   %04X jump %08X\n", o, h & 0x1FFFFFFC);
            break;
        }
        fprintf(stderr, "   %04X sub %u method %04X x%u%s:", o, sub, method, count, ni ? " (same)" : "");
        for (uint32_t i = 0; i < count && i < 8; i++)
            fprintf(stderr, " %08X", MEM32(data + o + 4 + 4 * i));
        fprintf(stderr, count > 8 ? " ...\n" : "\n");
        o += 4 + 4 * count;
    }
}

/* The Direct3D renderer (src/d3d/hle.cpp): the call's registers and stack
 * pointer; nonzero when it took the call (the recompiled body is skipped). */
int d3d_hle_call(int fn, uint32_t regs[8]);
void d3d_hle_after(int fn, const uint32_t regs[8]);
static int s_hle = -1;
/* The renderer takes host locks shared with the NV2A renderer's thread; a
 * guest thread must not be parked by the runtime's scheduler while it
 * holds one (xboxrecomp win32_compat.h: a host section). */
#include "platform/vcpu.h"
#if !XBOX_HAS_VCPU
/* (the runtime's real Windows threads, without the virtual CPU, need no host/guest bracketing) */
static inline void compat_host_enter(void) {}
static inline void compat_host_leave(void) {}
#else
void compat_host_enter(void);
void compat_host_leave(void);
#endif

/* NGB_D3D_WATCHDOG=1: a thread that reports the title's thread held in a
 * hook for over two seconds, and the presenter's frame count (debugging
 * stalls). */
#include <pthread.h>
#include <time.h>
static volatile int s_in_fn = -1;
static volatile long s_in_since;
static int s_watchdog;
static long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}
static void *watchdog(void *arg)
{
    extern unsigned long nv2a_host_presents;
    (void)arg;
    for (;;) {
        struct timespec ts = { 2, 0 };
        nanosleep(&ts, NULL);
        int fn = s_in_fn;
        long since = s_in_since;
        fprintf(stderr, "[D3D] watchdog: presenter frames %lu; title thread %s%s (%ld ms)\n",
                nv2a_host_presents, fn >= 0 ? "in " : "outside the hooks",
                fn >= 0 ? d3d_funcs[fn].name : "", fn >= 0 ? now_ms() - since : 0L);
    }
    return NULL;
}

void ngb_frame_tick(void);   /* src/cheats.c */

int d3d_hook(int fn)
{
    if (fn == D3DF_D3DDevice_Swap)
        ngb_frame_tick();
    if (s_hle < 0) {
        const char *e = getenv("NGB_RENDERER");
        s_hle = e && (!strcmp(e, "d3d") || !strcmp(e, "mirror"));
        if (getenv("NGB_D3D_WATCHDOG")) {
            pthread_t th;
            s_watchdog = 1;
            if (pthread_create(&th, NULL, watchdog, NULL) == 0)
                pthread_detach(th);
        }
    }
    s_in_fn = fn;
    if (s_watchdog)   /* the clock only for it: two reads a call were 400,000 a second */
        s_in_since = now_ms();
    if (fn == D3DF_D3DDevice_RunPushBuffer)
        push_dump();
    if (s_hle) {
        uint32_t regs[8] = { eax, ecx, edx, ebx, esp, g_ebp, esi, edi };
        compat_host_enter();
        int took = d3d_hle_call(fn, regs);
        compat_host_leave();
        if (took) {
            eax = regs[0];                       /* its return value */
            esp = regs[4];                       /* its stack (ret n) */
            s_in_fn = -1;
            return 1;
        }
        const char *ren = getenv("NGB_RENDERER");
        if (ren && !strcmp(ren, "xemu"))
            s_hle = 0;
    }
    if (s_trace < 0) {
        const char *e = getenv("NGB_D3D_TRACE");
        s_trace = e ? atoi(e) : 0;
    }
    if (s_trace > 0)
        trace(fn);
    s_in_fn = -1;
    return d3d_handlers[fn] ? d3d_handlers[fn]() : 0;
}

/* After the recompiled body ran (the call was not taken): its results. */
void d3d_after(int fn)
{
    if (s_hle > 0) {
        uint32_t regs[8] = { eax, ecx, edx, ebx, esp, g_ebp, esi, edi };
        s_in_fn = fn;
        if (s_watchdog)
            s_in_since = now_ms();
        compat_host_enter();
        d3d_hle_after(fn, regs);
        compat_host_leave();
        s_in_fn = -1;
    }
}

/* D3DDevice_BeginVisibilityTest (inlined into the title, src/d3d/d3d_hooks.c): the renderer told in a host
 * section like the hooks above, as it takes g_lock -- a guest thread parked while holding it froze the game
 * in scenes with the sun (#40, found by akirahayabusa). */
void d3d_visibility_begin(void);   /* src/d3d/hle.cpp */
void d3d_visibility_hook(void)
{
    compat_host_enter();
    d3d_visibility_begin();
    compat_host_leave();
}
