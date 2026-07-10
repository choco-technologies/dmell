#ifndef DMELL_HANDLERS_H
#define DMELL_HANDLERS_H

#include "dmod.h"
#include "dmell_handlers_defs.h"

/**
 * @brief Makes sure the dmell_handlers module is loaded.
 *
 * The built-in commands (echo, set, cd, ...) are registered into dmell_cmd's
 * shared registry from THIS module's own dmod_init(), which the dmod loader
 * calls exactly once, the first time dmell_handlers is loaded - not from here.
 *
 * This function's body does nothing; its only purpose is to give dmell (or
 * whichever module is meant to have the built-ins available) an actual call
 * into dmell_handlers, so that a "requires dmell_handlers" dependency gets
 * recorded in its .dmd (dmod only auto-detects a required module from a real
 * call to one of its exported APIs). Without this call dmell_handlers would
 * never be loaded at all, and its dmod_init() would never run.
 *
 * Safe and cheap to call from every dmell process; the registration itself
 * only ever happens once regardless of how many times this is called.
 */
dmod_dmell_handlers_global_api( 1.0, int, dmell_register_handlers, (void) );

#endif // DMELL_HANDLERS_H
