# =====================================================================
#               DMOD ymodem Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

set(YMODEM_MODULE_NAME ymodem)

set(DMOD_STACK_SIZE 3072)

dmod_add_executable(${YMODEM_MODULE_NAME} ${DMOD_MODULE_VERSION}
    ymodem.c
)

# dmosi: the thread sending 'C' until the sender starts
dmod_link_modules(${YMODEM_MODULE_NAME} dmosi)
