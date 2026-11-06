# CMakeList.txt for HAL
#   Build a library (`hal`) which exposes the header files as "hal/*.h"
#   Use header as: #include "hal/button.h"

include_directories(hal/include)
file(GLOB MY_SOURCES "src/*.c")

add_library(hal STATIC ${MY_SOURCES})

target_include_directories(hal PUBLIC include)

# added for gpio
target_link_libraries(hal PRIVATE gpiod)

# ALSA audio support
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED alsa)

target_include_directories(hal PRIVATE ${ALSA_INCLUDE_DIRS})

target_link_libraries(hal PUBLIC ${ALSA_LIBRARIES})

