# Find app headers
include_directories(include)

# Existing application sources (exclude study_menu.c)
file(GLOB MY_SOURCES "src/*.c")
list(REMOVE_ITEM MY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/study_menu.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/keypad.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/session.c"
)

add_executable(study_session ${MY_SOURCES})
target_link_libraries(study_session PRIVATE hal)

add_executable(study_menu_test
    src/study_menu.c
src/keypad.c
src/session.c
)

target_link_libraries(study_menu_test PRIVATE hal gpiod)

add_executable(timer_test
    src/timer_test.c
)

target_link_libraries(timer_test PRIVATE hal)

add_executable(keypad_test src/keypad_test.c)
target_link_libraries(keypad_test PRIVATE gpiod)

# Copy existing application to public NFS directory
add_custom_command(TARGET study_session POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy
        "$<TARGET_FILE:study_session>"
        "$ENV{HOME}/ensc351/public/myApps/study_session"
    COMMENT "Copying study_session to public NFS directory"
)
