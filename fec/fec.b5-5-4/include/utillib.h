/*****************************************************************************

Filename:   include/utillib.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/08/23 19:53:58 $
 * $Header: /home/hbray/cvsroot/fec/include/utillib.h,v 1.3.4.1 2011/08/23 19:53:58 hbray Exp $
 *
 $Log: utillib.h,v $
 Revision 1.3.4.1  2011/08/23 19:53:58  hbray
 eliminate fecplugin.h

 Revision 1.3  2011/07/27 20:22:13  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:38  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: utillib.h,v 1.3.4.1 2011/08/23 19:53:58 hbray Exp $ "


#ifndef _UTILLIB_H
#define _UTILLIB_H

// External Functions


typedef struct EnumToStringMap
{
	int		e;
	char	*t;
} EnumToStringMap;


extern void* GetSymbolAddr(char *libname, char *symbol);
extern char *ErrnoToString(int error);
extern int EnumMapStringToVal(EnumToStringMap * map, char *text, char *prefix);
extern char *EnumMapValToString(EnumToStringMap * map, int val);

extern char *FTrim(char *text, char *trimchr);
extern char *LTrim(char *text, char *trimchr);
extern char *RTrim(char *text, char *trimchr);

extern char *GetMnemonicCh(unsigned char ch, char *mnem);
extern char *GetMnemonicString(unsigned char *text, int slen);
extern char *Downshift(char *string);
extern char *GetTimeZone(void);
extern char *TimeToStringHHMM(time_t tod, char *output);
extern char *TimeToStringHHMMSS(time_t tod, char *output);
extern char* MsTimeToStringHHMMSS(NxTime_t ms, char *output);
extern int HexDump(int (*oFnc) (char *, void *), void *oFncArg, void *mem, int len, long offset);
extern boolean DebugConnected();
extern void RequestDebug(int ms);
extern StringArray_t* GetStackTrace();
extern int MakeCoreFile();
extern char *TimeToStringLong(time_t tod, char *output);
extern char *TimeToStringShort(time_t tod, char *output);
extern char *MsTimeToStringShort(NxTime_t msTime, char *output);
extern char *UsTimeToStringShort(NxTime_t usTime, char *output);
extern char *Upshift(char *string);
extern char *TimeToStringYYMMDD(time_t tod, char *output);
extern long GetDeltaTime(time_t startTime);
extern NxTime_t GetUsTime(void);
extern int strprefix(const char *stg, const char *pattern);
extern int striprefix(const char *stg, const char *pattern);
extern char *stristr(const char *s0, const char *s1);
extern char* strreplace(char *inp, char *from, char *to, int caseless, int once);
extern NxTime_t GetMsTime(void);
extern long unsigned int GetSecTime(void);

extern int IntFromString(const char *num, boolean * err);
extern UINT64 Int64FromString(const char *num, boolean * err);

extern char *DecodeUrlCharacter(char *val, char *out);
extern char *DecodeUrlCharacters(char *val, int *decodedLen);
extern char *EncodeUrlCharacters(char *val, int len);

extern char *DecodeEntityCharacter(char *val, char *out);
extern char *DecodeEntityCharacters(char *val, int *decodedLen);
extern char* EncodeEntityCharacter(char val, char *out);
extern char* EncodeEntityCharacters(char *val, int len);
extern char *EncodeEntityString(char *val);
extern char* EncodeCdataCharacters(char *val, int len);

typedef int (*WalkDirCallBack_t)(char *, struct stat *);
extern int MkDirPath(char *path);
extern int RmDirPath(char *path);
extern int WalkDirPath(char *dirPath, WalkDirCallBack_t callback);
extern int RmDirNode(char *path, struct stat *st);
extern int OpenFilePath(const char *pathname, int flags, mode_t mode);
#endif
