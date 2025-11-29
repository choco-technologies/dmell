# =====================================================================
#               DMOD ls Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(LS_MODULE_NAME ls)

# Create the module executable
dmod_add_executable(${LS_MODULE_NAME} ${DMOD_MODULE_VERSION}
    ls.c
)
