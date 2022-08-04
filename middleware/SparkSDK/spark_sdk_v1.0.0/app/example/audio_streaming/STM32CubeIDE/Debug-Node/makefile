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
-include lib/third-party/arm-software/cmsis-dsp/Source/FilteringFunctions/subdir.mk
-include lib/spark/resampling/subdir.mk
-include lib/spark/queue/subdir.mk
-include lib/spark/memory/subdir.mk
-include lib/spark/crc/subdir.mk
-include driver/spark/max98091/subdir.mk
-include core/wireless/protocol_stack/sr1000/single_radio/subdir.mk
-include core/wireless/protocol_stack/sr1000/subdir.mk
-include core/wireless/protocol_stack/subdir.mk
-include core/wireless/phy/sr1000/subdir.mk
-include core/wireless/link/sr1000/subdir.mk
-include core/wireless/link/subdir.mk
-include core/wireless/api/subdir.mk
-include core/audio/processing/subdir.mk
-include core/audio/module/subdir.mk
-include core/audio/subdir.mk
-include bsp/interface/core_wireless/subdir.mk
-include bsp/interface/core_audio/subdir.mk
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


BUILD_ARTIFACT_NAME := Audio Streaming
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
Audio\ Streaming.elf \

MAP_FILES += \
Audio\ Streaming.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
Audio\ Streaming.list \

OBJCOPY_BIN += \
Audio\ Streaming.bin \


# All Target
all: main-build

# Main-build Target
main-build: Audio\ Streaming.elf secondary-outputs

# Tool invocations
Audio\ Streaming.elf Audio\ Streaming.map: $(OBJS) $(USER_OBJS) /home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/app/example/audio_streaming/STM32CubeIDE/STM32G473RETX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "Audio Streaming.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"/home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/app/example/audio_streaming/STM32CubeIDE/STM32G473RETX_FLASH.ld" --specs=nosys.specs -Wl,-Map="Audio Streaming.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

Audio\ Streaming.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "Audio Streaming.list"
	@echo 'Finished building: $@'
	@echo ' '

Audio\ Streaming.bin: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "Audio Streaming.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) Audio\ Streaming.bin Audio\ Streaming.elf Audio\ Streaming.list Audio\ Streaming.map Audio\\ Streaming.elf default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
