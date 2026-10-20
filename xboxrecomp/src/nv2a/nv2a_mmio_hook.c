/*
 * NV2A MMIO Hook - VEH instruction decoder for GPU register access
 *
 * Decodes MOV/TEST/CMP instructions that access NV2A MMIO registers,
 * routes them through the NV2A register handlers, and advances RIP/PC.
 */

#include "nv2a_mmio_hook.h"
#include "nv2a_state.h"
#include <stdio.h>

/* NV2A MMIO base in Xbox VA space */
#define NV2A_MMIO_BASE  0xFD000000u
#define NV2A_MMIO_SIZE  0x01000000u  /* 16MB */
#define NV2A_VRAM_BASE  0xF0000000u
#define NV2A_VRAM_SIZE  (64 * 1024 * 1024)  /* 64MB Xbox VRAM */
#define NV2A_RAMIN_SIZE (1 * 1024 * 1024)   /* 1MB RAMIN */

static ptrdiff_t g_mem_offset = 0;
static uint8_t *g_nv2a_vram = NULL;

/* Statistics */
static int g_mmio_read_count = 0;
static int g_mmio_write_count = 0;
static int g_mmio_decode_fail = 0;

/* Global APU state pointer is declared in apu.h and referenced from main.c
 * (regardless of whether the MMIO hook is active). Keep its definition
 * outside the Win32 guard. */

#if RECOMP_HAS_VEH
#if defined(_WIN32)
#include <windows.h>
#else
#include "platform/win32_compat.h"
#endif

#include "platform/mmio_decode.h"

static uint64_t nv2a_rd(void *dev, uint32_t off, int size)
{
    g_mmio_read_count++;
    return nv2a_mmio_read((NV2AState *)dev, off, size);
}

static void nv2a_wr(void *dev, uint32_t off, uint64_t val, int size)
{
    g_mmio_write_count++;
    nv2a_mmio_write((NV2AState *)dev, off, val, size);
}

static bool decode_and_handle(PCONTEXT ctx, uint32_t mmio_offset, int is_write)
{
    (void)is_write;
    NV2AState *nv2a = nv2a_get_state();
    if (!nv2a) return false;

    if (mmio_emulate(ctx, mmio_offset, nv2a, nv2a_rd, nv2a_wr))
        return true;

    g_mmio_decode_fail++;
    if (g_mmio_decode_fail <= 20) {
#if defined(__aarch64__) || defined(_M_ARM64)
        const uint8_t *ip = (const uint8_t *)ctx->Pc;
        void *cur_ip = (void *)(uintptr_t)ctx->Pc;
#else
        const uint8_t *ip = (const uint8_t *)ctx->Rip;
        void *cur_ip = (void *)(uintptr_t)ctx->Rip;
#endif
        fprintf(stderr, "[NV2A] MMIO decode fail at PC/RIP=%p offset=0x%X: %02X %02X %02X %02X %02X %02X\n",
                cur_ip, mmio_offset,
                ip[0], ip[1], ip[2], ip[3], ip[4], ip[5]);
        fflush(stderr);
    }
    return false;
}

/* ============================================================
 * Public API
 * ============================================================ */

void nv2a_hook_init(ptrdiff_t xbox_mem_offset)
{
    g_mem_offset = xbox_mem_offset;

    /* Allocate NV2A VRAM (64MB) */
    g_nv2a_vram = (uint8_t *)VirtualAlloc(NULL, NV2A_VRAM_SIZE + NV2A_RAMIN_SIZE,
                                            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!g_nv2a_vram) {
        fprintf(stderr, "[NV2A] Failed to allocate %u MB for VRAM!\n",
                (NV2A_VRAM_SIZE + NV2A_RAMIN_SIZE) / (1024*1024));
        return;
    }

    uint8_t *ramin_ptr = g_nv2a_vram + NV2A_VRAM_SIZE;

    /* Initialize NV2A state machine */
    nv2a_init_standalone(g_nv2a_vram, NV2A_VRAM_SIZE,
                         ramin_ptr, NV2A_RAMIN_SIZE);

    fprintf(stderr, "[NV2A] MMIO hook initialized: VRAM=%p RAMIN=%p\n",
            (void*)g_nv2a_vram, (void*)ramin_ptr);
}

bool nv2a_hook_handle_mmio(PCONTEXT ctx, uintptr_t fault_addr,
                           uint32_t fault_xbox_va, int is_write)
{
    /* Compute MMIO offset within NV2A register space */
    uint32_t mmio_offset = fault_xbox_va - NV2A_MMIO_BASE;

    return decode_and_handle(ctx, mmio_offset, is_write);
}

bool nv2a_hook_handle_vram(uintptr_t fault_addr, uint32_t fault_xbox_va)
{
    /* For VRAM range (0xF0000000-0xF3FFFFFF), allocate pages as before.
     * In the future, we can map these to NV2A VRAM for push buffer DMA.
     * For now, just allocate writable pages. */
    uintptr_t alloc_base = fault_addr & ~(uintptr_t)0xFFFF;
    LPVOID result = VirtualAlloc((LPVOID)alloc_base, 0x10000,
                                 MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!result) {
        result = VirtualAlloc((LPVOID)alloc_base, 0x10000,
                              MEM_COMMIT, PAGE_READWRITE);
    }
    if (result) {
        memset(result, 0, 0x10000);
        return true;
    }
    return false;
}

#else /* !RECOMP_HAS_VEH */

void nv2a_hook_init(ptrdiff_t xbox_mem_offset)
{ (void)xbox_mem_offset; }

bool nv2a_hook_handle_mmio(PCONTEXT ctx, uintptr_t fault_addr,
                           uint32_t fault_xbox_va, int is_write)
{ (void)ctx; (void)fault_addr; (void)fault_xbox_va; (void)is_write; return false; }

bool nv2a_hook_handle_vram(uintptr_t fault_addr, uint32_t fault_xbox_va)
{ (void)fault_addr; (void)fault_xbox_va; return false; }

#endif /* RECOMP_HAS_VEH */
