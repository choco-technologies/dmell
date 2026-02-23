# =====================================================================
#               DMOD which Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(WHICH_MODULE_NAME which)

# Stack size for the which module (larger due to 512-byte stack-allocated path buffer)
set(DMOD_STACK_SIZE 1024)

# Create the module executable
dmod_add_executable(${WHICH_MODULE_NAME} ${DMOD_MODULE_VERSION}
    which.c
)
