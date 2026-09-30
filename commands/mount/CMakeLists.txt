# =====================================================================
#               DMOD mount Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(MOUNT_MODULE_NAME mount)

# Stack size for the mount module
set(DMOD_STACK_SIZE 1024)

# Create the module executable
dmod_add_executable(${MOUNT_MODULE_NAME} ${DMOD_MODULE_VERSION}
    mount.c
)

# dmvfs: dmvfs_mount_fs() - dmvfs is built into the firmware, only its
# header is needed (dmvfs.h includes dmfsi.h, hence dmfsi)
dmod_link_modules(${MOUNT_MODULE_NAME}
    dmvfs
    dmfsi
)
