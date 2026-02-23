# =====================================================================
#               DMOD find Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(FIND_MODULE_NAME find)

# Stack size for the find module (larger due to recursive directory traversal)
set(DMOD_STACK_SIZE 1024)

# Create the module executable
dmod_add_executable(${FIND_MODULE_NAME} ${DMOD_MODULE_VERSION}
    find.c
)
