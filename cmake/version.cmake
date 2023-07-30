# Firmware version define(1.0-255.255)
set(FW_VER_MAJOR 1)
set(FW_VER_MINOR 0)
set(FW_VER_PATCH 0)

# Get git tag information
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND git describe --long --dirty=+dev --always
        OUTPUT_VARIABLE RAW_GIT_DESCRIBE
        RESULT_VARIABLE STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

# Check if get git information
if(NOT RAW_GIT_DESCRIBE)
    set(RAW_GIT_DESCRIBE "NOGIT")
endif()

# Get current time and hostname
string(TIMESTAMP BUILD_DATE "%Y/%m/%d %H:%M" )
cmake_host_system_information(RESULT BUILD_MACHINE QUERY HOSTNAME)

# Add version info to global definition
add_compile_definitions(FW_VER_MAJOR=${FW_VER_MAJOR})
add_compile_definitions(FW_VER_MINOR=${FW_VER_MINOR})
add_compile_definitions(FW_VER_PATCH=${FW_VER_PATCH})
add_compile_definitions(VERSION_BUILD_MACHINE="${BUILD_MACHINE}")
add_compile_definitions(VERSION_BUILD_INFO="${RAW_GIT_DESCRIBE}")
add_compile_definitions(VERSION_BUILD_DATE="${BUILD_DATE}")

# Print to build terminal
message("")
message("Version Information")
message("Firmware Version:  ${FW_VER_MAJOR}.${FW_VER_MINOR}.${FW_VER_PATCH}")
message("Build Machine:     ${BUILD_MACHINE}")
message("Build Information: ${RAW_GIT_DESCRIBE}")
message("Build Date:        ${BUILD_DATE}")
message("")