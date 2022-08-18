################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include lib/third-party/stmicroelectronics/stm32g4xx_hal_driver/Src/subdir.mk
-include lib/third-party/stmicroelectronics/stm32_mw_usb_device/Core/Src/subdir.mk
-include lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/subdir.mk
-include lib/spark/queue/subdir.mk
-include lib/spark/memory/subdir.mk
-include core/wireless/protocol_stack/single_radio/subdir.mk
-include core/wireless/protocol_stack/subdir.mk
-include core/wireless/phy/sr1000/subdir.mk
-include core/wireless/link/subdir.mk
-include core/wireless/api/subdir.mk
-include bsp/interface/core_wireless/subdir.mk
-include bsp/interface/app/subdir.mk
-include bsp/evk/usb_device/target/subdir.mk
-include bsp/evk/usb_device/app/subdir.mk
-include bsp/evk/usb_device/subdir.mk
-include bsp/evk/GCC/subdir.mk
-include bsp/evk/subdir.mk
-include app/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := Hello World
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
Hello\ World.elf \

MAP_FILES += \
Hello\ World.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
Hello\ World.list \

OBJCOPY_BIN += \
Hello\ World.bin \


# All Target
all: main-build

# Main-build Target
main-build: Hello\ World.elf secondary-outputs

# Tool invocations
Hello\ World.elf Hello\ World.map: $(OBJS) $(USER_OBJS) /home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/app/example/hello_world/STM32CubeIDE/STM32G473RETX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "Hello World.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"/home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/app/example/hello_world/STM32CubeIDE/STM32G473RETX_FLASH.ld" --specs=nosys.specs -Wl,-Map="Hello World.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -u _printf_float -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

Hello\ World.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "Hello World.list"
	@echo 'Finished building: $@'
	@echo ' '

Hello\ World.bin: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "Hello World.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) Hello\ World.bin Hello\ World.elf Hello\ World.list Hello\ World.map Hello\\ World.elf default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
