################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../checksums/crc32.c \
../checksums/crc32driver.c \
../checksums/md5.c \
../checksums/md5driver.c \
../checksums/sha1.c \
../checksums/sha1driver.c 

OBJS += \
./checksums/crc32.o \
./checksums/crc32driver.o \
./checksums/md5.o \
./checksums/md5driver.o \
./checksums/sha1.o \
./checksums/sha1driver.o 

C_DEPS += \
./checksums/crc32.d \
./checksums/crc32driver.d \
./checksums/md5.d \
./checksums/md5driver.d \
./checksums/sha1.d \
./checksums/sha1driver.d 


# Each subdirectory must supply rules for building sources it contributes
checksums/%.o: ../checksums/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -m32 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


