# =====================================================================
#               DMOD cp Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(CP_MODULE_NAME cp)

# Create the module executable
dmod_add_executable(${CP_MODULE_NAME} ${DMOD_MODULE_VERSION}
    cp.c
)
