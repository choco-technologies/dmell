# =====================================================================
#               DMOD module Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(MODULE_MODULE_NAME module)

# Create the module executable
dmod_add_executable(${MODULE_MODULE_NAME} ${DMOD_MODULE_VERSION}
    module.c
)
