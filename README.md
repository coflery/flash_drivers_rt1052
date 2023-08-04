# <center>RT1052 Flash Driver in OpenOCD

## Require

`Chip factory default setting whithout FlexRAM efuse modify`
* ITCM   >=32 KByte /* Default 128KB */
* DTCM   >=32 KByte /* Default 128KB */
* OCRAM  >=32 KByte /* Default 256KB */

## Test pass platform
* openocd 0.12.0
* Crystal 24MHZ
* Crystal 32.768kHz
* QSPI flash IS25LP064A(down to 1 MBytes)

## Midify script `target/imxrt.cfg `
* `adapter speed 4000`
* `flash bank imxrt plugin $FLASH_MEMORY_BASE 0x100000 4 4 $_TARGETNAME flash/IMXRT1050_QSPIFLASH_ROMAPI.elf`

## Plugin path
* `flash/IMXRT1050_QSPIFLASH_ROMAPI.elf`

## Build
### Require
* cmake (3.12)
* make (GNU Make 4.4)
* gcc toolchain (GNU Arm Embedded Toolchain 10.3-2021.10)

### Linux
* mkdir build
* cd build
* cmake ..
* make

### MinGW
* mkdir build
* cd build
* cmake -G "MinGW Makefiles" ..
* make

## Work Flow
When you start a debug session now, OpenOCD will automatically do the following:
* Reset the device and let the boot ROM initialize the peripherals
* Load the FLASH plugin into the RAM
* Use the FLASH plugin to erase and program the memory (note that due to the way OpenOCD implements FLASH drivers, the plugin will get unloaded and loaded again between the erase and program operations).
* If the currently debugged project contained any RAM sections, OpenOCD would load them after unloading the FLASH plugin. In either case, the FLASH plugin will be completely unloaded before the application gets a chance to run and will never collide with it.
* Set the $PC register to the application’s entry point and let it run. As a result, the application you are debugging will seamlessly get programmed into FLASH and you will be able to debug it.
