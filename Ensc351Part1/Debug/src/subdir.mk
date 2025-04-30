################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Ensc351Part1.cpp \
../src/PeerY.cpp \
../src/SenderY.cpp \
../src/myIO.cpp 

CPP_DEPS += \
./src/Ensc351Part1.d \
./src/PeerY.d \
./src/SenderY.d \
./src/myIO.d 

OBJS += \
./src/Ensc351Part1.o \
./src/PeerY.o \
./src/SenderY.o \
./src/myIO.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++2a -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/PeerY.o: ../src/PeerY.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++2a -O0 -g3 -Wall -c -fmessage-length=0 -Wno-register -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/Ensc351Part1.d ./src/Ensc351Part1.o ./src/PeerY.d ./src/PeerY.o ./src/SenderY.d ./src/SenderY.o ./src/myIO.d ./src/myIO.o

.PHONY: clean-src

