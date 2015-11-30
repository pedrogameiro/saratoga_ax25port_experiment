################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../beacon.cpp \
../checksum.cpp \
../cli.cpp \
../data.cpp \
../dirent.cpp \
../dirflags.cpp \
../execute.cpp \
../fileio.cpp \
../flags.cpp \
../frame.cpp \
../globals.cpp \
../holes.cpp \
../htonll.cpp \
../htonlll.cpp \
../ip.cpp \
../metadata.cpp \
../offsetstr.cpp \
../peerinfo.cpp \
../readconf.cpp \
../request.cpp \
../saratoga.cpp \
../sarflags.cpp \
../screen.cpp \
../status.cpp \
../sysinfo.cpp \
../test.cpp \
../test1.cpp \
../test2.cpp \
../test3.cpp \
../timer.cpp \
../timestamp.cpp \
../tran.cpp 

OBJS += \
./beacon.o \
./checksum.o \
./cli.o \
./data.o \
./dirent.o \
./dirflags.o \
./execute.o \
./fileio.o \
./flags.o \
./frame.o \
./globals.o \
./holes.o \
./htonll.o \
./htonlll.o \
./ip.o \
./metadata.o \
./offsetstr.o \
./peerinfo.o \
./readconf.o \
./request.o \
./saratoga.o \
./sarflags.o \
./screen.o \
./status.o \
./sysinfo.o \
./test.o \
./test1.o \
./test2.o \
./test3.o \
./timer.o \
./timestamp.o \
./tran.o 

CPP_DEPS += \
./beacon.d \
./checksum.d \
./cli.d \
./data.d \
./dirent.d \
./dirflags.d \
./execute.d \
./fileio.d \
./flags.d \
./frame.d \
./globals.d \
./holes.d \
./htonll.d \
./htonlll.d \
./ip.d \
./metadata.d \
./offsetstr.d \
./peerinfo.d \
./readconf.d \
./request.d \
./saratoga.d \
./sarflags.d \
./screen.d \
./status.d \
./sysinfo.d \
./test.d \
./test1.d \
./test2.d \
./test3.d \
./timer.d \
./timestamp.d \
./tran.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -std=c++11 -g -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


