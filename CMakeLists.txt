cmake_minimum_required(VERSION 3.22)

project(mobile_charging_robot LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

set(CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

set(PROJECT_SOURCES
    startup/startup_stm32f427xx.s
    platform/syscalls.c

    User/Src/main.c
    # 由 cubemx/sync-generated.ps1 同步 CubeMX 生成源码。
    User/Src/can.c
    User/Src/gpio.c
    User/Src/stm32f4xx_hal_msp.c
    User/Src/stm32f4xx_it.c
    User/Src/usart_callback.c
    User/Src/Robot_Config.c
    User/Src/system_stm32f4xx.c

    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_can.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c

    BSP/src/bsp_can.c
    BSP/src/bsp_key.c
    BSP/src/bsp_L298N.c
    BSP/src/bsp_led.c
    BSP/src/bsp_limitSwitch.c
    BSP/src/bsp_photogate.c
    BSP/src/bsp_ultrawave.c
    BSP/src/pid.c
    BSP/src/time.c
    BSP/src/bsp_debug_usart.c
    BSP/src/cJSON.c
    BSP/src/bsp_rgb.c
    BSP/src/bsp_rc.c
    BSP/src/bsp_485_battery.c
    BSP/src/bsp_485_rfid.c
    BSP/src/bsp_485_server.c

    Controller/src/ControllerDummy.c
    Controller/src/Chassiscontroller.c
    Controller/src/ControllerRC.c

    Entity/src/Battery.c
    Entity/src/RC.c
    Entity/src/RFID.c
    Entity/src/Server.c

    math/src/first_order_filter.c
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    User
    User/Inc
    User/Src
    BSP
    BSP/inc
    BSP/src
    Controller
    Controller/inc
    Entity
    Entity/inc
    Entity/src
    math
    math/inc
    math/src
    Drivers/STM32F4xx_HAL_Driver/Inc
    Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    Drivers/CMSIS/Device/ST/STM32F4xx/Include
    Drivers/CMSIS/Include
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    USE_HAL_DRIVER
    STM32F427xx
)

target_compile_options(${PROJECT_NAME} PRIVATE
    ${CPU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -fcommon
    -Wall
    -Wextra
    -Wno-unused-parameter
    $<$<CONFIG:Debug>:-Og>
    $<$<CONFIG:Debug>:-g3>
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:Release>:-g0>
)

set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker/STM32F427IIHx_FLASH.ld")

target_link_options(${PROJECT_NAME} PRIVATE
    ${CPU_FLAGS}
    -T${LINKER_SCRIPT}
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,--no-warn-rwx-segments
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map
    -Wl,-u,_printf_float
    -Wl,-u,_scanf_float
)

target_link_libraries(${PROJECT_NAME} PRIVATE m)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:${PROJECT_NAME}>
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:${PROJECT_NAME}>
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Generating HEX/BIN images and reporting firmware size"
)
