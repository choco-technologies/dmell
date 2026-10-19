cmake_minimum_required(VERSION 3.18)

set(DMOD_STACK_SIZE 1024)
dmod_add_executable(tree ${DMOD_MODULE_VERSION} tree.c)
dmod_link_modules(tree dmlist)
