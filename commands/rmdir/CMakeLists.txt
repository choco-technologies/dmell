# =====================================================================
#               DMOD rmdir Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(RMDIR_MODULE_NAME rmdir)

# Create the module executable
dmod_add_executable(${RMDIR_MODULE_NAME} ${DMOD_MODULE_VERSION}
    rmdir.c
)
