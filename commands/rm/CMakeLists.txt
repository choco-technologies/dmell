# =====================================================================
#               DMOD rm Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(RM_MODULE_NAME rm)

# Stack size for the rm module
set(DMOD_STACK_SIZE 512)

# Create the module executable
dmod_add_executable(${RM_MODULE_NAME} ${DMOD_MODULE_VERSION}
    rm.c
)
