/*
 * autosave.h -- Dedicated autosave subsystem for Ninja Gaiden Black (recomp).
 *
 * Automatically saves the player's live state (0x572F40, 0x17460 bytes)
 * upon door interactions and scene/room transitions, shows a sleek UI banner,
 * and allows resuming from this checkpoint upon death (Game Over -> Continue)
 * or through the Load Game menu.
 */
#ifndef NGB_AUTOSAVE_H
#define NGB_AUTOSAVE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the autosave subsystem (paths, initial state). */
void ngb_autosave_init(void);

/* Called every frame on the game thread from ngb_frame_tick(). */
void ngb_autosave_tick(void);

/* Returns true if the on-screen autosave banner should currently be displayed. */
bool ngb_autosave_banner_active(void);

/* Triggers an autosave immediately. Returns true on success. */
bool ngb_autosave_save(void);

/* Loads the autosave state into live game memory. Returns true on success. */
bool ngb_autosave_load(void);

/* Quick loads the autosave state and reloads the scene. Returns true on success. */
bool ngb_autosave_quick_load(void);

/* Returns the filesystem path to save_auto.dat. */
const char *ngb_autosave_get_path(void);

/* Renders the autosave banner ("[ AUTOSAVED ]" or "[ AUTOSAVE LOADED ]") into a BGRA/RGBA pixel buffer
 * using the DX12-style 5x7 bitmap font in a translucent dark charcoal box (0xC0181818).
 * Returns true if the autosave banner is currently active and rendered, false otherwise. */
bool ngb_autosave_get_bitmap(uint32_t *px, uint32_t max_w, uint32_t max_h,
                             uint32_t *ow, uint32_t *oh,
                             uint32_t screen_w, uint32_t screen_h);

/* Returns the current banner text ("[ AUTOSAVED ]" or "[ AUTOSAVE LOADED ]" or "[ NO AUTOSAVE FOUND ]"). */
const char *ngb_autosave_banner_text(void);

#ifdef __cplusplus
}
#endif

#endif /* NGB_AUTOSAVE_H */
