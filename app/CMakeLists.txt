# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(motion ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(motion LINK_PRIVATE hal)

# Copy executable to final location (change `hello_world` to project name as needed)
add_custom_command(TARGET motion POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:motion>"
     "~/ensc351/public/myApps/motion" 
  COMMENT "Copying ARM executable to public NFS directory")
