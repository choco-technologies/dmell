# =====================================================================
#               DMOD rmdir Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(RMDIR_MODULE_NAME rmdir)

# Stack size for the rmdir module
set(DMOD_STACK_SIZE 512)

# Create the module executable
dmod_add_executable(${RMDIR_MODULE_NAME} ${DMOD_MODULE_VERSION}
    rmdir.c
)
