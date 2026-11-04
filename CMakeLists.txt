cmake_minimum_required(VERSION 3.16)


# give the project a name and specify with which languages I am working with
project(thesis LANGUAGES CXX)


# Compile as C++17 and fail if it is not available
# C++17 is chosen because of the limit of the apt avr-gcc therefore C++20 is not available
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Default configuration: Release (if you did not chose a configuration)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()


# Set here and not in the toolChain becuase CMake appends its own "-O3 DNDEBUG"
# Set here and not in the toolchain file because CMake's default Release value is
# "-O3 -DNDEBUG" and, if set in the toolchain, -O3 would come after -Os and win.
# Setting the variable here replaces the whole default value (so -DNDEBUG is dropped too)
set(CMAKE_CXX_FLAGS_RELEASE "-Os")


# create and executable from main.cc and name it main
add_executable(main src/main.cc)


# add_executable create the target 'main' the instruction below append .elf (main -> main.elf)
set_target_properties(main PROPERTIES SUFFIX ".elf")




# After linking main (POST_BUILD), run these commands:
#
# FIRST COMMAND: convert main.elf -> main.hex
#   avr-objcopy                 converts a compiled file to another format (binutils-avr)
#   -O ihex                     write the result in Intel HEX format (the bytes to be loaded)
#   -R .eeprom                  remove the .eeprom section (EEPROM data is loaded separately)
#   $<TARGET_FILE:main>         path of main.elf (the input)
#   $<TARGET_FILE_DIR:main>     folder of main.elf (main.hex is written there)
#
# SECOND COMMAND: print the flash and RAM usage with the percentage of the chip
#   -C                 use 'avr' output format (percentage)
#   --mcu=atmega328p   chip whose memory sizes are used to computer the percentage
# 
# VERBATIM: pass the args as I wrote them protecting from special characters
add_custom_command(TARGET main POST_BUILD
    COMMAND avr-objcopy -O ihex -R .eeprom $<TARGET_FILE:main> $<TARGET_FILE_DIR:main>/main.hex
    COMMAND avr-size -C --mcu=atmega328p $<TARGET_FILE:main>
    VERBATIM)



# tell the compiler to check in include/ dir to find header files
# PRIVATE means that is valid only for main target
target_include_directories(main PRIVATE include)



