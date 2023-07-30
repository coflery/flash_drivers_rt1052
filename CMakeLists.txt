cmake_minimum_required(VERSION 3.12)
include(cmake/toolchain.cmake)
include(cmake/version.cmake)

# Define system require
set(CMAKE_SYSTEM_NAME           Generic)
set(CMAKE_SYSTEM_PROCESSOR      arm)
set(CMAKE_C_STANDARD            11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD          14)

# Define project information
set(PROJECT_NAME        IMXRT1050_QSPIFLASH_ROMAPI)
project(${PROJECT_NAME} C CXX ASM)
set(TARGET              ${PROJECT_NAME}.elf)
set(SRC_ROOT            ${CMAKE_SOURCE_DIR}/src)

#------------------------------ Config -------------------------------#
# Startup and link script
set(SRC_ASM         "${SRC_ROOT}/MIMXRT1052/gcc/startup_MIMXRT1052.S")
set(LDSCRIPT        "${SRC_ROOT}/imxrt/IMXRT1050_QSPIFLASH_ROMAPI/MIMXRT1052xxxxx_ram.ld")
# DSP hardware acceleration library
#set(DSP_LIB         "${SRC_ROOT}/libarm_cortexM7lfsp_math.a")

# Load config file
set(PRODUCT_CONFIG "${CMAKE_SOURCE_DIR}/product.config" CACHE PATH "Config product file")
if(EXISTS ${PRODUCT_CONFIG})
    configure_file(${PRODUCT_CONFIG} ${CMAKE_SOURCE_DIR}/CMakeCache.txt COPYONLY)
    load_cache(${CMAKE_SOURCE_DIR})
    file(REMOVE ${CMAKE_SOURCE_DIR}/CMakeCache.txt)
else()
    message(STATUS "product.config not found")
endif()


#----------------------------- Compiler ------------------------------#
# Compiler flags
set(CPU           "cortex-m7")
set(INSTRUCTION   "thumb")
set(FPU           "fpv5-d16")
set(FLOAT_ABI     "hard")
set(PLATFORM      "-mcpu=${CPU} -m${INSTRUCTION} -mfpu=${FPU} -mfloat-abi=${FLOAT_ABI}")
set(OPTIMIZE      "-O0")
set(DEBUG_LEVEL   "-g")
set(WARN_LEVEL    "-Wall")
set(SECTION       "-fdata-sections -ffunction-sections")
set(COMPILE_FLAGS "${PLATFORM} ${OPTIMIZE} ${DEBUG_LEVEL} ${WARN_LEVEL} ${SECTION}")
#set(WNOPSABI     "-Wno-psabi")    # Ignore note about ABI compatibility in GCC10
#set(WNOREG       "-Wno-register") # Ignore warning about register removed from c++17
set(WNOATTRIB     "-Wno-attribute-alias -Wno-missing-attributes -Wno-unknown-pragmas")

# Add c compiler flags
set(CMAKE_C_FLAGS   "${COMPILE_FLAGS}")
# Add c++ compiler flags
set(CMAKE_CXX_FLAGS "${COMPILE_FLAGS} ${WNOPSABI} ${WNOREG} ${WNOATTRIB}")


#------------------------------ Linker -------------------------------#
# Linker flags
set(LIBS                   "-lm -lnosys")
set(CLIB                   "-specs=nosys.specs -specs=nano.specs")
#set(STD_PRINT              "-u _printf_float -u _scanf_float")
set(GC_SECTIONS            "-Wl,--gc-sections")
set(MAP                    "-Wl,-Map=${PROJECT_NAME}.map,--cref")
set(PRINT_MEMORY           "-Wl,--print-memory-usage")
#set(RTOS_USEDPRIORITY      "-Wl,--undefined=uxTopUsedPriority")
set(CMAKE_EXE_LINKER_FLAGS "${LIBS} ${CLIB} ${STD_PRINT} ${GC_SECTIONS} ${MAP} ${PRINT_MEMORY} ${RTOS_USEDPRIORITY} -T${LDSCRIPT}")


#--------------------------- Source files ----------------------------#
# Add all source file
file(GLOB_RECURSE SRC_LIST
    ${SRC_ROOT}/*.c
    ${SRC_ROOT}/*.cpp
)

# Convert list to newline
foreach(line IN LISTS SRC_LIST)
    string(APPEND src_dir "${line}\n")
endforeach()
# Print source file list
message("Source files: \n${src_dir}")

# Macro of get subdirectory tree
MACRO(SUBDIRLIST result curdir)
  FILE(GLOB_RECURSE children LIST_DIRECTORIES true ${curdir}/*)
  SET(dirlist "")
  LIST(APPEND dirlist ${curdir})
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${child})
        LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

# Get all the folder(include the self-root)
SUBDIRLIST(inc_dirs_list ${SRC_ROOT})

# Convert list to newline
foreach(line IN LISTS inc_dirs_list)
    string(APPEND inc_dirs "${line}\n")
endforeach()
# Print header directory list
message("Header files directory: \n${inc_dirs}")

# Include all header directory
foreach(subfolder ${inc_dirs_list})
    include_directories(${subfolder})
endforeach()

# Add all executable files
add_executable(${TARGET}
    ${SRC_ASM}
    ${SRC_LIST}
)

# Add all library files
target_link_libraries(${TARGET}
    ${DSP_LIB}
)

# Add public definition
target_compile_definitions(${TARGET}
    PRIVATE __USE_CMSIS
    PRIVATE CPU_MIMXRT1052DVL6B
    PRIVATE CODEC_MULTI_ADAPTERS
    PRIVATE SERIAL_PORT_TYPE_UART
    PRIVATE SERIAL_PORT_TYPE_SPI
)


#-------------------------- Output Setting ---------------------------#
# Targets for debug & download
set(LST_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.lst)
set(HEX_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin)

# Generate lst file
add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -St $<TARGET_FILE:${TARGET}> > ${LST_FILE}
    COMMENT "Building ${LST_FILE}"
)

# Generate hex file
add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${TARGET}> ${HEX_FILE}
    COMMENT "Building ${HEX_FILE}"
)

# Generate bin file
add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET}> ${BIN_FILE}
    COMMENT "Building ${BIN_FILE}"
)

# Print target size
add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND ${CMAKE_SIZE} ${TARGET}
)


#------------------------- Debugger Setting --------------------------#
# JLink as gdb server
add_custom_target(jlink
    COMMAND JLinkGDBServer
    -device MIMXRT1052DVL6B
    -endian little
    -speed 4000
    -port 3333
    -c "bindto 0.0.0.0"

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Openocd as gdb server
add_custom_target(openocd
    COMMAND openocd
    -f interface/ft4232.cfg
    -f target/imxrt.cfg
    -c "gdb_port 3333"
    -c "telnet_port 4444"
    -c "bindto 0.0.0.0"

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Flash the previously built firmware to the target
add_custom_target(flash
    COMMAND openocd
    -f interface/ft4232.cfg
    -f target/imxrt.cfg
    -c init
    -c "reset_into_boot_rom 1"
    -c "reset halt"
    -c "program ${TARGET}"
    -c reset
    -c exit

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS ${TARGET}
)

# Write to chip
add_custom_target(write
    COMMAND openocd
    -f interface/ft4232.cfg
    -f target/imxrt.cfg
    -c init
    -c halt
    #-c "load_image write.elf 0x20200000"
    -c "flash write_image write.bin 0x60000000"
    -c "reset"
    -c exit

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Read back full chip
add_custom_target(read
    COMMAND openocd
    -f interface/ft4232.cfg
    -f target/imxrt.cfg
    -c init
    -c halt
    #-c "dump_image read.bin 0x60000000 0x100000"
    -c "flash read_bank 0 read.bin"
    -c reset
    -c exit

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Erase full chip
add_custom_target(erase
    COMMAND openocd
    -f interface/ft4232.cfg
    -f target/imxrt.cfg
    -c init
    -c halt
    -c "flash probe 0"
    -c "flash erase_sector 0 0 last"
    -c exit

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Unlock read write protect(may full chip erase)
add_custom_target(unlock
    COMMAND openocd
    -f interface/ft4232.cfg
    -f target/imxrt.cfg
    -c init
    -c "reset"
    -c "flash protect 0 0 last off"
    -c exit

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Reset target
add_custom_target(reset
    COMMAND openocd
    -f interface/ft4232.cfg
    -f target/imxrt.cfg
    -c init
    -c reset
    -c exit

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)