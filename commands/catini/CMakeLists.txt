# =====================================================================
#               DMOD catini Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(CATINI_MODULE_NAME catini)

# Stack size for the catini module
set(DMOD_STACK_SIZE 512)

# Create the module executable
dmod_add_executable(${CATINI_MODULE_NAME} ${DMOD_MODULE_VERSION}
    catini.c
)
