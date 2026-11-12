add_executable(study_session
    src/main.c src/timer.c src/input_handler.c src/feedback.c
    src/feedback_system.c src/keypad.c src/session.c)
target_include_directories(study_session PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(study_session PRIVATE hal)
