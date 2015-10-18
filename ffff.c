/*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
Set tab spacing to 4
Mettez les arrˆts … tous 4 colonnes
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

									FFFF.C
									ÍÍÍÍÍÍ

						A "fast false floppy formatter"

ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ*/

#pragma pack(1)

#define INCL_BASE
#define INCL_NOPM
#define INCL_DOSDEVICES
#include <stdio.h>
#include <os2.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define DirectoryEntrySize	32

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define	FALSE	0
#endif

char	drive[] = "a:";
typedef	UCHAR	FAT;
typedef FAT		*pFAT;

FAT *allocFAT(PBIOSPARAMETERBLOCK);
VOID readFAT (pFAT, PBIOSPARAMETERBLOCK, HFILE);
VOID clearFAT(pFAT, PBIOSPARAMETERBLOCK);
VOID writeFAT(pFAT, PBIOSPARAMETERBLOCK, HFILE);
VOID readSector(HFILE, USHORT, pFAT, PBIOSPARAMETERBLOCK);
VOID writeSector(HFILE, USHORT, pFAT, PBIOSPARAMETERBLOCK);
VOID scrubDir(HFILE, PBIOSPARAMETERBLOCK);
VOID Notify(char **, USHORT, USHORT);
VOID Notify2(char **, USHORT, USHORT, USHORT);
USHORT ErrorMap(USHORT);
USHORT Language(VOID);
TRACKLAYOUT *buildTrackLayout(PBIOSPARAMETERBLOCK);

FSINFO	FSInfo;
USHORT	DiskHasVolumeLabel, OldLabel;
USHORT	language;
TRACKLAYOUT *tlo = NULL;

#define FFFMSG(x) (language+x)
BYTE msgFile[] = "FFFF.MSG";

#define USA				  1
#define FRENCH_CANADA	  2
#define LATIN_AMERICA	  3
#define NETHERLANDS		 31
#define BELGIUM			 32
#define FRANCE			 33
#define SPAIN			 34
#define ITALY			 39
#define SWITZERLAND		 41
#define UNITED_KINGDOM	 44
#define DENMARK			 45
#define SWEDEN			 46
#define NORWAY			 47
#define GERMANY			 49
#define AUSTRALIA		 61
#define PORTUGAL		351
#define FINLAND			358

// Add new definitions as needed
#define ENGLISH			  0
#define	FRENCH			 40
#define DUTCH			 80

struct _language
{
	USHORT	country;
	USHORT	language;
} countryMap[] =
{
	{	USA,			ENGLISH	},
	{	UNITED_KINGDOM,	ENGLISH },
	{	AUSTRALIA,		ENGLISH	},
	{	FRENCH_CANADA,	FRENCH	},
	{	FRANCE,			FRENCH	},
	{	SWITZERLAND,	FRENCH	},
	{	NETHERLANDS,	DUTCH	},
	{	0,				0		}
};

/*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ


	
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ*/

VOID main(int argc, char *argv[])
{
	BYTE
		a = 0,
		bCommand = 0;
 	HFILE
		hDrive;
	USHORT
		usAction,
		rv;
	BIOSPARAMETERBLOCK
			bpb;
	FAT
		*pFAT;
	BYTE
		errx[3][30],
		*ppsMsg[3];
		
	ppsMsg[0] = errx[0];
	ppsMsg[1] = errx[1];
	ppsMsg[2] = errx[2];

	language = Language();
	Notify(ppsMsg,0,FFFMSG(1));		// Announce program title
	Notify(ppsMsg,0,FFFMSG(2));		// and version
	putchar('\n');
	if (argc > 1)
	{
		a = tolower(*argv[1]);
		if (a < 'a' | a > 'b')
			a = 0;
   	}
	if (a == 0)
	{
		Notify(ppsMsg,0,FFFMSG(3));
		exit(1);
	}
	drive[0] = a;

	rv = DosQFSInfo(a-'a'+1, 2, (PBYTE)&FSInfo, sizeof FSInfo);
	if (rv & rv != ERROR_NO_VOLUME_LABEL)
	{
		Notify2(ppsMsg,0,FFFMSG(4),rv);
		exit(1);
	}
	DiskHasVolumeLabel = !rv;
	if (argc > 2)
		if (argv[2][0] == '/' && tolower(argv[2][1]) == 'v')
			if (argc > 3)
			{
				DiskHasVolumeLabel = TRUE;
				OldLabel = FALSE;
				if (strlen(argv[3]) > 11)
					strncpy(FSInfo.vol.szVolLabel,argv[3],11);
				else
					strcpy(FSInfo.vol.szVolLabel,argv[3]);
				FSInfo.vol.cch = (BYTE)strlen(FSInfo.vol.szVolLabel);
			}
			else
				OldLabel = TRUE;
		else
			DiskHasVolumeLabel = FALSE;
	else
		DiskHasVolumeLabel = FALSE;

	if (rv=DosOpen(drive, &hDrive, &usAction, 0L, FILE_NORMAL, FILE_OPEN,
			OPEN_ACCESS_READWRITE |
			OPEN_SHARE_DENYREADWRITE |
			OPEN_FLAGS_DASD, 0L))
	{
		errx[0][0] = toupper(argv[1][0]);
		errx[0][1] = ':';
		errx[0][2] = '\0';
		Notify2(ppsMsg,1,FFFMSG(10),rv);
		exit(1);
	}

	DosDevIOCtl(0L, &bCommand, DSK_LOCKDRIVE, IOCTL_DISK, hDrive);
	bCommand=1;
	if (rv=DosDevIOCtl(&bpb, &bCommand,
						DSK_GETDEVICEPARAMS, IOCTL_DISK, hDrive))
	{
		errx[0][0] = toupper(argv[1][0]);
		errx[0][1] = ':';
		errx[0][2] = '\0';
		Notify2(ppsMsg,1,FFFMSG(22),rv);
		exit(1);
	}

	pFAT = allocFAT(&bpb);

	Notify(ppsMsg,0,FFFMSG(23));
	readFAT (pFAT, &bpb, hDrive);

	Notify(ppsMsg,0,FFFMSG(24));
	clearFAT(pFAT, &bpb);

	Notify(ppsMsg,0,FFFMSG(25));
	writeFAT(pFAT, &bpb, hDrive);

	Notify(ppsMsg,0,FFFMSG(26));
	scrubDir(hDrive, &bpb);

	if (DiskHasVolumeLabel)
	{
		Notify(ppsMsg,0,OldLabel ? FFFMSG(28) : FFFMSG(27));
		bCommand = 0;
		DosDevIOCtl(0L, &bCommand, DSK_REDETERMINEMEDIA, IOCTL_DISK, hDrive);
		DosDevIOCtl(0L, &bCommand, DSK_UNLOCKDRIVE, IOCTL_DISK, hDrive);
		DosClose(hDrive);
		DosSetFSInfo(a-'a'+1, 2, (PBYTE)(&FSInfo.vol), sizeof(FSInfo.vol));
	}
	else
	{
		DosDevIOCtl(0L, &bCommand, DSK_UNLOCKDRIVE, IOCTL_DISK, hDrive);
		DosClose(hDrive);
	}
}

/*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

	Allocate sufficient memory to hold a copy of the FAT
	
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ*/

FAT *allocFAT(BIOSPARAMETERBLOCK *bpb)
{
	FAT		*f;

	f = (pFAT)malloc(bpb->usBytesPerSector * bpb->cFATs * bpb->usSectorsPerFAT);
	if (f == NULL)
	{
		Notify(NULL,0,FFFMSG(29));
		exit(1);
	}
	return f;
}

/*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

	Read the File Allocation Table into (allocated) memory
	
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ*/

void readFAT(FAT *f, BIOSPARAMETERBLOCK *bpb, HFILE hDrive)
{
	USHORT	sector;

	for (sector=0; sector < bpb->usSectorsPerFAT; ++sector)
		readSector(hDrive, sector+1, f+(sector * bpb->usBytesPerSector), bpb);
}

/*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

	Build a skeleton track layout table
	
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ*/

TRACKLAYOUT *buildTrackLayout(BIOSPARAMETERBLOCK *bpb)
{
	TRACKLAYOUT
		*t;
	USHORT
		x;

	t = (PTRACKLAYOUT)malloc(sizeof(TRACKLAYOUT)
							   +(bpb->usSectorsPerTrack - 1)*4);

	t->bCommand = 0;

	for(x=0; x < bpb->usSectorsPerTrack; ++x)
	{
		t->TrackTable[x].usSectorNumber = x+1;
		t->TrackTable[x].usSectorSize = bpb->usBytesPerSector;
	}
	return t;
}

/*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

	Read a single sector from a diskette
	
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ*/

void readSector(HFILE hDrive, USHORT sector, FAT *f, BIOSPARAMETERBLOCK *bpb)
{
	USHORT rv;

	if (tlo == NULL)
		tlo = buildTrackLayout(bpb);

	tlo->usHead = (sector % (bpb->usSectorsPerTrack * bpb->cHeads))
				 / bpb->usSectorsPerTrack;
	tlo->usCylinder = sector / (bpb->usSectorsPerTrack * bpb->cHeads);
	tlo->usFirstSector = sector % bpb->usSectorsPerTrack;
	tlo->cSectors = 1;

	if (rv=DosDevIOCtl(f, tlo, DSK_READTRACK, IOCTL_DISK, hDrive))
	{
		Notify2(NULL,0,FFFMSG(30),rv);
		exit(1);
	}
}

/*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

	Clear out all entries in the FAT which are not marked bad.
	
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ*/

void clearFAT(FAT *f, BIOSPARAMETERBLOCK *bpb)
{
	USHORT	usEntriesInFAT,
			usEntryNumber,
			usEntry,
			FATword,
			*pEntry;

	usEntriesInFAT = bpb->usSectorsPerFAT * bpb->usBytesPerSector * 2 / 3;
	for (usEntryNumber = 2; usEntryNumber < usEntriesInFAT; ++usEntryNumber)
	{
		pEntry = (PUSHORT)(f+(usEntryNumber*3/2));
		FATword = usEntry = *pEntry;
		if (usEntryNumber & 1)
			usEntry >>= 4;
		if ((usEntry &= 0x0FFF) != 0xFF7)
			usEntry = 0;
		if (usEntryNumber & 1)
			FATword = (FATword & 0x000F) | (usEntry << 4);
		else
			FATword = (FATword & 0xF000) | usEntry;
		*pEntry = FATword;
	}
}

/*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ


	
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ*/

void writeFAT(FAT *f, BIOSPARAMETERBLOCK *bpb, HFILE hDrive)
{
	USHORT	sector, secBase;
	BYTE	fat;
	for (fat=0; fat < bpb->cFATs; ++fat)
	{
		secBase = fat * bpb->usSectorsPerFAT + 1;
		for (sector=0; sector < bpb->usSectorsPerFAT; ++sector)
			writeSector(hDrive, sector+secBase,
						f+(sector * bpb->usBytesPerSector), bpb);
	}
}

/*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ


	
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ*/

void writeSector(HFILE hDrive, USHORT sector, FAT *f, BIOSPARAMETERBLOCK *bpb)
{
	USHORT rv;

	if (tlo == NULL)
		tlo = buildTrackLayout(bpb);

	tlo->usHead = (sector % (bpb->usSectorsPerTrack * bpb->cHeads))
				 / bpb->usSectorsPerTrack;
	tlo->usCylinder = sector / (bpb->usSectorsPerTrack * bpb->cHeads);
	tlo->usFirstSector = sector % bpb->usSectorsPerTrack;
	tlo->cSectors = 1;

	if (rv=DosDevIOCtl(f, tlo, DSK_WRITETRACK, IOCTL_DISK, hDrive))
	{
		Notify2(NULL,0,FFFMSG(31),rv);
		exit(1);
	}
}

/*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

	Clear the root directory of a floppy disk
	
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ*/

void scrubDir(HFILE hDrive, BIOSPARAMETERBLOCK *bpb)
{
	BYTE	*secBuf;
	USHORT	sector, secBase;

	secBuf = calloc(1,bpb->usBytesPerSector);
	
	secBase = bpb->cFATs * bpb->usSectorsPerFAT + 1;
	for (sector=0; sector < bpb->cRootEntries / DirectoryEntrySize; ++sector)
		writeSector(hDrive, sector+secBase, secBuf, bpb);
}

/*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

	Notify() retrieves and displays a single message from the message
	segment or file.
	
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ*/

VOID Notify(char **ppsMsg, USHORT fields, USHORT MessageNumber)
{
	USHORT msgLen;
	char *message;

	if ((message=malloc(200)) != NULL)
	{
		DosGetMessage(ppsMsg,fields,message,200,MessageNumber,msgFile,&msgLen);
		DosPutMessage(fileno(stdout),msgLen,message);
		free(message);
	}
	else
		puts("Trouble getting memory for message display");
}

/*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

	Notify2() retrieves a primary and secondary message from the message
	segment or file and displays both.
	
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ*/

VOID Notify2(char **ppsMsg, USHORT fields,
			USHORT MessageNumber, USHORT SecondaryMessageNumber)
{
	USHORT msgLen, msg2Len;
	char *message;

	if ((message=malloc(400)) != NULL)
	{
		DosGetMessage(ppsMsg, fields, message,     200,
					  MessageNumber,                    msgFile, &msgLen);
		DosGetMessage(ppsMsg, 0,      message+200, 200,
					  ErrorMap(SecondaryMessageNumber), msgFile, &msg2Len);
		strncpy(message+msgLen, message+200, msg2Len); 
		DosPutMessage(fileno(stdout), msgLen+msg2Len, message);
		free(message);
	}
	else
		puts("Trouble getting memory for message display");
}

/*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ


	
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ*/

USHORT ErrorMap(USHORT rv)
{
	switch (rv)
	{
case ERROR_BUFFER_OVERFLOW:			rv = FFFMSG(5);  break;
case ERROR_INVALID_DRIVE:			rv = FFFMSG(6);  break;
case ERROR_INVALID_LEVEL:			rv = FFFMSG(7);  break;
case ERROR_NO_VOLUME_LABEL:			rv = FFFMSG(8);  break;
case ERROR_ACCESS_DENIED:			rv = FFFMSG(11); break;
case ERROR_DRIVE_LOCKED:			rv = FFFMSG(12); break;
case ERROR_FILE_NOT_FOUND:			rv = FFFMSG(13); break;
case ERROR_INVALID_ACCESS:			rv = FFFMSG(14); break;
case ERROR_INVALID_PARAMETER:		rv = FFFMSG(15); break;
case ERROR_NOT_DOS_DISK:			rv = FFFMSG(16); break;
case ERROR_OPEN_FAILED:				rv = FFFMSG(17); break;
case ERROR_PATH_NOT_FOUND:			rv = FFFMSG(18); break;
case ERROR_SHARING_BUFFER_EXCEEDED:	rv = FFFMSG(19); break;
case ERROR_SHARING_VIOLATION:		rv = FFFMSG(20); break;
case ERROR_TOO_MANY_OPEN_FILES:		rv = FFFMSG(21); break;
default:					 		rv = FFFMSG(9);  break;
	}
	return rv;
}

/*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ


	
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ*/

USHORT Language(VOID)
{
	COUNTRYCODE cc;
 	COUNTRYINFO ci;
	USHORT		cil, rv;

	cc.country = cc.codepage = 0;
	if (rv=DosGetCtryInfo(sizeof ci, &cc, &ci, &cil))
	{
		printf("Error %u getting country code\n\n", rv);
		return 0;
	}

//	printf("Country=%u; codepage=%u\n\n", ci.country, ci.codepage);
	for (rv = 0; countryMap[rv].country; ++rv)
		if (ci.country == countryMap[rv].country)
			return countryMap[rv].language;
	return 0;
}
