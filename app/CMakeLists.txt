# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(study_session ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(study_session LINK_PRIVATE hal)

# Copy executable to final location (change `hello_world` to project name as needed)
add_custom_command(TARGET study_session POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:study_session>"
     "~/ensc351/public/myApps/study_session" 
  COMMENT "Copying ARM executable to public NFS directory")
