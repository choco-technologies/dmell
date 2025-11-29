# =====================================================================
#               DMOD mv Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(MV_MODULE_NAME mv)

# Create the module executable
dmod_add_executable(${MV_MODULE_NAME} ${DMOD_MODULE_VERSION}
    mv.c
)
