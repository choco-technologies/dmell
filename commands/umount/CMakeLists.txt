# =====================================================================
#               DMOD umount Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(UMOUNT_MODULE_NAME umount)

# Stack size for the umount module
set(DMOD_STACK_SIZE 1024)

# Create the module executable
dmod_add_executable(${UMOUNT_MODULE_NAME} ${DMOD_MODULE_VERSION}
    umount.c
)
