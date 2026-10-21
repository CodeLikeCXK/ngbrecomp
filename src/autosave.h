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

/* Returns the filesystem path to save_auto.dat. */
const char *ngb_autosave_get_path(void);

#ifdef __cplusplus
}
#endif

#endif /* NGB_AUTOSAVE_H */
