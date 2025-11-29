# =====================================================================
#               DMOD find Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(FIND_MODULE_NAME find)

# Create the module executable
dmod_add_executable(${FIND_MODULE_NAME} ${DMOD_MODULE_VERSION}
    find.c
)
