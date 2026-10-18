cmake_minimum_required(VERSION 3.18)

set(DMOD_STACK_SIZE 1024)
dmod_add_executable(reboot ${DMOD_MODULE_VERSION} reboot.c)
