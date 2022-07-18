# NXP IMX8MM-EVK
# Implement Spark Stack on MCUXpresso SDK


# NXP IMX8MM-EVK
## Evaluation kit
- [EVK][1]

## Getting started with the EVK
### Installing Linux
  - Download [Linux binary demo][7]
  - Download [UUU][2]
~~~~
~$ chmod a+x UUU
~$ sudo ./uuu <linux_images_MX8MMEVK.zip>
~~~~
  - Boot switch setup
  - [Linux User Guide][3]
  - [Reference Manual][4]

### MCUXpresso SDK
- Download [SDK][5]. 
- We used MCUXpresso version 2.11.1. 
- NOTE: Initialization of pins in pinmux.c of few examples is missing (fixed in new version 2.12.0).
```css
void BOARD_InitBootPins(void)
{
    BOARD_InitPins(); //missing function added
}
```
- [PinMux Tool][6]
- Set environment:
> export ARMGCC_DIR=/gcc-arm-11.2-2022.02-x86_64-arm-none-eabi
- To build example run
> <DIRECTORY>/armgcc/build_.sh
- Running application on Cortex-4: 
-   Setup TFTP on PC.
~~~~
~$ sudo apt update
~$ sudo apt install tftpd-hpa
~$ sudo systemctl status tftpd-hpa
Change username and directory:
~$ sudo nano /etc/default/tftpd-hpa
~$ sudo mkdir /tftp
~$ sudo chown tftp:tftp /tftp
~$ sudo systemctl restart tftpd-hpa
~$ sudo systemctl status tftpd-hpa
~~~~
- Copy .bin to tftp/
~~~~
~$ sudo cp release/app.bin ~/tftp/
~$ sudo chown tftp:tftp ~/tftp/app.bin
~~~~
- Open terminal on PC with following config:
~~~~
~$ sudo picocom -b 112500 /dev/ttyUSB*
~~~~
- Load u-boot on IMX8MM-EVK
- To run TCM builds from u-boot:
~~~~
=> setenv <env_name> "dhcp; setenv serverip <ip_addr>; tftp 0x48000000 .bin; cp.b 0x48000000 0x7e0000 0x20000; bootaux 0x7e0000"
=> saveenv
=> run <env_name>
~~~~
- Open terminal for Cortex-M4 to see serial output from the app running on it.

#### Testing Examples
##### ECSPI Driver
- Interrupt B2B Transfer master as loopback using single board.
- Connect MOSI and MISO pins physically on board (J1003-PIN_19-ECSPI2_MOSI & J1003-PIN_21-ECSPI2_MISO ).
- Edition in driver code:
> Assign masterTxData and masterRxData buffer to escpi_transfer_t object.

##### GPIO Driver
- Used led_output example
- Digital output -> Digital input (rising edge triggered)
- Changing pin from ECSPI2_MOSI to SAI5_RXD0 in pinmux.c.
- Refer to PinMux Tool for more information.
- Interrupt Enabled on GPIO pin J1003-38 (originally configured to J1003-19).
- Implement GPIO3_Combined_16_31_IRQHandler() function 
#### Extras
- [Logic Analyzer][8]
- [ARM GCC Toolchain][9]

# Spark SDK & UWB
- Historical Background of UWB: https://semiwiki.com/5g/283112-the-story-of-ultra-wideband-part-1-the-genesis/
## Tools and Software
- SDK & Documentation: https://www.sparkmicro.com/products/#SDK
- Version we used: v1.0.0 CubeIDE for debugging.
- STM32 ST-Link V2.
- STM32CubeProgrammer to flash .bin files.

## Testing Examples
- Add SDK to the workspace in CubeIDE.
- Connect the EVK board via STM32 ST-Link V2.
- Build the project and start debugging.
### Audio Streaming
- Added necessary functions and header files for serial output as implemented in Hello World example.
```css
void iface_board_init(void)
{     
    evk_init();
    usb_detect_callback_init();
    usb_connection_init(); 
}

void evk_usb_device_cdc_send_buf(uint8_t *buf, uint16_t buf_len);
```
- Serial output
~~~~
~$ sudo picocom -b 112500 /dev/ttyACM* 
~~~~
- Generating a custom sine wave and sending it though wireless core.
- Codec writes data into DMA then DMA transfer the data to wireless core.
- Implementing our sine wave where the codec generates buffer and starts the DMA; "iface_audio_evk.c".
- Amplitude = 32767.0
- Sample Rate = 48kHz
```css
uint8_t dummy[256];
static uint16_t ep_max98091_action_produce(void *instance, uint8_t *data, uint16_t size)
{
    (void)instance;
    int16_t *samples = (int16_t *)data;
    uint16_t samples_count = size / 2;
    for (int n = 0; n < samples_count / 2; n = n + 2)
    {
        samples[n] = wave[sample_count];
        samples[n + 1] = samples[n];
        sample_count += 1;
        if (sample_count >= 48000)
        {
            sample_count = 0;
        }
    }
    evk_audio_sai_read_non_blocking(dummy, size);
    return 0;
}
static void ep_max98091_start_produce(void *instance)
{
    (void)instance;
    for (int n = 0; n < 48000; n++)
    {
        wave[n] = (double)(AMP * sinf((M_PI * 2 / SAMPLE_RATE) * FREQUENCY * n));
    }
    evk_audio_sai_start_read_non_blocking();
}
```
# Implement Spark Stack on MCUXpresso SDK





[1]: https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/evaluation-kit-for-the-i-mx-8m-mini-applications-processor:8MMINILPD4-EVK "EVK"

[2]: https://www.nxp.com/design/software/embedded-software/i-mx-software/embedded-linux-for-i-mx-applications-processors:IMXLINUX "UUU"

[3]: https://www.nxp.com/docs/en/user-guide/IMXLUG.pdf "Linux User Guide"

[4]: https://www.nxp.com/docs/en/reference-manual/i.MX_Reference_Manual_Linux.pdf "Reference Manual"

[5]: https://mcuxpresso.nxp.com/en/welcome "SDK"

[6]: https://mcuxpresso.nxp.com/en/pins "PinMux tool"

[7]: https://www.nxp.com/design/software/embedded-software/i-mx-software/embedded-linux-for-i-mx-applications-processors:IMXLINUX "Linux binary demo"

[8]: https://www.saleae.com/downloads/ "Logic Analyzer"

[9]: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/downloads "ARM GCC Toolchain"
