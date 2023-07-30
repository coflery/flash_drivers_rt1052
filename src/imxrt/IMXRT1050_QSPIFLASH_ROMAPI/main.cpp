/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_romapi.h"
#include "fsl_cache.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_common.h"
#include <FLASHPluginInterface.h>
#include <FLASHPluginConfig.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/* Get FlexSPI NOR Configuration Block */
void FLEXSPI_NorFlash_GetConfig(flexspi_nor_config_t *config);
void error_trap(void);

#pragma region Functions from the SDK
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief FLEXSPI NOR flash driver Structure */
static flexspi_nor_config_t norConfig;


/*******************************************************************************
 * Code
 ******************************************************************************/
/* Get FLEXSPI NOR Configuration Block */
void FLEXSPI_NorFlash_GetConfig(flexspi_nor_config_t *config)
{
    config->memConfig.tag              = FLEXSPI_CFG_BLK_TAG;
    config->memConfig.version          = FLEXSPI_CFG_BLK_VERSION;
    config->memConfig.readSampleClkSrc = kFLEXSPIReadSampleClk_LoopbackFromSckPad;
    config->memConfig.serialClkFreq = kFLEXSPISerialClk_100MHz; /* Serial Flash Frequencey.See System Boot Chapter for more details */
    config->memConfig.lutCustomSeqEnable = false;
    config->memConfig.sflashA1Size       = FLASH_SIZE;
    config->memConfig.csHoldTime         = 3U;                       /* Data hold time, default value: 3 */
    config->memConfig.csSetupTime        = 3U;                       /* Date setup time, default value: 3 */
    config->memConfig.deviceType     = kFLEXSPIDeviceType_SerialNOR; /* Flash device type default type: Serial NOR */
    /* Always enable Safe configuration Frequency */
    config->memConfig.controllerMiscOption = FSL_ROM_FLEXSPI_BITMASK(kFLEXSPIMiscOffset_SafeConfigFreqEnable);
    config->memConfig.sflashPadType = kSerialFlash_4Pads; /* Pad Type: 1 - Single, 2 - Dual, 4 - Quad, 8 - Octal */
    config->pageSize                = FLASH_PAGE_SIZE;
    config->sectorSize              = FLASH_SECTOR_SIZE;
    config->blockSize               = FLASH_BLOCK_SIZE;
    config->isUniformBlockSize      = false;
    config->ipcmdSerialClkFreq      = kFLEXSPISerialClk_50MHz; /* Clock frequency for IP command */

    // QSPI Read
    config->memConfig.lookupTable[NOR_CMD_LUT_SEQ_IDX_READ] = FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0xEB, RADDR_SDR, FLEXSPI_4PAD, 24);
    config->memConfig.lookupTable[NOR_CMD_LUT_SEQ_IDX_READ + 1] = FSL_ROM_FLEXSPI_LUT_SEQ(DUMMY_SDR, FLEXSPI_4PAD, 6, READ_SDR, FLEXSPI_4PAD, 4);
    // Read Status
    config->memConfig.lookupTable[4 * NOR_CMD_LUT_SEQ_IDX_READSTATUS] = FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x05, READ_SDR, FLEXSPI_1PAD, 4);
    // Write Enable
    config->memConfig.lookupTable[4 * NOR_CMD_LUT_SEQ_IDX_WRITEENABLE] = FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x06, STOP, FLEXSPI_1PAD, 0);
    // Erase Sector
    config->memConfig.lookupTable[4 * NOR_CMD_LUT_SEQ_IDX_ERASESECTOR] = FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x20, RADDR_SDR, FLEXSPI_1PAD, 24);
    // Erase Block 64KB
    config->memConfig.lookupTable[4 * NOR_CMD_LUT_SEQ_IDX_ERASEBLOCK] = FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0xD8, RADDR_SDR, FLEXSPI_1PAD, 24);
    // Page Program
    config->memConfig.lookupTable[4 * NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM] = FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x02, RADDR_SDR, FLEXSPI_1PAD, 24);
    config->memConfig.lookupTable[4 * NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM + 1] = FSL_ROM_FLEXSPI_LUT_SEQ(WRITE_SDR, FLEXSPI_1PAD, 0x04, STOP, FLEXSPI_1PAD, 0);
    // Erase Chip
    config->memConfig.lookupTable[4 * NOR_CMD_LUT_SEQ_IDX_CHIPERASE] = FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x60, STOP, FLEXSPI_1PAD, 0);
    // Dummy
    config->memConfig.lookupTable[4 * NOR_CMD_LUT_SEQ_IDX_EXIT_NOCMD] = 0;
    // Write Status Reg
    config->memConfig.lookupTable[4 * NOR_CMD_LUT_SEQ_IDX_READSTATUS_XPI] = FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x01, WRITE_SDR, FLEXSPI_1PAD, 8);
}

void error_trap(void)
{
	asm("bkpt 255");
}

#pragma endregion

FLASHBankInfo FLASHPlugin_Probe(unsigned base, unsigned size, unsigned width1, unsigned width2)
{
	FLASHBankInfo result = {
		.BaseAddress = base, 
		.BlockCount = norConfig.memConfig.sflashA1Size /  norConfig.sectorSize,
		.BlockSize = norConfig.sectorSize,
		.WriteBlockSize = MINIMUM_PROGRAMMED_BLOCK_SIZE
	};
	
	if (norConfig.pageSize != MINIMUM_PROGRAMMED_BLOCK_SIZE)
		error_trap();
	
	return result;
}

WorkAreaInfo FLASHPlugin_FindWorkArea(void *endOfStack)
{
	InterruptEnabler enabler;
    
	WorkAreaInfo info = { .Address = endOfStack, .Size = 4096 };
	return info;
}

int FLASHPlugin_EraseSectors(unsigned firstSector, unsigned sectorCount)
{
	status_t status = ROM_FLEXSPI_NorFlash_Erase(FlexSpiInstance, &norConfig, firstSector * norConfig.sectorSize, sectorCount * norConfig.sectorSize);
	if (status != kStatus_Success)
		error_trap();

	return sectorCount;
}

int FLASHPlugin_Unload()
{
	return 0;
}

int FLASHPlugin_DoProgramSync(unsigned startOffset, const void *pData, int bytesToWrite)
{
	int sectors = bytesToWrite / FLASH_PAGE_SIZE;
	for (int i = 0; i < sectors; i++)
	{
		status_t status = ROM_FLEXSPI_NorFlash_ProgramPage(FlexSpiInstance,
			&norConfig,
			startOffset + i * FLASH_PAGE_SIZE,
			(const uint32_t *)((const char *)pData + i  * FLASH_PAGE_SIZE));
		
		if (status != kStatus_Success)
			return i * FLASH_PAGE_SIZE;
	}
	
	return sectors * FLASH_PAGE_SIZE;
}

int main(void)
{
    // BOARD_ConfigMPU();
    BOARD_InitPins();
    // BOARD_BootClockRUN();

    memset(&norConfig, 0, sizeof(flexspi_nor_config_t));

    FLEXSPI_NorFlash_GetConfig(&norConfig);
    status_t status = ROM_FLEXSPI_NorFlash_Init(FlexSpiInstance, &norConfig);
    if (status != kStatus_Success)
        error_trap();

    /* Perform software reset after initializing flexspi module */
    ROM_FLEXSPI_NorFlash_ClearCache(FlexSpiInstance);

    FLASHPlugin_InitDone();

#ifdef DEBUG
    TestFLASHProgramming(0x60000000, 0);
#endif

    asm("bkpt 255");

    return 0;
}
