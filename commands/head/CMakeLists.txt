# =====================================================================
#               DMOD head Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(HEAD_MODULE_NAME head)

# Stack size for the head module
set(DMOD_STACK_SIZE 512)

# Create the module executable
dmod_add_executable(${HEAD_MODULE_NAME} ${DMOD_MODULE_VERSION}
    head.c
)
