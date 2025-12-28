# DMOD API Patch for module command

This patch adds the `Dmod_ReadRequiredModules` function to the DMOD module API,
making it accessible from modules (not just from system code).

## Required Change

Add the following line to `inc/dmod.h` in the DMOD repository, after line 116
(after `_RunModule` and before `_IsFunctionConnected`):

```c
DMOD_BUILTIN_API( Dmod, 1.0, bool       , _ReadRequiredModules, (const char* Path, Dmod_RequiredModule_t* outRequiredModules, size_t MaxModules) );
```

## Full Context

```c
DMOD_BUILTIN_API( Dmod, 1.0, bool       , _DisableModule, (const char* ModuleName, bool Force ) );
DMOD_BUILTIN_API( Dmod, 1.0, int        , _RunModule, (const char* Module, int argc, char *argv[]) );
DMOD_BUILTIN_API( Dmod, 1.0, bool       , _ReadRequiredModules, (const char* Path, Dmod_RequiredModule_t* outRequiredModules, size_t MaxModules) );
DMOD_BUILTIN_API( Dmod, 1.0, bool       , _IsFunctionConnected, (void* FunctionPointer) );
```

## Purpose

This change makes the `Dmod_ReadRequiredModules` function available to DMOD modules,
allowing the `module info` command to display the list of required modules for any
given module.

Previously, this function was only available in `dmod_system.h` (system-side API),
but not accessible to modules themselves.

## Implementation Note

The actual implementation of `Dmod_ReadRequiredModules` already exists in the DMOD
system code. This patch only exposes it through the module API using the 
`DMOD_BUILTIN_API` macro.
