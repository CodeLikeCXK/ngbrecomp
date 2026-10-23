/*
 * MCPX APU DSP (GP/EP) - Stub implementation
 *
 * The DSP Global Processor (GP) and Encode Processor (EP) handle effects
 * processing (reverb, chorus, etc.) and final output encoding. The full
 * DSP is ~3000 lines of DSP56300 emulation code.
 *
 * For initial audio, we bypass the DSP entirely:
 * - VP mixbins are passed directly to the EP output
 * - GP effects processing is skipped
 * - The EP just copies mixbin 0/1 (front L/R) to the monitor buffer
 *
 * This gives us basic voice playback without effects. The DSP can be
 * connected later for reverb, EQ, and other processing.
 *
 * Copyright (c) 2012 espes
 * Copyright (c) 2019-2025 Matt Borgerson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include "apu_state.h"
#include "fpconv.h"

#include <stdlib.h>
#include <string.h>

/* ── DSP command doorbell acknowledgement ────────────────────────────────
 *
 * DirectSound does not stop at creating the device. It hands the audio DSP a
 * command block in guest RAM, writes a command word, and spins until the DSP
 * writes zero back. On real hardware the GP runs a DSP56300 program that does
 * that. Here the DSP is a passthrough stub, so the word never changes and the
 * title hangs inside DirectSound initialisation -- which on Wreckless gates the
 * entire engine, not just audio.
 *
 * RECOMP_APU_DSP_ACK=<addr>[,<addr>...] clears those guest dwords once per APU
 * frame, which is what "the command completed" looks like to the title.
 *
 * ponytail: this is a handshake acknowledgement, not a DSP. It says every
 * command succeeded instantly and computes nothing, so anything whose *result*
 * the title reads back will still be wrong. The real fix is DSP56300 emulation
 * in the GP/EP; this exists so audio init stops blocking everything behind it.
 *
 * The address is not derivable from the APU registers: GPSADDR/GPFADDR/
 * EPSADDR/EPFADDR point at the DSP's own scratch and frame memory, while the
 * command block is a DirectSound heap allocation. On Wreckless the registers
 * read 0x01504000 / 0x014EC000 / 0x0151C000 / 0x014F0000 and the doorbell is at
 * 0x014F8810 -- inside none of them. So it has to be observed: run with
 * RECOMP_WATCHDOG_SECS and the spin shows up as ebx plus the poll offset.
 */
#define APU_DSP_ACK_MAX 8
static uint32_t s_dsp_ack[APU_DSP_ACK_MAX];
static int s_dsp_ack_count = -1;

static void dsp_ack_init(void)
{
    const char *spec = getenv("RECOMP_APU_DSP_ACK");
    char buf[256], *p, *end;

    s_dsp_ack_count = 0;
    if (!spec || !*spec)
        return;
    strncpy(buf, spec, sizeof buf - 1);
    buf[sizeof buf - 1] = 0;
    for (p = buf; *p && s_dsp_ack_count < APU_DSP_ACK_MAX; ) {
        unsigned long v = strtoul(p, &end, 0);
        if (end == p)
            break;
        if (v)
            s_dsp_ack[s_dsp_ack_count++] = (uint32_t)v;
        p = (*end == ',') ? end + 1 : end;
    }
    if (s_dsp_ack_count)
        fprintf(stderr, "[APU] DSP doorbell ack: %d address(es), first 0x%08X\n",
                s_dsp_ack_count, s_dsp_ack[0]);
}

/* Downmix every bin the guest routes to (see mcpx_apu_dsp_frame), not just
 * the first two. Jet Set Radio Future's 3D sound effects live in bins 6-10
 * and were all lost to the two-bin read. RECOMP_APU_MIXDOWN_ALL=0 restores
 * it. */
static int mcpx_apu_mixdown_all(void)
{
    static int on = -1;
    if (on < 0) {
        const char *e = getenv("RECOMP_APU_MIXDOWN_ALL");
        on = (e && *e) ? (atoi(e) != 0) : 1;
    }
    return on;
}

/* The word the EP's program keeps at P:6 while it runs. DirectSound reads it from its APU service
 * routine, and when the EP is not running (no 0xCCCCCC there) it resets the whole APU, which ends in a
 * 10 ms KeStallExecutionProcessor. The EP is not run here, so every service found it stopped: the arcade
 * Ninja Gaiden (NinjaTrilogy.xbe, #18) lost 10 ms of each 16.7 ms frame to the stall and restarted the
 * audio each time. The GP and EP memories are plain RAM (xbox_memory_layout.c), so this is where the
 * stand-in for the DSPs says the EP is up. */
#define APU_EP_RUNNING_VA   0xFE85A018u   /* EP P memory (+0x5A000) word 6 */
#define APU_EP_RUNNING      0x00CCCCCCu
extern int xbox_ApuMmioTrapped(void);

void mcpx_apu_dsp_ack_poll(MCPXAPUState *d)
{
    int i;

    if (s_dsp_ack_count < 0)
        dsp_ack_init();
    if (!d->ram_ptr)
        return;
    if (xbox_ApuMmioTrapped())
        *(volatile uint32_t *)(d->ram_ptr + APU_EP_RUNNING_VA) = APU_EP_RUNNING;
    for (i = 0; i < s_dsp_ack_count; i++) {
        uint32_t *slot = (uint32_t *)(d->ram_ptr + s_dsp_ack[i]);
        if (*slot) {
            static int shown[APU_DSP_ACK_MAX];
            if (shown[i]++ < 3)
                fprintf(stderr, "[APU] DSP doorbell 0x%08X: command 0x%08X"
                                " acknowledged\n", s_dsp_ack[i], *slot);
            *slot = 0;
        }
    }
}

void mcpx_apu_dsp_init(MCPXAPUState *d)
{
    /* Allocate minimal DSP state for GP and EP.
     * We need these to exist so reset doesn't crash,
     * but they won't actually run DSP programs. */
    d->gp.dsp = (DSPState *)calloc(1, sizeof(DSPState));
    d->ep.dsp = (DSPState *)calloc(1, sizeof(DSPState));

    if (d->gp.dsp) d->gp.dsp->is_gp = true;
    if (d->ep.dsp) d->ep.dsp->is_gp = false;

    d->gp.realtime = false;
    d->ep.realtime = false;

    fprintf(stderr, "[APU] DSP GP/EP initialized (STUBBED - passthrough mode)\n");
}

void mcpx_apu_update_dsp_preference(MCPXAPUState *d)
{
    /* In the real xemu, this reads settings to decide whether
     * GP/EP should run in realtime or cached mode. We ignore it. */
    (void)d;
}

/* A small stereo reverb (Freeverb's structure: eight parallel combs and four
 * series allpasses per side) standing in for the GP's I3DL2 program, fed by
 * the reverb send. Without it the send was either dropped or, worse, added
 * as a second dry copy. Tunings are Freeverb's at 44.1 kHz scaled to 48. */
#define RV_COMBS 8
#define RV_APS 4
static const int rv_comb_len[RV_COMBS] = { 1214, 1293, 1390, 1475, 1548, 1622, 1695, 1764 };
static const int rv_ap_len[RV_APS] = { 605, 480, 371, 245 };
#define RV_SPREAD 25
static struct {
    float comb[2][RV_COMBS][1800 + RV_SPREAD];
    int comb_pos[2][RV_COMBS];
    float comb_lp[2][RV_COMBS];
    float ap[2][RV_APS][700 + RV_SPREAD];
    int ap_pos[2][RV_APS];
} s_rv;

static float reverb_side(int ch, float in)
{
    const float feedback = 0.80f, damp = 0.25f;
    float out = 0.0f;
    for (int c = 0; c < RV_COMBS; c++) {
        int len = rv_comb_len[c] + ch * RV_SPREAD;
        float *buf = s_rv.comb[ch][c];
        int *pos = &s_rv.comb_pos[ch][c];
        float y = buf[*pos];
        s_rv.comb_lp[ch][c] = y * (1.0f - damp) + s_rv.comb_lp[ch][c] * damp;
        buf[*pos] = in + s_rv.comb_lp[ch][c] * feedback;
        if (++*pos >= len) *pos = 0;
        out += y;
    }
    for (int a = 0; a < RV_APS; a++) {
        int len = rv_ap_len[a] + ch * RV_SPREAD;
        float *buf = s_rv.ap[ch][a];
        int *pos = &s_rv.ap_pos[ch][a];
        float b = buf[*pos];
        buf[*pos] = out + b * 0.5f;
        out = b - out;
        if (++*pos >= len) *pos = 0;
    }
    return out;
}

void mcpx_apu_dsp_frame(MCPXAPUState *d,
                         float mixbins[NUM_MIXBINS][NUM_SAMPLES_PER_FRAME])
{
    /* The GP and EP DSPs are not run; this does what their stereo output
     * amounts to. DirectSound's hardware mixbins are its DSMIXBIN numbers:
     *
     *   0/1 front L/R   2 centre   3 LFE   4/5 rear L/R
     *   6-9 3D crosstalk (front L/R, rear L/R)   10 I3DL2 reverb send
     *   11+ effect sends -- inputs to GP effect programs
     *
     * Downmix: each side gets its front, rear and crosstalk bins, the
     * centre at -3 dB and LFE at -6 dB; the reverb send goes through the
     * reverb above. Effect sends are dropped: they are inputs to programs
     * that do not run, and the dry signal is already in the main bins.
     *
     * This replaced "even bins left, odd bins right", which put the centre
     * and the reverb send in the left ear only and LFE in the right only --
     * in Ninja Gaiden Black's gameplay that is most of the sound effects
     * (measured: bins 0,1,2,3,6,7,10). */

    int off = (d->ep_frame_div % 8) * NUM_SAMPLES_PER_FRAME;

    if (d->monitor.point != MCPX_APU_DEBUG_MON_VP) {
        for (int i = 0; i < NUM_SAMPLES_PER_FRAME; i++) {
            float left, right;
            if (mcpx_apu_mixdown_all()) {
                const float c3 = 0.7071f, c6 = 0.5f, wet = 0.4f;
                float centre = c3 * mixbins[2][i] + c6 * mixbins[3][i];
                float send = mixbins[10][i] * 0.015f;
                left  = mixbins[0][i] + mixbins[4][i] + mixbins[6][i] +
                        mixbins[8][i] + centre;
                right = mixbins[1][i] + mixbins[5][i] + mixbins[7][i] +
                        mixbins[9][i] + centre;
                left  += wet * reverb_side(0, send);
                right += wet * reverb_side(1, send);
            } else {
                left = mixbins[0][i];
                right = mixbins[1][i];
            }
            /* Clamp to [-1, 1] range */
            if (left > 1.0f) left = 1.0f;
            if (left < -1.0f) left = -1.0f;
            if (right > 1.0f) right = 1.0f;
            if (right < -1.0f) right = -1.0f;

            /* Each of the 8 sub-frames writes its own 32-sample slice. */
            d->monitor.frame_buf[off + i][0] = (int16_t)(left * 32767.0f);
            d->monitor.frame_buf[off + i][1] = (int16_t)(right * 32767.0f);
        }
    }

    g_dbg.gp.cycles = 0;
    g_dbg.ep.cycles = 0;
}
