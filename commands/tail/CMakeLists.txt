# =====================================================================
#               DMOD tail Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(TAIL_MODULE_NAME tail)

# Stack size for the tail module
set(DMOD_STACK_SIZE 512)

# Create the module executable
dmod_add_executable(${TAIL_MODULE_NAME} ${DMOD_MODULE_VERSION}
    tail.c
)
