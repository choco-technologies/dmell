# =====================================================================
#               DMOD which Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(WHICH_MODULE_NAME which)

# Create the module executable
dmod_add_executable(${WHICH_MODULE_NAME} ${DMOD_MODULE_VERSION}
    which.c
)
