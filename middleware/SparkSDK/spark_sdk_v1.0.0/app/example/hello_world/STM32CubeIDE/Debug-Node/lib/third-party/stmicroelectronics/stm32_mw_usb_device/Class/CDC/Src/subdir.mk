################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc.c \
/home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc_if_template.c 

OBJS += \
./lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc.o \
./lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc_if_template.o 

C_DEPS += \
./lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc.d \
./lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc_if_template.d 


# Each subdirectory must supply rules for building sources it contributes
lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc.o: /home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc.c lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G473xx -c -I../../../../../app/example/hello_world -I../../../../../bsp -I../../../../../bsp/hardware -I../../../../../bsp/hardware/evk -I../../../../../bsp/hardware/evk/CMSIS/Core/Include -I../../../../../bsp/hardware/evk/usb_device/app -I../../../../../bsp/hardware/evk/usb_device/target -I../../../../../bsp/interface -I../../../../../bsp/interface/wireless_core -I../../../../../bsp/interface/audio_core -I../../../../../bsp/interface/app/hello_world -I../../../../../core/audio -I../../../../../core/audio/processing -I../../../../../core/wireless/api -I../../../../../core/wireless/link -I../../../../../core/wireless/link/sr1000 -I../../../../../core/wireless/phy -I../../../../../core/wireless/phy/sr1000 -I../../../../../core/wireless/protocol_stack -I../../../../../core/wireless/protocol_stack/sr1000 -I../../../../../core/wireless/protocol_stack/sr1000/single_radio -I../../../../../core/wireless/transceiver -I../../../../../core/wireless/transceiver/sr1000 -I../../../../../core/wireless/xlayer -I../../../../../driver/spark/max98091 -I../../../../../lib/spark/adpcm -I../../../../../lib/spark/queue -I../../../../../lib/spark/memory -I../../../../../lib/spark/resampling -I../../../../../lib/third-party/stmicroelectronics/cmsis_device_g4/Include -I../../../../../lib/third-party/stmicroelectronics/stm32_mw_usb_device/Core/Inc -I../../../../../lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Inc -I../../../../../lib/third-party/stmicroelectronics/stm32g4xx_hal_driver/Inc -I../../../../../bsp/interface/lib/queue/evk -I../../../../../bsp/hardware/evk/usb_device -O2 -ffunction-sections -fdata-sections -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc_if_template.o: /home/foronz/NXP-MCUXpresso/middleware/SparkSDK/spark_sdk_v1.0.0/lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc_if_template.c lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G473xx -c -I../../../../../app/example/hello_world -I../../../../../bsp -I../../../../../bsp/hardware -I../../../../../bsp/hardware/evk -I../../../../../bsp/hardware/evk/CMSIS/Core/Include -I../../../../../bsp/hardware/evk/usb_device/app -I../../../../../bsp/hardware/evk/usb_device/target -I../../../../../bsp/interface -I../../../../../bsp/interface/wireless_core -I../../../../../bsp/interface/audio_core -I../../../../../bsp/interface/app/hello_world -I../../../../../core/audio -I../../../../../core/audio/processing -I../../../../../core/wireless/api -I../../../../../core/wireless/link -I../../../../../core/wireless/link/sr1000 -I../../../../../core/wireless/phy -I../../../../../core/wireless/phy/sr1000 -I../../../../../core/wireless/protocol_stack -I../../../../../core/wireless/protocol_stack/sr1000 -I../../../../../core/wireless/protocol_stack/sr1000/single_radio -I../../../../../core/wireless/transceiver -I../../../../../core/wireless/transceiver/sr1000 -I../../../../../core/wireless/xlayer -I../../../../../driver/spark/max98091 -I../../../../../lib/spark/adpcm -I../../../../../lib/spark/queue -I../../../../../lib/spark/memory -I../../../../../lib/spark/resampling -I../../../../../lib/third-party/stmicroelectronics/cmsis_device_g4/Include -I../../../../../lib/third-party/stmicroelectronics/stm32_mw_usb_device/Core/Inc -I../../../../../lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Inc -I../../../../../lib/third-party/stmicroelectronics/stm32g4xx_hal_driver/Inc -I../../../../../bsp/interface/lib/queue/evk -I../../../../../bsp/hardware/evk/usb_device -O2 -ffunction-sections -fdata-sections -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-lib-2f-third-2d-party-2f-stmicroelectronics-2f-stm32_mw_usb_device-2f-Class-2f-CDC-2f-Src

clean-lib-2f-third-2d-party-2f-stmicroelectronics-2f-stm32_mw_usb_device-2f-Class-2f-CDC-2f-Src:
	-$(RM) ./lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc.d ./lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc.o ./lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc.su ./lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc_if_template.d ./lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc_if_template.o ./lib/third-party/stmicroelectronics/stm32_mw_usb_device/Class/CDC/Src/usbd_cdc_if_template.su

.PHONY: clean-lib-2f-third-2d-party-2f-stmicroelectronics-2f-stm32_mw_usb_device-2f-Class-2f-CDC-2f-Src

