add_library(hal STATIC
    src/adc.c src/buzzer_gpio.c src/led.c src/lcd.c src/joystick.c)
target_include_directories(hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/hal)
target_link_libraries(hal PUBLIC ${GPIOD_LIBRARY})
