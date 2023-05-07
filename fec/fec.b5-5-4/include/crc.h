/*****************************************************************************

Filename:   include/crc.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
History: (top down change order, newest to oldest changes)
YYYY.MM.DD --- developer ---    ----------------- Comments -------------------
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/


#ifndef _CRC_H
#define _CRC_H

// UINT64
// Initialize a CRC accumulator
extern void Crc64Init(UINT64 *crc);

// Finish a CRC calculation
extern void Crc64Fin(UINT64 *crc);

// Accumulate some (more) bytes into a CRC Block
extern void Crc64Accum(UINT64 *crc, unsigned char *data, unsigned int len);

// Calc a CRC Block
extern UINT64 Crc64Block(unsigned char *data, unsigned int len);
extern UINT64 Crc64String(unsigned char *string);

// Calc a CRC from file
extern UINT64 Crc64File(FILE *file);

// UINT32
extern UINT32 Crc32Block (unsigned char *ptr, int len);
extern UINT32 Crc32String (unsigned char *string);

#endif
