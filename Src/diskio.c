/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "SDCard.h"


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
extern struct SDCardPort sdport0;

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS Stat = STA_NOINIT;
	if(pdrv == 0){
		if(sdport0.state == INITIALIZED)
			Stat &= ~STA_NOINIT;
	}
    return Stat;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS Stat = STA_NOINIT;
	if(pdrv == 0){
		if(sdport0.state == INITIALIZED)
			Stat &= ~STA_NOINIT;
	}
	return Stat;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	UINT x = count+1;
	for(UINT i = 0;i<count;i++)
		if(SDReadData(&sdport0, (uint8_t*)buff+i*512, sector*512) != 0)
			return RES_ERROR;
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	UINT x = count+1;
	for(UINT i = 0;i<count;i++)
		if(SDWriteData(&sdport0, (uint8_t*)buff+i*sdport0.blockSize, sector*sdport0.blockSize) != 0)
			return RES_ERROR;
	return RES_OK;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	if (disk_status(pdrv) & STA_NOINIT) return RES_NOTRDY;
	res = RES_ERROR;
	switch (cmd)
	{
		case GET_SECTOR_SIZE : /* Get sectors on the disk (WORD) */
			*(WORD*)buff = (WORD)sdport0.blockSize;
			res = RES_OK;
			break;
		case CTRL_SYNC:
			res = RES_OK;
			break;
		default:
			res = RES_PARERR;
	}
	return res;
}

