# =====================================================================
#               DMOD cat Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(CAT_MODULE_NAME cat)

# Create the module executable
dmod_add_executable(${CAT_MODULE_NAME} ${DMOD_MODULE_VERSION}
    cat.c
)
