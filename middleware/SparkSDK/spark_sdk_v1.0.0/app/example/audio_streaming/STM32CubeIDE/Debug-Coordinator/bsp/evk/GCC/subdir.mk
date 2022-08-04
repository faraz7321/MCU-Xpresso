################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
/home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/bsp/hardware/evk/GCC/startup_stm32g473retx.s 

OBJS += \
./bsp/evk/GCC/startup_stm32g473retx.o 

S_DEPS += \
./bsp/evk/GCC/startup_stm32g473retx.d 


# Each subdirectory must supply rules for building sources it contributes
bsp/evk/GCC/startup_stm32g473retx.o: /home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/bsp/hardware/evk/GCC/startup_stm32g473retx.s bsp/evk/GCC/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m4 -g3 -DDEBUG -c -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-bsp-2f-evk-2f-GCC

clean-bsp-2f-evk-2f-GCC:
	-$(RM) ./bsp/evk/GCC/startup_stm32g473retx.d ./bsp/evk/GCC/startup_stm32g473retx.o

.PHONY: clean-bsp-2f-evk-2f-GCC

