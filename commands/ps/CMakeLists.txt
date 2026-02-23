# =====================================================================
#               DMOD ps Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(PS_MODULE_NAME ps)

# Stack size for the ps module
set(DMOD_STACK_SIZE 2048)

# Create the module executable
dmod_add_executable(${PS_MODULE_NAME} ${DMOD_MODULE_VERSION}
    ps.c
)

# Link against dmosi_if to get its include directories and API declarations
target_link_libraries(${PS_MODULE_NAME} PRIVATE dmosi_if)
