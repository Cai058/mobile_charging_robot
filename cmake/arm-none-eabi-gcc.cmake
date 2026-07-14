set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(ARM_GCC_HINTS
    "$ENV{ARM_GCC_PATH}"
    "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin"
    "C:/Program Files/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin"
)

find_program(ARM_GCC arm-none-eabi-gcc HINTS ${ARM_GCC_HINTS} REQUIRED)
get_filename_component(ARM_GCC_BIN_DIR "${ARM_GCC}" DIRECTORY)

set(CMAKE_C_COMPILER "${ARM_GCC}" CACHE FILEPATH "ARM GCC compiler")
set(CMAKE_ASM_COMPILER "${ARM_GCC}" CACHE FILEPATH "ARM GCC assembler")
set(CMAKE_AR "${ARM_GCC_BIN_DIR}/arm-none-eabi-ar.exe" CACHE FILEPATH "ARM archiver")
set(CMAKE_RANLIB "${ARM_GCC_BIN_DIR}/arm-none-eabi-ranlib.exe" CACHE FILEPATH "ARM ranlib")
set(CMAKE_OBJCOPY "${ARM_GCC_BIN_DIR}/arm-none-eabi-objcopy.exe" CACHE FILEPATH "ARM objcopy")
set(CMAKE_SIZE "${ARM_GCC_BIN_DIR}/arm-none-eabi-size.exe" CACHE FILEPATH "ARM size")

set(CMAKE_FIND_ROOT_PATH "${ARM_GCC_BIN_DIR}/..")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
