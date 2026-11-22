add_library(hal STATIC
    src/adc.c src/buzzer_gpio.c src/led.c src/lcd.c src/joystick.c
    src/motion_sensor.c src/audio.c src/rotary_encoder.c)
target_include_directories(hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/hal)
target_link_libraries(hal PUBLIC ${GPIOD_LIBRARY})

find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)
target_link_libraries(hal PUBLIC PkgConfig::ALSA Threads::Threads m)
