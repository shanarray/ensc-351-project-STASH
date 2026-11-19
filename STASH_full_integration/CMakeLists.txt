cmake_minimum_required(VERSION 3.18)
project(STASH LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic -Werror)
find_library(GPIOD_LIBRARY gpiod REQUIRED)
find_package(Threads REQUIRED)

option(STASH_ENABLE_ROTARY_ENCODER "Enable music rotary on GPIO22/17/15" ON)
set(STASH_ENCODER_CHIP_A "/dev/gpiochip1" CACHE STRING "Encoder A gpiochip")
set(STASH_ENCODER_CHIP_B_SW "/dev/gpiochip2" CACHE STRING "Encoder B and switch gpiochip")
set(STASH_ENCODER_A "41" CACHE STRING "Encoder A offset (tested GPIO22)")
set(STASH_ENCODER_B "8" CACHE STRING "Encoder B offset (tested GPIO17)")
set(STASH_ENCODER_SW "13" CACHE STRING "Encoder switch offset (tested GPIO15)")
if(STASH_ENABLE_ROTARY_ENCODER)
    foreach(pin A B SW)
        if(NOT STASH_ENCODER_${pin} MATCHES "^[0-9]+$")
            message(FATAL_ERROR "Encoder ${pin} offset must be a decimal integer")
        endif()
    endforeach()
    # Keypad now uses named GPIO12 for row 2; other keypad offsets remain unchanged.
    foreach(pin A B SW)
        if(pin STREQUAL "A")
            set(chip "${STASH_ENCODER_CHIP_A}")
        else()
            set(chip "${STASH_ENCODER_CHIP_B_SW}")
        endif()
        set(offset "${STASH_ENCODER_${pin}}")
        if((chip STREQUAL "/dev/gpiochip1" AND offset MATCHES "^(38|36)$") OR
           (chip STREQUAL "/dev/gpiochip2" AND offset MATCHES "^(15|17|7)$") OR
           (chip STREQUAL "/dev/gpiochip0" AND offset MATCHES "^(7|10)$"))
            message(FATAL_ERROR "Encoder ${pin} uses ${chip} offset ${offset}, reserved by keypad.")
        endif()
    endforeach()
    if(STASH_ENCODER_CHIP_A STREQUAL STASH_ENCODER_CHIP_B_SW)
        message(FATAL_ERROR "Current rotary driver requires encoder A on a separate gpiochip from B/SW")
    endif()
    if(STASH_ENCODER_B STREQUAL STASH_ENCODER_SW)
        message(FATAL_ERROR "Encoder B and switch cannot share an offset")
    endif()
endif()

add_subdirectory(hal)
add_subdirectory(app)

include(CTest)
if(BUILD_TESTING)
    add_executable(timer_test tests/timer_test.c app/src/timer.c)
    target_include_directories(timer_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/app/include)
    add_test(NAME timer_logic COMMAND timer_test)
endif()

target_compile_definitions(hal PRIVATE
    STASH_ENCODER_CHIP_A="${STASH_ENCODER_CHIP_A}"
    STASH_ENCODER_CHIP_B_SW="${STASH_ENCODER_CHIP_B_SW}"
    STASH_ENCODER_A=${STASH_ENCODER_A}
    STASH_ENCODER_B=${STASH_ENCODER_B}
    STASH_ENCODER_SW=${STASH_ENCODER_SW})
target_compile_definitions(study_session PRIVATE STASH_ENABLE_ROTARY_ENCODER=$<BOOL:${STASH_ENABLE_ROTARY_ENCODER}>)
