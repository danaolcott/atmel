/*-----------------------------------------------------------------------
/  Low level disk interface module include file   (C)ChaN, 2014
/-----------------------------------------------------------------------*/

#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////
//FATFS Enable Defines
#define _USE_WRITE	1	/* 1: Enable disk_write() function */
#define _USE_IOCTL	1	/* 1: Enable disk_ioctl() fucntion */

#include "integer.h"


//////////////////////////////////
#define	MMC_CD		1//!(GPIOC_IDR & _BV(4))	/* Card detect (yes:true, no:false, default:true) */
#define	MMC_WP		0 /* Write protected (yes:true, no:false, default:false) */

#define ON     1
#define OFF    0

/////////////////////////////////////////////
//Commands
/* MMC/SD command */
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND (MMC) */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT (MMC) */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD32	(32)		/* ERASE_ER_BLK_START */
#define CMD33	(33)		/* ERASE_ER_BLK_END */
#define CMD38	(38)		/* ERASE */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */


//////////////////////////////////////////
/* Disk Status Bits (DSTATUS) */
#define STA_NOINIT		0x01	/* Drive not initialized */
#define STA_NODISK		0x02	/* No medium in the drive */
#define STA_PROTECT		0x04	/* Write protected */


/* Command code for disk_ioctrl fucntion */

/* Generic command (Used by FatFs) */
#define CTRL_SYNC			0	/* Complete pending write process (needed at _FS_READONLY == 0) */
#define GET_SECTOR_COUNT	1	/* Get media size (needed at _USE_MKFS == 1) */
#define GET_SECTOR_SIZE		2	/* Get sector size (needed at _MAX_SS != _MIN_SS) */
#define GET_BLOCK_SIZE		3	/* Get erase block size (needed at _USE_MKFS == 1) */
#define CTRL_TRIM			4	/* Inform device that the data on the block of sectors is no longer used (needed at _USE_TRIM == 1) */

/* Generic command (Not used by FatFs) */
#define CTRL_FORMAT			5	/* Create physical format on the media */
#define CTRL_POWER_IDLE		6	/* Put the device idle state */
#define CTRL_POWER_OFF		7	/* Put the device off state */
#define CTRL_LOCK			8	/* Lock media removal */
#define CTRL_UNLOCK			9	/* Unlock media removal */
#define CTRL_EJECT			10	/* Eject media */

/* MMC/SDC specific command (Not used by FatFs) */
#define MMC_GET_TYPE		50	/* Get card type */
#define MMC_GET_CSD			51	/* Get CSD */
#define MMC_GET_CID			52	/* Get CID */
#define MMC_GET_OCR			53	/* Get OCR */
#define MMC_GET_SDSTAT		54	/* Get SD status */

/* ATA/CF specific command (Not used by FatFs) */
#define ATA_GET_REV			60	/* Get F/W revision */
#define ATA_GET_MODEL		61	/* Get model name */
#define ATA_GET_SN			62	/* Get serial number */


/* MMC card type flags (MMC_GET_TYPE) */
#define CT_MMC		0x01		/* MMC ver 3 */
#define CT_SD1		0x02		/* SD ver 1 */
#define CT_SD2		0x04		/* SD ver 2 */
#define CT_SDC		(CT_SD1|CT_SD2)	/* SD */
#define CT_BLOCK	0x08		/* Block addressing */



//////////////////////////////////////
//Type Defines
//
/* Status of Disk Functions */
typedef BYTE	DSTATUS;

/* Results of Disk Functions */
typedef enum {
	RES_OK = 0,		/* 0: Successful */
	RES_ERROR,		/* 1: R/W Error */
	RES_WRPRT,		/* 2: Write Protected */
	RES_NOTRDY,		/* 3: Not Ready */
	RES_PARERR		/* 4: Invalid Parameter */
} DRESULT;


///////////////////////////////////////
//Globals / externs
//Status variables and timer/timeout
//used by a few files
//variables
extern volatile DSTATUS Stat; // = STA_NOINIT;	/* Physical drive status */
extern volatile UINT Timer1, Timer2;        /* 1kHz decrement timer stopped at zero (disk_timerproc()) */
extern BYTE CardType;                       /* Card type flags */



////////////////////////////////////////////////////
/*---------------------------------------*/
/* Prototypes for disk control functions */
/*---------------------------------------*/

DSTATUS disk_initialize (BYTE pdrv);
DSTATUS disk_status (BYTE pdrv);
DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
#if	_USE_WRITE
DRESULT disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
#endif
#if	_USE_IOCTL
DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff);
#endif


#ifdef __cplusplus
}
#endif

#endif
