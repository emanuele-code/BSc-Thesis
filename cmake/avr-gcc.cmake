# Generic because destination system has not an os
set(CMAKE_SYSTEM_NAME Generic)    


# destination architecture is AVR (for microcontrollers)
set(CMAKE_SYSTEM_PROCESSOR avr)


# use avr-gcc and avr-g++ compiler instead of gcc or g++
set(CMAKE_C_COMPILER   avr-gcc)     
set(CMAKE_CXX_COMPILER avr-g++)


set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY) 


# Initial options for every C++ compilation:
#  -mmcu=atmega328p      target chip (ATmega328P, the Arduino Uno MCU)
#  -DF_CPU=16000000UL    defines F_CPU for avr-libc: the Uno has a 16 MHz crystal
#  -fno-exceptions       disable exceptions (try/throw/catch) to save flash
#  -fno-rtti             disable run-time type information (typeid, dynamic_cast);
#                        needed here because classes with virtual functions
#                        fail to link with RTTI enabled
set(CMAKE_CXX_FLAGS_INIT "-mmcu=atmega328p -DF_CPU=16000000UL -fno-exceptions -fno-rtti")
