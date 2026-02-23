# =====================================================================
#               DMOD ps Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(PS_MODULE_NAME ps)

# Create the module executable
dmod_add_executable(${PS_MODULE_NAME} ${DMOD_MODULE_VERSION}
    ps.c
)

# Add dmosi headers so that dmosi.h is resolvable at compile time
target_include_directories(${PS_MODULE_NAME} PRIVATE
    ${dmosi_SOURCE_DIR}/include
)
