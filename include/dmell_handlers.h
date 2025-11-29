#ifndef DMELL_HANDLERS_H
#define DMELL_HANDLERS_H

#include "dmell_cmd.h"

extern int dmell_handler_echo( int argc, char** argv );
extern int dmell_handler_set( int argc, char** argv );
extern int dmell_handler_unset( int argc, char** argv );
extern int dmell_handler_export( int argc, char** argv );
extern int dmell_handler_cd( int argc, char** argv );
extern int dmell_handler_exit( int argc, char** argv );

extern int dmell_register_handlers( void );

#endif // DMELL_HANDLERS_H