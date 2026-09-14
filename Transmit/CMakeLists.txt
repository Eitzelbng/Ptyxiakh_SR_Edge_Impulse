# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Transmit C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
file(GLOB_RECURSE EI_SOURCES 
    "Libraries/Edge_Impulse/*.cpp"
    "Libraries/Edge_Impulse/*.c"
    "Libraries/Edge_Impulse/*.cc"
)


add_executable(Transmit Transmit.cpp 
Libraries/Edge_Impulse/edge-impulse-sdk/classifier/ei_run_classifier_c.cpp
Libraries/sx1276/LoRa-RP2040.cpp
Libraries/sx1276/Print.cpp
Libraries/Edge_Impulse/tflite-model/tflite_learn_794595_164.cpp
${EI_SOURCES}
)

pico_set_program_name(Transmit "Transmit")
pico_set_program_version(Transmit "0.1")

# Generate PIO header
pico_generate_pio_header(Transmit ${CMAKE_CURRENT_LIST_DIR}/i2s.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Transmit 0)
pico_enable_stdio_usb(Transmit 1)

# Add the standard library to the build
target_link_libraries(Transmit
        pico_stdlib
        hardware_pio
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_dma
        pico_multicore
        )

# Add the standard include files to the build
target_include_directories(Transmit PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/Libraries
        ${CMAKE_CURRENT_LIST_DIR}/Libraries/Edge_Impulse
        ${CMAKE_CURRENT_LIST_DIR}/Libraries/Edge_Impulse/edge-impulse-sdk
        ${CMAKE_CURRENT_LIST_DIR}/Libraries/Edge_Impulse/model-parameters
        ${CMAKE_CURRENT_LIST_DIR}/Libraries/Edge_Impulse/tflite-model
)


pico_add_extra_outputs(Transmit)

target_compile_definitions(Transmit PRIVATE
    EI_CLASSIFIER_TFLITE_ENABLE_CMSIS_NN=1
    PICO_RP2350=1
)