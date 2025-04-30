################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Ensc351Part6.cpp \
../src/Kvm.cpp \
../src/Medium.cpp \
../src/main.cpp 

CPP_DEPS += \
./src/Ensc351Part6.d \
./src/Kvm.d \
./src/Medium.d \
./src/main.d 

OBJS += \
./src/Ensc351Part6.o \
./src/Kvm.o \
./src/Medium.o \
./src/main.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++2a -I"/mnt/hgfs/VMsf/eclipse-workspace_24-06_1247/Ensc351" -I"/mnt/hgfs/VMsf/eclipse-workspace_24-06_1247/Ensc351ymodLib" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/Ensc351Part6.d ./src/Ensc351Part6.o ./src/Kvm.d ./src/Kvm.o ./src/Medium.d ./src/Medium.o ./src/main.d ./src/main.o

.PHONY: clean-src

