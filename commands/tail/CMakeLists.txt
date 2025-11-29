# =====================================================================
#               DMOD tail Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(TAIL_MODULE_NAME tail)

# Create the module executable
dmod_add_executable(${TAIL_MODULE_NAME} ${DMOD_MODULE_VERSION}
    tail.c
)
