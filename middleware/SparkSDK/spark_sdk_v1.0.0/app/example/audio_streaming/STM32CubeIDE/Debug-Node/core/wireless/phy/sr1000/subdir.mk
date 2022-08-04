################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/core/wireless/phy/sr1000/sr_calib.c \
/home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/core/wireless/phy/sr1000/sr_nvm.c \
/home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/core/wireless/phy/sr1000/sr_nvm_private.c \
/home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/core/wireless/phy/sr1000/sr_spectral.c 

OBJS += \
./core/wireless/phy/sr1000/sr_calib.o \
./core/wireless/phy/sr1000/sr_nvm.o \
./core/wireless/phy/sr1000/sr_nvm_private.o \
./core/wireless/phy/sr1000/sr_spectral.o 

C_DEPS += \
./core/wireless/phy/sr1000/sr_calib.d \
./core/wireless/phy/sr1000/sr_nvm.d \
./core/wireless/phy/sr1000/sr_nvm_private.d \
./core/wireless/phy/sr1000/sr_spectral.d 


# Each subdirectory must supply rules for building sources it contributes
core/wireless/phy/sr1000/sr_calib.o: /home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/core/wireless/phy/sr1000/sr_calib.c core/wireless/phy/sr1000/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G473xx -c -I../../../../../app/example/audio_streaming -I../../../../../bsp/hardware/evk/usb_device -I../../../../../bsp/interface/lib/queue/evk -I../../../../../bsp -I../../../../../bsp/hardware -I../../../../../bsp/hardware/evk -I../../../../../bsp/hardware/evk/CMSIS/Core/Include -I../../../../../bsp/hardware/evk/usb_device/app -I../../../../../bsp/hardware/evk/usb_device/target -I../../../../../bsp/interface -I../../../../../bsp/interface/wireless_core -I../../../../../bsp/interface/audio_core -I../../../../../bsp/interface/app/audio_streaming -I../../../../../core/audio -I../../../../../core/audio/module -I../../../../../core/audio/processing -I../../../../../core/wireless/api -I../../../../../core/wireless/link -I../../../../../core/wireless/link/sr1000 -I../../../../../core/wireless/phy -I../../../../../core/wireless/phy/sr1000 -I../../../../../core/wireless/protocol_stack -I../../../../../core/wireless/protocol_stack/sr1000 -I../../../../../core/wireless/protocol_stack/sr1000/single_radio -I../../../../../core/wireless/transceiver -I../../../../../core/wireless/transceiver/sr1000 -I../../../../../core/wireless/xlayer -I../../../../../driver/spark/max98091 -I../../../../../lib/spark/adpcm -I../../../../../lib/spark/queue -I../../../../../lib/spark/memory -I../../../../../lib/spark/resampling -I../../../../../lib/third-party/stmicroelectronics/cmsis_device_g4/Include -I../../../../../lib/third-party/stmicroelectronics/stm32_mw_usb_device/Core/Inc -I../../../../../lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Inc -I../../../../../lib/third-party/stmicroelectronics/stm32g4xx_hal_driver/Inc -I../../../../../lib/third-party/arm-software/cmsis_5/CMSIS/DSP/Include -I../../../../../lib/third-party/arm-software/cmsis_5/CMSIS/DSP/PrivateInclude -I../../../../../lib/spark/crc -O2 -ffunction-sections -fdata-sections -Wall -Wextra -Wshadow=compatible-local -Werror=implicit-function-declaration -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
core/wireless/phy/sr1000/sr_nvm.o: /home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/core/wireless/phy/sr1000/sr_nvm.c core/wireless/phy/sr1000/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G473xx -c -I../../../../../app/example/audio_streaming -I../../../../../bsp/hardware/evk/usb_device -I../../../../../bsp/interface/lib/queue/evk -I../../../../../bsp -I../../../../../bsp/hardware -I../../../../../bsp/hardware/evk -I../../../../../bsp/hardware/evk/CMSIS/Core/Include -I../../../../../bsp/hardware/evk/usb_device/app -I../../../../../bsp/hardware/evk/usb_device/target -I../../../../../bsp/interface -I../../../../../bsp/interface/wireless_core -I../../../../../bsp/interface/audio_core -I../../../../../bsp/interface/app/audio_streaming -I../../../../../core/audio -I../../../../../core/audio/module -I../../../../../core/audio/processing -I../../../../../core/wireless/api -I../../../../../core/wireless/link -I../../../../../core/wireless/link/sr1000 -I../../../../../core/wireless/phy -I../../../../../core/wireless/phy/sr1000 -I../../../../../core/wireless/protocol_stack -I../../../../../core/wireless/protocol_stack/sr1000 -I../../../../../core/wireless/protocol_stack/sr1000/single_radio -I../../../../../core/wireless/transceiver -I../../../../../core/wireless/transceiver/sr1000 -I../../../../../core/wireless/xlayer -I../../../../../driver/spark/max98091 -I../../../../../lib/spark/adpcm -I../../../../../lib/spark/queue -I../../../../../lib/spark/memory -I../../../../../lib/spark/resampling -I../../../../../lib/third-party/stmicroelectronics/cmsis_device_g4/Include -I../../../../../lib/third-party/stmicroelectronics/stm32_mw_usb_device/Core/Inc -I../../../../../lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Inc -I../../../../../lib/third-party/stmicroelectronics/stm32g4xx_hal_driver/Inc -I../../../../../lib/third-party/arm-software/cmsis_5/CMSIS/DSP/Include -I../../../../../lib/third-party/arm-software/cmsis_5/CMSIS/DSP/PrivateInclude -I../../../../../lib/spark/crc -O2 -ffunction-sections -fdata-sections -Wall -Wextra -Wshadow=compatible-local -Werror=implicit-function-declaration -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
core/wireless/phy/sr1000/sr_nvm_private.o: /home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/core/wireless/phy/sr1000/sr_nvm_private.c core/wireless/phy/sr1000/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G473xx -c -I../../../../../app/example/audio_streaming -I../../../../../bsp/hardware/evk/usb_device -I../../../../../bsp/interface/lib/queue/evk -I../../../../../bsp -I../../../../../bsp/hardware -I../../../../../bsp/hardware/evk -I../../../../../bsp/hardware/evk/CMSIS/Core/Include -I../../../../../bsp/hardware/evk/usb_device/app -I../../../../../bsp/hardware/evk/usb_device/target -I../../../../../bsp/interface -I../../../../../bsp/interface/wireless_core -I../../../../../bsp/interface/audio_core -I../../../../../bsp/interface/app/audio_streaming -I../../../../../core/audio -I../../../../../core/audio/module -I../../../../../core/audio/processing -I../../../../../core/wireless/api -I../../../../../core/wireless/link -I../../../../../core/wireless/link/sr1000 -I../../../../../core/wireless/phy -I../../../../../core/wireless/phy/sr1000 -I../../../../../core/wireless/protocol_stack -I../../../../../core/wireless/protocol_stack/sr1000 -I../../../../../core/wireless/protocol_stack/sr1000/single_radio -I../../../../../core/wireless/transceiver -I../../../../../core/wireless/transceiver/sr1000 -I../../../../../core/wireless/xlayer -I../../../../../driver/spark/max98091 -I../../../../../lib/spark/adpcm -I../../../../../lib/spark/queue -I../../../../../lib/spark/memory -I../../../../../lib/spark/resampling -I../../../../../lib/third-party/stmicroelectronics/cmsis_device_g4/Include -I../../../../../lib/third-party/stmicroelectronics/stm32_mw_usb_device/Core/Inc -I../../../../../lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Inc -I../../../../../lib/third-party/stmicroelectronics/stm32g4xx_hal_driver/Inc -I../../../../../lib/third-party/arm-software/cmsis_5/CMSIS/DSP/Include -I../../../../../lib/third-party/arm-software/cmsis_5/CMSIS/DSP/PrivateInclude -I../../../../../lib/spark/crc -O2 -ffunction-sections -fdata-sections -Wall -Wextra -Wshadow=compatible-local -Werror=implicit-function-declaration -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
core/wireless/phy/sr1000/sr_spectral.o: /home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/core/wireless/phy/sr1000/sr_spectral.c core/wireless/phy/sr1000/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G473xx -c -I../../../../../app/example/audio_streaming -I../../../../../bsp/hardware/evk/usb_device -I../../../../../bsp/interface/lib/queue/evk -I../../../../../bsp -I../../../../../bsp/hardware -I../../../../../bsp/hardware/evk -I../../../../../bsp/hardware/evk/CMSIS/Core/Include -I../../../../../bsp/hardware/evk/usb_device/app -I../../../../../bsp/hardware/evk/usb_device/target -I../../../../../bsp/interface -I../../../../../bsp/interface/wireless_core -I../../../../../bsp/interface/audio_core -I../../../../../bsp/interface/app/audio_streaming -I../../../../../core/audio -I../../../../../core/audio/module -I../../../../../core/audio/processing -I../../../../../core/wireless/api -I../../../../../core/wireless/link -I../../../../../core/wireless/link/sr1000 -I../../../../../core/wireless/phy -I../../../../../core/wireless/phy/sr1000 -I../../../../../core/wireless/protocol_stack -I../../../../../core/wireless/protocol_stack/sr1000 -I../../../../../core/wireless/protocol_stack/sr1000/single_radio -I../../../../../core/wireless/transceiver -I../../../../../core/wireless/transceiver/sr1000 -I../../../../../core/wireless/xlayer -I../../../../../driver/spark/max98091 -I../../../../../lib/spark/adpcm -I../../../../../lib/spark/queue -I../../../../../lib/spark/memory -I../../../../../lib/spark/resampling -I../../../../../lib/third-party/stmicroelectronics/cmsis_device_g4/Include -I../../../../../lib/third-party/stmicroelectronics/stm32_mw_usb_device/Core/Inc -I../../../../../lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Inc -I../../../../../lib/third-party/stmicroelectronics/stm32g4xx_hal_driver/Inc -I../../../../../lib/third-party/arm-software/cmsis_5/CMSIS/DSP/Include -I../../../../../lib/third-party/arm-software/cmsis_5/CMSIS/DSP/PrivateInclude -I../../../../../lib/spark/crc -O2 -ffunction-sections -fdata-sections -Wall -Wextra -Wshadow=compatible-local -Werror=implicit-function-declaration -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-core-2f-wireless-2f-phy-2f-sr1000

clean-core-2f-wireless-2f-phy-2f-sr1000:
	-$(RM) ./core/wireless/phy/sr1000/sr_calib.d ./core/wireless/phy/sr1000/sr_calib.o ./core/wireless/phy/sr1000/sr_calib.su ./core/wireless/phy/sr1000/sr_nvm.d ./core/wireless/phy/sr1000/sr_nvm.o ./core/wireless/phy/sr1000/sr_nvm.su ./core/wireless/phy/sr1000/sr_nvm_private.d ./core/wireless/phy/sr1000/sr_nvm_private.o ./core/wireless/phy/sr1000/sr_nvm_private.su ./core/wireless/phy/sr1000/sr_spectral.d ./core/wireless/phy/sr1000/sr_spectral.o ./core/wireless/phy/sr1000/sr_spectral.su

.PHONY: clean-core-2f-wireless-2f-phy-2f-sr1000

