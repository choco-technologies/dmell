# =====================================================================
#               DMOD grep Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(GREP_MODULE_NAME grep)

# Create the module executable
dmod_add_executable(${GREP_MODULE_NAME} ${DMOD_MODULE_VERSION}
    grep.c
)
