#pragma once

#define FlexSpiInstance           0U
#define EXAMPLE_FLEXSPI_AMBA_BASE FlexSPI_AMBA_BASE
#define FLASH_SIZE                1U * 1024U * 1024U /* 8MByte */
#define FLASH_PAGE_SIZE           256U               /* 256Bytes */
#define FLASH_SECTOR_SIZE         4U * 1024U         /* 4KBytes */
#define FLASH_BLOCK_SIZE          64U * 1024U        /* 64KBytes */


#define MINIMUM_PROGRAMMED_BLOCK_SIZE FLASH_PAGE_SIZE
#define FLASH_PLUGIN_SUPPORT_ASYNC_PROGRAMMING 0