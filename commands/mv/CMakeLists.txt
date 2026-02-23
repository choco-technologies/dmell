# =====================================================================
#               DMOD mv Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(MV_MODULE_NAME mv)

# Stack size for the mv module
set(DMOD_STACK_SIZE 512)

# Create the module executable
dmod_add_executable(${MV_MODULE_NAME} ${DMOD_MODULE_VERSION}
    mv.c
)
