# CROSS COMPILER SETTING
SET(CMAKE_SYSTEM_NAME Generic)
CMAKE_MINIMUM_REQUIRED (VERSION 3.10.0)

# THE VERSION NUMBER
SET (Tutorial_VERSION_MAJOR 1)
SET (Tutorial_VERSION_MINOR 0)

# ENABLE ASM
ENABLE_LANGUAGE(ASM)

SET(CMAKE_STATIC_LIBRARY_PREFIX)
SET(CMAKE_STATIC_LIBRARY_SUFFIX)

SET(CMAKE_EXECUTABLE_LIBRARY_PREFIX)
SET(CMAKE_EXECUTABLE_LIBRARY_SUFFIX)

# CURRENT DIRECTORY
SET(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR})

SET(EXECUTABLE_OUTPUT_PATH ${ProjDirPath}/${CMAKE_BUILD_TYPE})
SET(LIBRARY_OUTPUT_PATH ${ProjDirPath}/${CMAKE_BUILD_TYPE})


project(freertos_event)

set(MCUX_SDK_PROJECT_NAME freertos_event.elf)

include(${ProjDirPath}/flags.cmake)

include(${ProjDirPath}/config.cmake)

add_executable(${MCUX_SDK_PROJECT_NAME} 
"${ProjDirPath}/../freertos_event.c"
"${ProjDirPath}/../pin_mux.c"
"${ProjDirPath}/../pin_mux.h"
"${ProjDirPath}/../FreeRTOSConfig.h"
"${ProjDirPath}/../board.c"
"${ProjDirPath}/../board.h"
"${ProjDirPath}/../clock_config.c"
"${ProjDirPath}/../clock_config.h"
"${ProjDirPath}/../fsl_iomuxc.h"
"${ProjDirPath}/../empty_rsc_table.c"
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${ProjDirPath}/..
)

set(CMAKE_MODULE_PATH
    ${ProjDirPath}/../../../../../devices/MIMX8MM6/drivers
    ${ProjDirPath}/../../../../../rtos/freertos/freertos_kernel
    ${ProjDirPath}/../../../../../devices/MIMX8MM6
    ${ProjDirPath}/../../../../../devices/MIMX8MM6/utilities
    ${ProjDirPath}/../../../../../components/uart
    ${ProjDirPath}/../../../../../components/serial_manager
    ${ProjDirPath}/../../../../../components/lists
    ${ProjDirPath}/../../../../../CMSIS/Core/Include
)

# include modules
include(driver_clock_MIMX8MM6)

include(middleware_freertos-kernel_heap_4_MIMX8MM6)

include(driver_common_MIMX8MM6)

include(driver_rdc_MIMX8MM6)

include(device_MIMX8MM6_CMSIS_MIMX8MM6)

include(utility_debug_console_MIMX8MM6)

include(component_iuart_adapter_MIMX8MM6)

include(component_serial_manager_MIMX8MM6)

include(component_lists_MIMX8MM6)

include(component_serial_manager_uart_MIMX8MM6)

include(driver_iuart_MIMX8MM6)

include(device_MIMX8MM6_startup_MIMX8MM6)

include(utility_assert_MIMX8MM6)

include(middleware_freertos-kernel_MIMX8MM6)

include(middleware_freertos-kernel_extension_MIMX8MM6)

include(utilities_misc_utilities_MIMX8MM6)

include(CMSIS_Include_core_cm_MIMX8MM6)

include(device_MIMX8MM6_system_MIMX8MM6)


TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--start-group)

target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE m)

target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE c)

target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE gcc)

target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE nosys)

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--end-group)

ADD_CUSTOM_COMMAND(TARGET ${MCUX_SDK_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_OBJCOPY}
-Obinary ${EXECUTABLE_OUTPUT_PATH}/${MCUX_SDK_PROJECT_NAME} ${EXECUTABLE_OUTPUT_PATH}/freertos_event.bin)

