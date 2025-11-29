# =====================================================================
#               DMOD touch Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(TOUCH_MODULE_NAME touch)

# Create the module executable
dmod_add_executable(${TOUCH_MODULE_NAME} ${DMOD_MODULE_VERSION}
    touch.c
)
