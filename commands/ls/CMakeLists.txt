# =====================================================================
#               DMOD ls Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(LS_MODULE_NAME ls)

# Stack size for the ls module
set(DMOD_STACK_SIZE 512)

# Create the module executable
dmod_add_executable(${LS_MODULE_NAME} ${DMOD_MODULE_VERSION}
    ls.c
)
