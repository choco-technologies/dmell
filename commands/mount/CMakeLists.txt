# =====================================================================
#               DMOD mount Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(MOUNT_MODULE_NAME mount)

# Mount runs the filesystem loader, initialization and block-driver calls on
# its own stack. 1 KiB overflows during a cold littlefs/QUADSPI mount on
# Cortex-M7; reserve space for that call chain, not just argument parsing.
# Keep the requirement within dmell's 4 KiB stack: synchronous shell commands
# are checked against the caller's remaining stack before they are run.
set(DMOD_STACK_SIZE 3072)

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
