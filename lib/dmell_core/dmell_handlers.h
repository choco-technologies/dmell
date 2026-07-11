#ifndef DMELL_HANDLERS_H
#define DMELL_HANDLERS_H

/**
 * @brief Registers all built-in command handlers into dmell_cmd's registry.
 *
 * Called once from dmell_core's dmod_init().
 *
 * @return int 0 on success
 */
extern int dmell_register_handlers( void );

#endif // DMELL_HANDLERS_H
