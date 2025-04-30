################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../PeerY.cpp \
../ReceiverY.cpp \
../SenderY.cpp \
../myIO.cpp \
../terminal.cpp \
../yReceiverSS.cpp \
../ySenderSS.cpp 

C_SRCS += \
../crc.c 

CPP_DEPS += \
./PeerY.d \
./ReceiverY.d \
./SenderY.d \
./myIO.d \
./terminal.d \
./yReceiverSS.d \
./ySenderSS.d 

C_DEPS += \
./crc.d 

OBJS += \
./PeerY.o \
./ReceiverY.o \
./SenderY.o \
./crc.o \
./myIO.o \
./terminal.o \
./yReceiverSS.o \
./ySenderSS.o 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++2a -I"/mnt/hgfs/VMsf/eclipse-workspace_24-06_1247/Ensc351" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.c subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/mnt/hgfs/VMsf/eclipse-workspace_24-06_1247/Ensc351" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

yReceiverSS.o: ../yReceiverSS.cpp subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++2a -I"/mnt/hgfs/VMsf/eclipse-workspace_24-06_1247/Ensc351" -O0 -g3 -Wall -c -fmessage-length=0 -Wno-unused-variable -Wno-unknown-pragmas -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

ySenderSS.o: ../ySenderSS.cpp subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++2a -I"/mnt/hgfs/VMsf/eclipse-workspace_24-06_1247/Ensc351" -O0 -g3 -Wall -c -fmessage-length=0 -Wno-unused-variable -Wno-unknown-pragmas -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean--2e-

clean--2e-:
	-$(RM) ./PeerY.d ./PeerY.o ./ReceiverY.d ./ReceiverY.o ./SenderY.d ./SenderY.o ./crc.d ./crc.o ./myIO.d ./myIO.o ./terminal.d ./terminal.o ./yReceiverSS.d ./yReceiverSS.o ./ySenderSS.d ./ySenderSS.o

.PHONY: clean--2e-

