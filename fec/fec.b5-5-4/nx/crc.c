/*****************************************************************************

Filename:   nx/crc.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
History: (top down change order, newest to oldest changes)
YYYY.MM.DD --- developer ---    ----------------- Comments -------------------
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/


#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/crc.h"


#define UINT64CONST(x) ((UINT64) x##ULL)

static const UINT64 Crc64Table[256] =
{
	UINT64CONST(0x0000000000000000), UINT64CONST(0x42F0E1EBA9EA3693),
	UINT64CONST(0x85E1C3D753D46D26), UINT64CONST(0xC711223CFA3E5BB5),
	UINT64CONST(0x493366450E42ECDF), UINT64CONST(0x0BC387AEA7A8DA4C),
	UINT64CONST(0xCCD2A5925D9681F9), UINT64CONST(0x8E224479F47CB76A),
	UINT64CONST(0x9266CC8A1C85D9BE), UINT64CONST(0xD0962D61B56FEF2D),
	UINT64CONST(0x17870F5D4F51B498), UINT64CONST(0x5577EEB6E6BB820B),
	UINT64CONST(0xDB55AACF12C73561), UINT64CONST(0x99A54B24BB2D03F2),
	UINT64CONST(0x5EB4691841135847), UINT64CONST(0x1C4488F3E8F96ED4),
	UINT64CONST(0x663D78FF90E185EF), UINT64CONST(0x24CD9914390BB37C),
	UINT64CONST(0xE3DCBB28C335E8C9), UINT64CONST(0xA12C5AC36ADFDE5A),
	UINT64CONST(0x2F0E1EBA9EA36930), UINT64CONST(0x6DFEFF5137495FA3),
	UINT64CONST(0xAAEFDD6DCD770416), UINT64CONST(0xE81F3C86649D3285),
	UINT64CONST(0xF45BB4758C645C51), UINT64CONST(0xB6AB559E258E6AC2),
	UINT64CONST(0x71BA77A2DFB03177), UINT64CONST(0x334A9649765A07E4),
	UINT64CONST(0xBD68D2308226B08E), UINT64CONST(0xFF9833DB2BCC861D),
	UINT64CONST(0x388911E7D1F2DDA8), UINT64CONST(0x7A79F00C7818EB3B),
	UINT64CONST(0xCC7AF1FF21C30BDE), UINT64CONST(0x8E8A101488293D4D),
	UINT64CONST(0x499B3228721766F8), UINT64CONST(0x0B6BD3C3DBFD506B),
	UINT64CONST(0x854997BA2F81E701), UINT64CONST(0xC7B97651866BD192),
	UINT64CONST(0x00A8546D7C558A27), UINT64CONST(0x4258B586D5BFBCB4),
	UINT64CONST(0x5E1C3D753D46D260), UINT64CONST(0x1CECDC9E94ACE4F3),
	UINT64CONST(0xDBFDFEA26E92BF46), UINT64CONST(0x990D1F49C77889D5),
	UINT64CONST(0x172F5B3033043EBF), UINT64CONST(0x55DFBADB9AEE082C),
	UINT64CONST(0x92CE98E760D05399), UINT64CONST(0xD03E790CC93A650A),
	UINT64CONST(0xAA478900B1228E31), UINT64CONST(0xE8B768EB18C8B8A2),
	UINT64CONST(0x2FA64AD7E2F6E317), UINT64CONST(0x6D56AB3C4B1CD584),
	UINT64CONST(0xE374EF45BF6062EE), UINT64CONST(0xA1840EAE168A547D),
	UINT64CONST(0x66952C92ECB40FC8), UINT64CONST(0x2465CD79455E395B),
	UINT64CONST(0x3821458AADA7578F), UINT64CONST(0x7AD1A461044D611C),
	UINT64CONST(0xBDC0865DFE733AA9), UINT64CONST(0xFF3067B657990C3A),
	UINT64CONST(0x711223CFA3E5BB50), UINT64CONST(0x33E2C2240A0F8DC3),
	UINT64CONST(0xF4F3E018F031D676), UINT64CONST(0xB60301F359DBE0E5),
	UINT64CONST(0xDA050215EA6C212F), UINT64CONST(0x98F5E3FE438617BC),
	UINT64CONST(0x5FE4C1C2B9B84C09), UINT64CONST(0x1D14202910527A9A),
	UINT64CONST(0x93366450E42ECDF0), UINT64CONST(0xD1C685BB4DC4FB63),
	UINT64CONST(0x16D7A787B7FAA0D6), UINT64CONST(0x5427466C1E109645),
	UINT64CONST(0x4863CE9FF6E9F891), UINT64CONST(0x0A932F745F03CE02),
	UINT64CONST(0xCD820D48A53D95B7), UINT64CONST(0x8F72ECA30CD7A324),
	UINT64CONST(0x0150A8DAF8AB144E), UINT64CONST(0x43A04931514122DD),
	UINT64CONST(0x84B16B0DAB7F7968), UINT64CONST(0xC6418AE602954FFB),
	UINT64CONST(0xBC387AEA7A8DA4C0), UINT64CONST(0xFEC89B01D3679253),
	UINT64CONST(0x39D9B93D2959C9E6), UINT64CONST(0x7B2958D680B3FF75),
	UINT64CONST(0xF50B1CAF74CF481F), UINT64CONST(0xB7FBFD44DD257E8C),
	UINT64CONST(0x70EADF78271B2539), UINT64CONST(0x321A3E938EF113AA),
	UINT64CONST(0x2E5EB66066087D7E), UINT64CONST(0x6CAE578BCFE24BED),
	UINT64CONST(0xABBF75B735DC1058), UINT64CONST(0xE94F945C9C3626CB),
	UINT64CONST(0x676DD025684A91A1), UINT64CONST(0x259D31CEC1A0A732),
	UINT64CONST(0xE28C13F23B9EFC87), UINT64CONST(0xA07CF2199274CA14),
	UINT64CONST(0x167FF3EACBAF2AF1), UINT64CONST(0x548F120162451C62),
	UINT64CONST(0x939E303D987B47D7), UINT64CONST(0xD16ED1D631917144),
	UINT64CONST(0x5F4C95AFC5EDC62E), UINT64CONST(0x1DBC74446C07F0BD),
	UINT64CONST(0xDAAD56789639AB08), UINT64CONST(0x985DB7933FD39D9B),
	UINT64CONST(0x84193F60D72AF34F), UINT64CONST(0xC6E9DE8B7EC0C5DC),
	UINT64CONST(0x01F8FCB784FE9E69), UINT64CONST(0x43081D5C2D14A8FA),
	UINT64CONST(0xCD2A5925D9681F90), UINT64CONST(0x8FDAB8CE70822903),
	UINT64CONST(0x48CB9AF28ABC72B6), UINT64CONST(0x0A3B7B1923564425),
	UINT64CONST(0x70428B155B4EAF1E), UINT64CONST(0x32B26AFEF2A4998D),
	UINT64CONST(0xF5A348C2089AC238), UINT64CONST(0xB753A929A170F4AB),
	UINT64CONST(0x3971ED50550C43C1), UINT64CONST(0x7B810CBBFCE67552),
	UINT64CONST(0xBC902E8706D82EE7), UINT64CONST(0xFE60CF6CAF321874),
	UINT64CONST(0xE224479F47CB76A0), UINT64CONST(0xA0D4A674EE214033),
	UINT64CONST(0x67C58448141F1B86), UINT64CONST(0x253565A3BDF52D15),
	UINT64CONST(0xAB1721DA49899A7F), UINT64CONST(0xE9E7C031E063ACEC),
	UINT64CONST(0x2EF6E20D1A5DF759), UINT64CONST(0x6C0603E6B3B7C1CA),
	UINT64CONST(0xF6FAE5C07D3274CD), UINT64CONST(0xB40A042BD4D8425E),
	UINT64CONST(0x731B26172EE619EB), UINT64CONST(0x31EBC7FC870C2F78),
	UINT64CONST(0xBFC9838573709812), UINT64CONST(0xFD39626EDA9AAE81),
	UINT64CONST(0x3A28405220A4F534), UINT64CONST(0x78D8A1B9894EC3A7),
	UINT64CONST(0x649C294A61B7AD73), UINT64CONST(0x266CC8A1C85D9BE0),
	UINT64CONST(0xE17DEA9D3263C055), UINT64CONST(0xA38D0B769B89F6C6),
	UINT64CONST(0x2DAF4F0F6FF541AC), UINT64CONST(0x6F5FAEE4C61F773F),
	UINT64CONST(0xA84E8CD83C212C8A), UINT64CONST(0xEABE6D3395CB1A19),
	UINT64CONST(0x90C79D3FEDD3F122), UINT64CONST(0xD2377CD44439C7B1),
	UINT64CONST(0x15265EE8BE079C04), UINT64CONST(0x57D6BF0317EDAA97),
	UINT64CONST(0xD9F4FB7AE3911DFD), UINT64CONST(0x9B041A914A7B2B6E),
	UINT64CONST(0x5C1538ADB04570DB), UINT64CONST(0x1EE5D94619AF4648),
	UINT64CONST(0x02A151B5F156289C), UINT64CONST(0x4051B05E58BC1E0F),
	UINT64CONST(0x87409262A28245BA), UINT64CONST(0xC5B073890B687329),
	UINT64CONST(0x4B9237F0FF14C443), UINT64CONST(0x0962D61B56FEF2D0),
	UINT64CONST(0xCE73F427ACC0A965), UINT64CONST(0x8C8315CC052A9FF6),
	UINT64CONST(0x3A80143F5CF17F13), UINT64CONST(0x7870F5D4F51B4980),
	UINT64CONST(0xBF61D7E80F251235), UINT64CONST(0xFD913603A6CF24A6),
	UINT64CONST(0x73B3727A52B393CC), UINT64CONST(0x31439391FB59A55F),
	UINT64CONST(0xF652B1AD0167FEEA), UINT64CONST(0xB4A25046A88DC879),
	UINT64CONST(0xA8E6D8B54074A6AD), UINT64CONST(0xEA16395EE99E903E),
	UINT64CONST(0x2D071B6213A0CB8B), UINT64CONST(0x6FF7FA89BA4AFD18),
	UINT64CONST(0xE1D5BEF04E364A72), UINT64CONST(0xA3255F1BE7DC7CE1),
	UINT64CONST(0x64347D271DE22754), UINT64CONST(0x26C49CCCB40811C7),
	UINT64CONST(0x5CBD6CC0CC10FAFC), UINT64CONST(0x1E4D8D2B65FACC6F),
	UINT64CONST(0xD95CAF179FC497DA), UINT64CONST(0x9BAC4EFC362EA149),
	UINT64CONST(0x158E0A85C2521623), UINT64CONST(0x577EEB6E6BB820B0),
	UINT64CONST(0x906FC95291867B05), UINT64CONST(0xD29F28B9386C4D96),
	UINT64CONST(0xCEDBA04AD0952342), UINT64CONST(0x8C2B41A1797F15D1),
	UINT64CONST(0x4B3A639D83414E64), UINT64CONST(0x09CA82762AAB78F7),
	UINT64CONST(0x87E8C60FDED7CF9D), UINT64CONST(0xC51827E4773DF90E),
	UINT64CONST(0x020905D88D03A2BB), UINT64CONST(0x40F9E43324E99428),
	UINT64CONST(0x2CFFE7D5975E55E2), UINT64CONST(0x6E0F063E3EB46371),
	UINT64CONST(0xA91E2402C48A38C4), UINT64CONST(0xEBEEC5E96D600E57),
	UINT64CONST(0x65CC8190991CB93D), UINT64CONST(0x273C607B30F68FAE),
	UINT64CONST(0xE02D4247CAC8D41B), UINT64CONST(0xA2DDA3AC6322E288),
	UINT64CONST(0xBE992B5F8BDB8C5C), UINT64CONST(0xFC69CAB42231BACF),
	UINT64CONST(0x3B78E888D80FE17A), UINT64CONST(0x7988096371E5D7E9),
	UINT64CONST(0xF7AA4D1A85996083), UINT64CONST(0xB55AACF12C735610),
	UINT64CONST(0x724B8ECDD64D0DA5), UINT64CONST(0x30BB6F267FA73B36),
	UINT64CONST(0x4AC29F2A07BFD00D), UINT64CONST(0x08327EC1AE55E69E),
	UINT64CONST(0xCF235CFD546BBD2B), UINT64CONST(0x8DD3BD16FD818BB8),
	UINT64CONST(0x03F1F96F09FD3CD2), UINT64CONST(0x41011884A0170A41),
	UINT64CONST(0x86103AB85A2951F4), UINT64CONST(0xC4E0DB53F3C36767),
	UINT64CONST(0xD8A453A01B3A09B3), UINT64CONST(0x9A54B24BB2D03F20),
	UINT64CONST(0x5D45907748EE6495), UINT64CONST(0x1FB5719CE1045206),
	UINT64CONST(0x919735E51578E56C), UINT64CONST(0xD367D40EBC92D3FF),
	UINT64CONST(0x1476F63246AC884A), UINT64CONST(0x568617D9EF46BED9),
	UINT64CONST(0xE085162AB69D5E3C), UINT64CONST(0xA275F7C11F7768AF),
	UINT64CONST(0x6564D5FDE549331A), UINT64CONST(0x279434164CA30589),
	UINT64CONST(0xA9B6706FB8DFB2E3), UINT64CONST(0xEB46918411358470),
	UINT64CONST(0x2C57B3B8EB0BDFC5), UINT64CONST(0x6EA7525342E1E956),
	UINT64CONST(0x72E3DAA0AA188782), UINT64CONST(0x30133B4B03F2B111),
	UINT64CONST(0xF7021977F9CCEAA4), UINT64CONST(0xB5F2F89C5026DC37),
	UINT64CONST(0x3BD0BCE5A45A6B5D), UINT64CONST(0x79205D0E0DB05DCE),
	UINT64CONST(0xBE317F32F78E067B), UINT64CONST(0xFCC19ED95E6430E8),
	UINT64CONST(0x86B86ED5267CDBD3), UINT64CONST(0xC4488F3E8F96ED40),
	UINT64CONST(0x0359AD0275A8B6F5), UINT64CONST(0x41A94CE9DC428066),
	UINT64CONST(0xCF8B0890283E370C), UINT64CONST(0x8D7BE97B81D4019F),
	UINT64CONST(0x4A6ACB477BEA5A2A), UINT64CONST(0x089A2AACD2006CB9),
	UINT64CONST(0x14DEA25F3AF9026D), UINT64CONST(0x562E43B4931334FE),
	UINT64CONST(0x913F6188692D6F4B), UINT64CONST(0xD3CF8063C0C759D8),
	UINT64CONST(0x5DEDC41A34BBEEB2), UINT64CONST(0x1F1D25F19D51D821),
	UINT64CONST(0xD80C07CD676F8394), UINT64CONST(0x9AFCE626CE85B507)
};


/*
 * If we have a 64-bit integer type, then a 64-bit CRC looks just like the
 * usual sort of implementation.  (See Ross Williams' excellent introduction
 * A PAINLESS GUIDE TO CRC ERROR DETECTION ALGORITHMS, available from
 * ftp://ftp.rocksoft.com/papers/crc_v3.txt or several other net sites.)
 * If we have no working 64-bit type, then fake it with two 32-bit registers.
 *
 * The present implementation is a normal (not "reflected", in Williams'
 * terms) 64-bit CRC, using initial all-ones register contents and a final
 * bit inversion.  The chosen polynomial is borrowed from the DLT1 spec
 * (ECMA-182, available from http://www.ecma.ch/ecma1/STAND/ECMA-182.HTM):
 *
 * x^64 + x^62 + x^57 + x^55 + x^54 + x^53 + x^52 + x^47 + x^46 + x^45 +
 * x^40 + x^39 + x^38 + x^37 + x^35 + x^33 + x^32 + x^31 + x^29 + x^27 +
 * x^24 + x^23 + x^22 + x^21 + x^19 + x^17 + x^13 + x^12 + x^10 + x^9 +
 * x^7 + x^4 + x + 1
 */


// Initialize a CRC accumulator
void
Crc64Init(UINT64 *crc)
{
	*crc = UINT64CONST(0xffffffffffffffff);
}

// Finish a CRC calculation
void
Crc64Fin(UINT64 *crc)
{
	*crc ^= UINT64CONST(0xffffffffffffffff);
}

// Accumulate some (more) bytes into a CRC
void
Crc64Accum(UINT64 *crc, unsigned char *data, unsigned int len)
{
	// Constant table for CRC calculation

	while (len-- > 0)
	{
		unsigned int	idx = ((unsigned int) ((*crc) >> 56) ^ *data++) & 0xFF;
		*crc = Crc64Table[idx] ^ ((*crc) << 8);
	}
}


UINT64
Crc64Block(unsigned char *data, unsigned int len)
{
	UINT64	crc;

	Crc64Init(&crc);
	Crc64Accum(&crc, data, len);
	Crc64Fin(&crc);
	return crc;
}


UINT64
Crc64String(unsigned char *string)
{
	return Crc64Block(string, strlen((char*)string));
}


UINT64
Crc64File(FILE *pfile)
{
	UINT64			crc;
	unsigned char	pbBuf[BUFSIZ];
	size_t			sizeBytesRead;

	Crc64Init(&crc);

	do {
		sizeBytesRead = fread(pbBuf, 1, sizeof(pbBuf), pfile);
		if (0 == sizeBytesRead)
		{
			Crc64Fin(&crc);
		}
		else
		{
			Crc64Accum(&crc, pbBuf, sizeBytesRead);
		}
	} while(sizeBytesRead);

	return crc;
}


/*
 *  Crc 32 calculation stuff
 */

/*
 * Copyright (C) 1986 Gary S. Brown.  You may use this program, or
 * code or tables extracted from it, as desired without restriction.
 */

/* First, the polynomial itself and its table of feedback terms.  The  */
/* polynomial is                                                       */
/* X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0 */
/* Note that we take it "backwards" and put the highest-order term in  */
/* the lowest-order bit.  The X^32 term is "implied"; the LSB is the   */
/* X^31 term, etc.  The X^0 term (usually shown as "+1") results in    */
/* the MSB being 1.                                                    */

/* Note that the usual hardware shift register implementation, which   */
/* is what we're using (we're merely optimizing it by doing eight-bit  */
/* chunks at a time) shifts bits into the lowest-order term.  In our   */
/* implementation, that means shifting towards the right.  Why do we   */
/* do it this way?  Because the calculated CRC must be transmitted in  */
/* order from highest-order term to lowest-order term.  UARTs transmit */
/* characters in order from LSB to MSB.  By storing the CRC this way,  */
/* we hand it to the UART in the order low-byte to high-byte; the UART */
/* sends each low-bit to hight-bit; and the result is transmission bit */
/* by bit from highest- to lowest-order term without requiring any bit */
/* shuffling on our part.  Reception works similarly.                  */

/* The feedback terms table consists of 256, 32-bit entries.  Notes:   */
/*                                                                     */
/*     The table can be generated at runtime if desired; code to do so */
/*     is shown later.  It might not be obvious, but the feedback      */
/*     terms simply represent the results of eight shift/xor opera-    */
/*     tions for all combinations of data and CRC register values.     */
/*                                                                     */
/*     The values must be right-shifted by eight bits by the "updcrc"  */
/*     logic; the shift must be unsigned (bring in zeroes).  On some   */
/*     hardware you could probably optimize the shift in assembler by  */
/*     using byte-swap instructions.                                   */

static UINT32 cr3tab[] =
{ /* CRC polynomial 0xedb88320 */
0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

#define _UtilCrc32(b, c) (cr3tab[((int)(c) ^ (b)) & 0xff] ^ (((c) >> 8) & 0x00FFFFFF))


UINT32
Crc32Block (unsigned char *ptr, int len)
{
	UINT32	crc;

	for ( crc = -1; len > 0; --len )
	{
		crc = _UtilCrc32 (*ptr, crc);
		++ptr;		/* avoid macro side effects */
	}

	return crc;
}


UINT32
Crc32String (unsigned char *string)
{

	return Crc32Block (string, strlen((char*)string));
}


#if 0
int
main(int argc, char *argv[])
{

	printf("%lld\n", Crc64File(stdin));
	return 0;
}
#endif
