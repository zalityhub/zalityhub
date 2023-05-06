#ifdef __TANDEM
#pragma columns 80
#if _TANDEM_ARCH_ != 0
#pragma ONCE
#endif
#endif
#ifndef _STDLIB
#define _STDLIB

/* T8645H04ABV - (01MAY2017) - stdlib.h   standard library definitions */

/*
 *  Copyright 2016, 2017 Hewlett Packard Enterprise Development LP.
 *
 *     ALL RIGHTS RESERVED
 */

#ifdef __cplusplus
   extern "C" {
#endif


#ifndef NULL
#if defined(_GUARDIAN_TARGET) && defined(__XMEM)
#define NULL   0L
#else
#define NULL   0
#endif
#endif /* NULL */

#ifndef __size_t_DEFINED
#define __size_t_DEFINED
#if _TANDEM_ARCH_ != 0 || !defined(__TANDEM)
#ifdef __LP64
typedef unsigned long  size_t;
#else  /* !__LP64 */
typedef unsigned int   size_t;
#endif /* !__LP64 */
#else /* _TANDEM_ARCH_ == 0 */
#if !defined(_GUARDIAN_TARGET) || defined(__XMEM)
typedef unsigned long  size_t;
#else
typedef unsigned short size_t;
#endif
#endif /* _TANDEM_ARCH_ == 0 */
#endif /* __size_t_DEFINED */

#ifndef __wchar_t_DEFINED
#define __wchar_t_DEFINED
typedef unsigned short wchar_t;
#endif

#ifdef _XOPEN_SOURCE
#ifndef _GUARDIAN_HOST
#include <sys/wait.h>
#else
#include <syswait.h> nolist
#endif /* _GUARDIAN_HOST */
#endif /* _XOPEN_SOURCE */

#define EXIT_FAILURE    -1
#define EXIT_SUCCESS     0
#if _TANDEM_ARCH_ != 0 || defined(__INT32)
#define MB_CUR_MAX       __getmbcurmax()
int    __getmbcurmax(void);
#else
#define MB_CUR_MAX       2
#endif

#if _TANDEM_ARCH_ != 0 || !defined(__TANDEM)
#define RAND_MAX     2147483647
#else
#define RAND_MAX     32767
#endif

#ifdef __TANDEM
#ifdef __LP64
#pragma fieldalign platform div_t ldiv_t lldiv_t
#else  /* !__LP64 */
#pragma fieldalign shared8 div_t ldiv_t lldiv_t
#endif /* !__LP64 */
#endif
typedef struct div_t   { int  quot, rem; } div_t;
typedef struct ldiv_t  { long quot, rem; } ldiv_t;
typedef struct lldiv_t { long long quot, rem; } lldiv_t;

void   abort (void);
int    abs (int);
int    atexit (void (*)(void));
void  *bsearch (const void *, const void *, size_t, size_t,
                int (*)(const void *, const void *));
void  *calloc (size_t, size_t);
div_t  div (int, int);
void   exit (int);
#if _TANDEM_ARCH_ == 2 && __H_Series_RVU >= 608
void  _Exit (int);
#pragma function  _Exit (alias("_exit"), unspecified)
#endif
char  *getenv (const char *);
long   labs (long);
ldiv_t ldiv (long, long);
#if _TANDEM_ARCH_ == 2 && __H_Series_RVU >= 608
long long llabs (long long);
#endif
#if _TANDEM_ARCH_ >= 1
lldiv_t lldiv (long long, long long);
#endif
void  *malloc (size_t);
#if _TANDEM_ARCH_ > 1
void  *malloc_pages (size_t);
#endif
int    mblen (const char *, size_t);
size_t mbstowcs (wchar_t *, const char *, size_t);
int    mbtowc (wchar_t *, const char *, size_t);
void   qsort (void *, size_t, size_t, int (*)(const void *, const void *));
int    rand (void);
void   srand (unsigned int);
double strtod (const char *, char **);
#if (_TANDEM_ARCH_ == 2 && __H_Series_RVU >= 621)
#pragma push warn
#pragma nowarn
long double strtold (const char *, char **);
#pragma function strtold  (alias("strtod"), unspecified)
#pragma pop warn
#endif
#if _TANDEM_ARCH_ == 2 && __H_Series_RVU >= 608
float  strtof (const char *, char **);
#endif
long   strtol (const char *, char **, int);
unsigned long strtoul (const char *, char **, int);
#if _TANDEM_ARCH_ != 0 || !defined(__TANDEM)
#if _TANDEM_ARCH_ > 1 || __G_Series_RVU >= 625
long long          strtoll (const char *, char **, int);
#endif /* G06.25 or later */
#if _TANDEM_ARCH_ > 1 || __G_Series_RVU >= 627 && __SQLCFE != 1
unsigned long long strtoull(const char *, char **, int);
#endif /* G06.27 or later and not SQLCFE */
#endif /* _TANDEM_ARCH_ != 0 */
int    system (const char *);
#if _TANDEM_ARCH_ != 0 || !defined(__TANDEM)
void  *valloc (size_t);
#endif
size_t wcstombs (char *, const wchar_t *, size_t);
int    wctomb (char *, wchar_t);

#if (_TANDEM_ARCH_ == 2 && __H_Series_RVU >= 621) \
    && !defined(_SPT_MODEL_)
int    rand_r  (unsigned int *);
#endif /* _TANDEM_ARCH_ == 2 && !_SPT_MODEL_ */

#if !defined(_GUARDIAN_TARGET) || defined(__XMEM)
double atof (const char *);
int    atoi (const char *);
long   atol (const char *);
#if _TANDEM_ARCH_ != 0 || !defined(__TANDEM)
long long atoll (const char *);
#endif
void   free (void *);
void  *realloc (void *, size_t);
#else /* small model */
double atof (const char _far *);
int    atoi (const char _far *);
long   atol (const char _far *);
void   free (void _far *);
void  *realloc (void _far *, size_t);
#endif /* !_GUARDIAN_TARGET || __XMEM */

#if defined(_XOPEN_SOURCE) || defined(_TANDEM_SOURCE)
#if (_TANDEM_ARCH_ == 2 && __H_Series_RVU >= 621) \
    && !defined(_SPT_MODEL_)
int    getenv_r(const char *, char *, size_t);
#endif /* _TANDEM_ARCH_ == 2 && !_SPT_MODEL_ */
#if _TANDEM_ARCH_ != 0 || defined(__INT32) || !defined(__TANDEM)
int    putenv  (const char *);   /* standard */
#elif defined(__XMEM)
long   putenv  (const char *);   /* large model */
#endif
#if defined(__TANDEM) && _TANDEM_ARCH_ == 0 && defined(__XMEM)
#pragma function putenv (alias("CRE_PUTENV_"), unspecified)
#endif
#endif /* _XOPEN_SOURCE || _TANDEM_SOURCE */

#ifdef _XOPEN_SOURCE
double   drand48 (void);
double   erand48 (unsigned short int *);    /* xsubi[3] */
long int jrand48 (unsigned short int *);    /* xsubi[3] */
long int lrand48 (void);
long int mrand48 (void);
long int nrand48 (unsigned short int *);    /* xsubi[3] */
void     srand48 (long int);

void     lcong48 (unsigned short int *);    /* param[7] */
unsigned short int *seed48 (unsigned short int *);    /* seed16v[3] */
void     setkey  (const char *);   /* not supported */
#endif /* _XOPEN_SOURCE */

#if _TANDEM_ARCH_ != 0 || !defined(__TANDEM)
#if _XOPEN_SOURCE_EXTENDED == 1
long   a64l(const char *);
char  *fcvt(double, int, int *, int *);
char  *gcvt(double, int, char *);
int    getsubopt(char **, char * const *, char **);
char  *initstate(unsigned int, char *, size_t);
char  *l64a(long);
char  *mktemp(char *);
int    mkstemp(char *);
long   random(void);
char  *realpath(const char *, char *);
char  *setstate(const char *);
void   srandom(unsigned int);
#endif /* _XOPEN_SOURCE_EXTENDED == 1 */
#if _XOPEN_SOURCE_EXTENDED == 1 || defined(_TANDEM_SOURCE)
char  *ecvt(double, int, int *, int *);
#endif /* _XOPEN_SOURCE_EXTENDED == 1 || _TANDEM_SOURCE */
#endif /* _TANDEM_ARCH_ != 0 || !__TANDEM */

#ifdef _TANDEM_SOURCE

#define _abs(x)   ((x) < 0 ? -(x) : (x))
#ifdef __NSK_BOOST_VERSION__
#define __max(x, y)  ((x) > (y) ? (x) : (y))
#define __min(x, y)  ((x) < (y) ? (x) : (y))
#else
#define _max(x, y)  ((x) > (y) ? (x) : (y))
#define _min(x, y)  ((x) < (y) ? (x) : (y))
#endif

void  heap_check (void);
void  heap_check_always (int);
void  heap_min_block_size (long);
#if _TANDEM_ARCH_ == 0
#if !defined(_GUARDIAN_TARGET) || defined(__XMEM)
char *ecvt (double, short, short *, short *);
#else /* small model */
char *ecvt (double, short, short _far *, short _far *);
#endif /* !_GUARDIAN_TARGET || __XMEM */
int   stcd_i (char *, int *);
int   stcd_l (char *, long *);
int   stch_i (char *, int *);
int   stci_d (char *, int, int);
int   stcu_d (char *, unsigned int, int);
#endif /* _TANDEM_ARCH_ == 0 */
#if !defined(_GUARDIAN_TARGET) || defined(__XMEM)
void terminate_program (short, short, short, short, short *, char *, short);
#else /* small model */
void terminate_program \
          (short, short, short, short, short _far *, char _far *, short);
#endif /* !_GUARDIAN_TARGET || __XMEM */

#ifdef __LP64
void _ptr32 *calloc32  ( unsigned int, unsigned int );
void         free32    ( void _ptr32 * );
void _ptr32 *malloc32  ( unsigned int );
void _ptr32 *realloc32 ( void _ptr32 *, unsigned int );
void         heap_check32 ( void );
void         heap_check_always32 ( int );
#endif /* __LP64 */

#ifdef __TANDEM
#pragma fieldalign shared8 CRE_RFOA
#endif
/*  Receive file open attributes template  */
typedef struct CRE_RFOA {
   short maximum_requesters;      /* Maximum requesters supported        */
   short maximum_syncdepth;       /* Maximum saved replies per requester */
   short maximum_reply;           /* Maximum bytes in saved reply        */
   short receive_depth;           /* Maximum server queue depth          */
   short report_flags[4];         /* Report system message flags         */
   short filler;                  /* Reserved (must be 0)                */
} CRE_RFOA;

/*  Masks for report_flags[0]  */
#define CRE_cpu_down_mask             0040000   /* System message   -2 */
#define CRE_cpu_up_mask               0020000   /* System message   -3 */
#define CRE_settime_mask              0010000   /* System message  -10 */
#define CRE_power_on_mask             0004000   /* System message  -11 */
#define CRE_NEWPROCESSNOWAIT_mask     0002000   /* System message  -12 */
#define CRE_message_missed_mask       0001000   /* System message  -13 */
#define CRE_3270_status_mask          0000100   /* System message  -21 */
#define CRE_SIGNALTIMEOUT_mask        0000040   /* System message  -22 */
#define CRE_LOCKMEMORY_done_mask      0000020   /* System message  -23 */
#define CRE_LOCKMEMORY_failed_mask    0000010   /* System message  -24 */
#define CRE_PROCSIGNALTIMEOUT_mask    0000002   /* System message  -26 */
/*  Masks for report_flags[1]  */
#define CRE_CONTROL_mask              0100000   /* System message  -32 */
#define CRE_SETMODE_mask              0040000   /* System message  -33 */
#define CRE_RESETSYNC_mask            0020000   /* System message  -34 */
#define CRE_CONTROLBUF_mask           0010000   /* System message  -35 */
#define CRE_SETPARAM_mask             0002000   /* System message  -37 */
#define CRE_message_cancelled_mask    0001000   /* System message  -38 */
#define CRE_DEVICEINFO2_mask          0000100   /* System message  -41 */
#define CRE_logical_close_mask        0000002
#define CRE_logical_open_mask         0000001
/*  Masks for report_flags[2]  */
#define CRE_remote_cpu_down_mask      0100000   /* System message -100 */
#define CRE_process_deleted_mask      0040000   /* System message -101 */
#define CRE_PROCESS_CREATE_mask       0020000   /* System message -102 */
#define CRE_OPEN_mask                 0010000   /* System message -103 */
#define CRE_CLOSE_mask                0004000   /* System message -104 */
#define CRE_break_mask                0002000   /* System message -105 */
#define CRE_devinfo_query_mask        0001000   /* System message -106 */
#define CRE_subname_mask              0000400   /* System message -107 */
#define CRE_FILE_GETINFO__mask        0000200   /* System message -108 */
#define CRE_FILENAME_FINDNEXT_mask    0000100   /* System message -109 */
#define CRE_node_down_mask            0000040   /* System message -110 */
#define CRE_node_up_mask              0000020   /* System message -111 */
#define CRE_GMOM_notify_mask          0000010   /* System message -112 */
#define CRE_remote_cpu_up_mask        0000004   /* System message -113 */
/*  Masks for report_flags[3]  */
#define CRE_pathsend_dialog_abort_mask 0002000  /* System message -121 */

#ifdef __TANDEM
#pragma fieldalign shared8 CRE_sender
#endif
/*  Receive message sender info template  */
typedef struct CRE_sender {
   short system_flag;             /* -1 if system message, 0 otherwise */
   short entry_number;            /* Requester number for sender       */
   short message_number;          /* CRE message number                */
   short file_number;             /* Guardian open number for sender   */
   short phandle[10];             /* Process handle of sender          */
   short read_count;              /* Maximum size for reply text       */
   short pathsend_dialog_info;    /* PATHSEND dialog info              */
} CRE_sender;

#if defined(__TANDEM) && _TANDEM_ARCH_ == 0
short CRE_RECEIVE_OPEN_CLOSE_(short, CRE_RFOA *, long _near *);
#else
short CRE_RECEIVE_OPEN_CLOSE_(short, CRE_RFOA *, long *);
#endif
short CRE_RECEIVE_READ_ (char *, short, short *, long, CRE_sender *, short);
short CRE_RECEIVE_WRITE_(char *, short, short, short);

#ifdef __TANDEM
#pragma fieldalign shared2 startup_msg_type
#pragma fieldalign shared2 assign_msg_type
#pragma fieldalign shared2 param_msg_type
#endif

/* Guardian STARTUP message format */
typedef struct startup_msg_type
   {
     short msg_code;
     union
         { char   whole[16];
           struct
             { char volume[8];
               char subvolume[8];
             }    parts;
         } defaults;
     union
         { char   whole[24];
           struct
             { char volume[8];
               char subvolume[8];
               char file[8];
             }    parts;
         } infile;
     union
         { char   whole[24];
           struct
             { char volume[8];
               char subvolume[8];
               char file[8];
             }    parts;
         } outfile;
     char  param[530];
   } startup_msg_type;

/* Guardian ASSIGN message format */
typedef struct assign_msg_type
   {
     short  msg_code;
     struct
         {
            char prognamelen;
            char progname[31];
            char filenamelen;
            char filename[31];
         }  logical_unit_name;
#ifdef __LP64
     unsigned int field_mask;
#else  /* !__LP64 */
     unsigned long field_mask;
#endif /* !__LP64 */
     union
         { char   whole[24];
           struct
             { char volume[8];
               char subvolume[8];
               char file[8];
             }    parts;
         }  filename;
     short  primary_extent;
     short  secondary_extent;
     short  file_code;
     short  exclusion_spec;
     short  access_spec;
     short  record_size;
     short  block_size;
   } assign_msg_type;

/* Guardian PARAM message format */
typedef struct param_msg_type
   {
     short  msg_code;
     short  num_params;
     char   parameters[1024];
   } param_msg_type;

#ifdef _GUARDIAN_TARGET
short get_max_assign_msg_ordinal (void);
#ifdef __XMEM
short get_assign_msg (short, assign_msg_type *);
short get_assign_msg_by_name (char *, assign_msg_type *);
short get_param_msg(param_msg_type *, short *);
short get_param_by_name(char *, char *, short);
short get_startup_msg (startup_msg_type *, short *);
#else /* small model */
short get_assign_msg (short, assign_msg_type _far *);
short get_assign_msg_by_name (char _far *, assign_msg_type _far *);
short get_param_msg (param_msg_type _far *, short _far *);
short get_param_by_name (char _far *, char _far *, short);
short get_startup_msg (startup_msg_type _far *, short _far *);
#endif /* __XMEM */
#if _TANDEM_ARCH_ == 0
void trap_overflows (short);
#endif
#endif /* _GUARDIAN_TARGET */

#endif /* _TANDEM_SOURCE */

/*
 * TNS C aliases and attributes
 */
#if defined(__TANDEM) && _TANDEM_ARCH_ == 0

/*  This header file has a different structure than others because many
 *  functions have both "check" and "nocheck" variants and many others
 *  don't.  Consequently, the 'aliases and attributes' repository here
 *  has one section for functions with "check" and "nocheck" variants
 *  and a different section for the functions without these variants.
 */

#ifdef _GUARDIAN_TARGET
/*
 * This section is for functions WITHOUT the "check" and "nocheck" variants
 */
#ifdef __XMEM
#ifdef __INT32
/*
 * Wide Model
 */
#pragma function abs                   (alias("RTL_ABSVAL_INT32_"        ),tal)
#pragma function atexit                (alias("C_LG_ATEXIT_32_"          ),tal)
#pragma function atof                  (alias("RTL_ATOF_"                ),tal)
#pragma function atoi                  (alias("RTL_ATOL_"                ),tal)
#pragma function atol                  (alias("RTL_ATOL_"                ),tal)
#pragma function exit                  (alias("C_EXIT_32_"               ),tal)
#pragma function getenv                (alias("CRE_GETENV_"),      unspecified)
#pragma function labs                  (alias("RTL_ABSVAL_INT32_"        ),tal)
#pragma function rand                  (alias("CRE_RANDOM_NEXT_32_"      ),tal)
#pragma function srand                 (alias("CRE_RANDOM_SET_32_"       ),tal)
#pragma function system                (alias("C_LG_SYSTEM_32_"          ),tal)
#ifdef _TANDEM_SOURCE
#pragma function ecvt                  (alias("RTL_ECVTX_"               ),tal)
#pragma function heap_check            (alias("C_HEAP_CHECK_"            ),tal)
#pragma function heap_check_always     (alias("C_HEAP_CHECK_ALWAYS_32_"  ),tal)
#pragma function heap_min_block_size   (alias("C_HEAP_MIN_BLOCKSIZE_"    ),tal)
#endif /* _TANDEM_SOURCE */

#else
/*
 * Large Model
 */
#pragma function abs                   (alias("RTL_ABSVAL_INT16_"        ),tal)
#pragma function atexit                (alias("C_LG_ATEXIT_"             ),tal)
#pragma function atof                  (alias("RTL_ATOF_"                ),tal)
#pragma function atoi                  (alias("RTL_ATOI_"                ),tal)
#pragma function atol                  (alias("RTL_ATOL_"                ),tal)
#pragma function exit                  (alias("C_EXIT_"                  ),tal)
#pragma function getenv                (alias("CRE_GETENV_"),      unspecified)
#pragma function labs                  (alias("RTL_ABSVAL_INT32_"        ),tal)
#pragma function rand                  (alias("CRE_RANDOM_NEXT_"         ),tal)
#pragma function srand                 (alias("CRE_RANDOM_SET_"          ),tal)
#pragma function system                (alias("C_LG_SYSTEM_"             ),tal)
#ifdef _TANDEM_SOURCE
#pragma function ecvt                  (alias("RTL_ECVTX_"               ),tal)
#pragma function heap_check            (alias("C_HEAP_CHECK_"            ),tal)
#pragma function heap_check_always     (alias("C_HEAP_CHECK_ALWAYS_"     ),tal)
#pragma function heap_min_block_size   (alias("C_HEAP_MIN_BLOCKSIZE_"    ),tal)
#endif /* _TANDEM_SOURCE */
#endif
#else
/*
 * Small Model
 */
#pragma function abs                   (alias("RTL_ABSVAL_INT16_"    ),tal)
#pragma function atexit                (alias("C_SM_ATEXIT_"         ),tal)
#pragma function atof                  (alias("RTL_ATOF_"),            tal)
#pragma function atoi                  (alias("RTL_ATOI_"),            tal)
#pragma function atol                  (alias("RTL_ATOL_"),            tal)
#pragma function ecvt                  (alias("RTL_ECVT_"),            tal)
#pragma function exit                  (alias("C_EXIT_"              ),tal)
#pragma function getenv                (alias("C_SM_GETENV_"         ),tal)
#pragma function heap_check            (alias("C_HEAP_CHECK_"        ),tal)
#pragma function heap_check_always     (alias("C_HEAP_CHECK_ALWAYS_" ),tal)
#pragma function heap_min_block_size   (alias("C_HEAP_MIN_BLOCKSIZE_"),tal)
#pragma function labs                  (alias("RTL_ABSVAL_INT32_"    ),tal)
#pragma function rand                  (alias("CRE_RANDOM_NEXT_"     ),tal)
#pragma function srand                 (alias("CRE_RANDOM_SET_"      ),tal)
#pragma function system                (alias("C_SM_SYSTEM_"         ),tal)
#endif /* __XMEM */

/*
 * This section is for functions WITH the "check" and "nocheck" variants
 */
#ifdef __CHECK

#ifdef __XMEM
#ifdef __INT32
/*
 * Wide Checked Model
 */
#pragma function calloc                (alias("C_LG_CK_CALLOC_"      ),tal)
#pragma function free                  (alias("C_CK_FREE_"           ),tal)
#pragma function malloc                (alias("C_LG_CK_MALLOC_"      ),tal)
#pragma function realloc               (alias("C_LG_CK_REALLOC_"     ),tal)
#pragma function strtod                (alias("CRE_CK_STRTODX_"      ),tal)
#pragma function strtol                (alias("CRE_CK_STRTOLX_32_"   ),tal)
#pragma function strtoul               (alias("CRE_CK_STRTOULX_32_"  ),tal)
#ifdef _TANDEM_SOURCE
#pragma function stcd_i                (alias("CRE_CK_STCD_LX_32_"   ),tal)
#pragma function stcd_l                (alias("CRE_CK_STCD_LX_32_"   ),tal)
#pragma function stch_i                (alias("CRE_CK_STCH_IX_32_"   ),tal)
#pragma function stci_d                (alias("CRE_CK_STCI_DX_32_"   ),tal)
#pragma function stcu_d                (alias("CRE_CK_STCU_DX_32_"   ),tal)
#endif /* _TANDEM_SOURCE */
#else
/*
 * Large Checked Model
 */
#pragma function calloc                (alias("C_LG_CK_CALLOC_"      ),tal)
#pragma function free                  (alias("C_CK_FREE_"           ),tal)
#pragma function malloc                (alias("C_LG_CK_MALLOC_"      ),tal)
#pragma function realloc               (alias("C_LG_CK_REALLOC_"     ),tal)
#pragma function strtod                (alias("CRE_CK_STRTODX_"      ),tal)
#pragma function strtol                (alias("CRE_CK_STRTOLX_"      ),tal)
#pragma function strtoul               (alias("CRE_CK_STRTOULX_"     ),tal)
#ifdef _TANDEM_SOURCE
#pragma function stcd_i                (alias("CRE_CK_STCD_IX_"      ),tal)
#pragma function stcd_l                (alias("CRE_CK_STCD_LX_"      ),tal)
#pragma function stch_i                (alias("CRE_CK_STCH_IX_"      ),tal)
#pragma function stci_d                (alias("CRE_CK_STCI_DX_"      ),tal)
#pragma function stcu_d                (alias("CRE_CK_STCU_DX_"      ),tal)
#endif /* _TANDEM_SOURCE */
#endif
#else
/*
 * Small Checked Model
 */
#pragma function calloc                (alias("C_SM_CK_CALLOC_"      ),tal)
#pragma function free                  (alias("C_CK_FREE_"           ),tal)
#pragma function malloc                (alias("C_SM_CK_MALLOC_"      ),tal)
#pragma function realloc               (alias("C_SM_CK_REALLOC_"     ),tal)
#pragma function strtod                (alias("CRE_CK_STRTOD_"       ),tal)
#pragma function strtol                (alias("CRE_CK_STRTOL_"       ),tal)
#pragma function strtoul               (alias("CRE_CK_STRTOUL_"      ),tal)
#pragma function stcd_i                (alias("CRE_CK_STCD_I_"       ),tal)
#pragma function stcd_l                (alias("CRE_CK_STCD_L_"       ),tal)
#pragma function stch_i                (alias("CRE_CK_STCH_I_"       ),tal)
#pragma function stci_d                (alias("CRE_CK_STCI_D_"       ),tal)
#pragma function stcu_d                (alias("CRE_CK_STCU_D_"       ),tal)
#endif /* __XMEM */

#else

#ifdef __XMEM
#ifdef __INT32
/*
 * Wide Unchecked Model
 */
#pragma function calloc                (alias("C_LG_CALLOC_"         ),tal)
#pragma function free                  (alias("C_FREE_"              ),tal)
#pragma function malloc                (alias("C_LG_MALLOC_"         ),tal)
#pragma function realloc               (alias("C_LG_REALLOC_"        ),tal)
#pragma function strtod                (alias("CRE_STRTODX_"         ),tal)
#pragma function strtol                (alias("CRE_STRTOLX_32_"      ),tal)
#pragma function strtoul               (alias("CRE_STRTOULX_32_"     ),tal)
#ifdef _TANDEM_SOURCE
#pragma function stcd_i                (alias("RTL_STCD_LX_32_"      ),tal)
#pragma function stcd_l                (alias("RTL_STCD_LX_32_"      ),tal)
#pragma function stch_i                (alias("RTL_STCH_IX_32_"      ),tal)
#pragma function stci_d                (alias("RTL_STCI_DX_32_"      ),tal)
#pragma function stcu_d                (alias("RTL_STCU_DX_32_"      ),tal)
#endif /* _TANDEM_SOURCE */
#else
/*
 * Large Unchecked Model
 */
#pragma function calloc                (alias("C_LG_CALLOC_"         ),tal)
#pragma function free                  (alias("C_FREE_"              ),tal)
#pragma function malloc                (alias("C_LG_MALLOC_"         ),tal)
#pragma function realloc               (alias("C_LG_REALLOC_"        ),tal)
#pragma function strtod                (alias("CRE_STRTODX_"         ),tal)
#pragma function strtol                (alias("CRE_STRTOLX_"         ),tal)
#pragma function strtoul               (alias("CRE_STRTOULX_"        ),tal)
#ifdef _TANDEM_SOURCE
#pragma function stcd_i                (alias("RTL_STCD_IX_"         ),tal)
#pragma function stcd_l                (alias("RTL_STCD_LX_"         ),tal)
#pragma function stch_i                (alias("RTL_STCH_IX_"         ),tal)
#pragma function stci_d                (alias("RTL_STCI_DX_"         ),tal)
#pragma function stcu_d                (alias("RTL_STCU_DX_"         ),tal)
#endif /* _TANDEM_SOURCE */
#endif
#else
/*
 * Small Unchecked Model
 */
#pragma function calloc                (alias("C_SM_CALLOC_"         ),tal)
#pragma function free                  (alias("C_FREE_"              ),tal)
#pragma function malloc                (alias("C_SM_MALLOC_"         ),tal)
#pragma function realloc               (alias("C_SM_REALLOC_"        ),tal)
#pragma function strtod                (alias("CRE_STRTOD_"          ),tal)
#pragma function strtol                (alias("CRE_STRTOL_"          ),tal)
#pragma function strtoul               (alias("CRE_STRTOUL_"         ),tal)
#pragma function stcd_i                (alias("RTL_STCD_I_"          ),tal)
#pragma function stcd_l                (alias("RTL_STCD_L_"          ),tal)
#pragma function stch_i                (alias("RTL_STCH_I_"          ),tal)
#pragma function stci_d                (alias("RTL_STCI_D_"          ),tal)
#pragma function stcu_d                (alias("RTL_STCU_D_"          ),tal)
#endif /* __XMEM */
#endif /* __CHECK */
#else
/*
 * OSS
 */
#pragma function abs                   (alias("RTL_ABSVAL_INT32_"    ),tal)
#pragma function atexit                (alias("C_LG_ATEXIT_32_"      ),tal)
#pragma function atof                  (alias("RTL_ATOF_"            ),tal)
#pragma function atoi                  (alias("RTL_ATOL_"            ),tal)
#pragma function atol                  (alias("RTL_ATOL_"            ),tal)
#pragma function exit                  (alias("C_EXIT_32_"           ),tal)
#pragma function getenv                (alias("CRE_GETENV_"),  unspecified)
#pragma function labs                  (alias("RTL_ABSVAL_INT32_"    ),tal)
#pragma function rand                  (alias("CRE_RANDOM_NEXT_32_"  ),tal)
#pragma function srand                 (alias("CRE_RANDOM_SET_32_"   ),tal)
#pragma function system                (alias("_C_PX_system_"), unspecified)
#ifdef _TANDEM_SOURCE
#pragma function ecvt                  (alias("RTL_ECVTX_"               ),tal)
#pragma function heap_check            (alias("C_HEAP_CHECK_"            ),tal)
#pragma function heap_check_always     (alias("C_HEAP_CHECK_ALWAYS_32_"  ),tal)
#pragma function heap_min_block_size   (alias("C_HEAP_MIN_BLOCKSIZE_"    ),tal)
#endif /* _TANDEM_SOURCE */
#ifndef __CHECK
#pragma function calloc                (alias("C_LG_CALLOC_"         ),tal)
#pragma function free                  (alias("C_FREE_"              ),tal)
#pragma function malloc                (alias("C_LG_MALLOC_"         ),tal)
#pragma function realloc               (alias("C_LG_REALLOC_"        ),tal)
#pragma function strtod                (alias("CRE_STRTODX_"         ),tal)
#pragma function strtol                (alias("CRE_STRTOLX_32_"      ),tal)
#pragma function strtoul               (alias("CRE_STRTOULX_32_"     ),tal)
#ifdef _TANDEM_SOURCE
#pragma function stcd_i                (alias("RTL_STCD_LX_32_"      ),tal)
#pragma function stcd_l                (alias("RTL_STCD_LX_32_"      ),tal)
#pragma function stch_i                (alias("RTL_STCH_IX_32_"      ),tal)
#pragma function stci_d                (alias("RTL_STCI_DX_32_"      ),tal)
#pragma function stcu_d                (alias("RTL_STCU_DX_32_"      ),tal)
#endif /* _TANDEM_SOURCE */
#else   /* __CHECK */
#pragma function calloc                (alias("C_LG_CK_CALLOC_"      ),tal)
#pragma function free                  (alias("C_CK_FREE_"           ),tal)
#pragma function malloc                (alias("C_LG_CK_MALLOC_"      ),tal)
#pragma function realloc               (alias("C_LG_CK_REALLOC_"     ),tal)
#pragma function strtod                (alias("CRE_CK_STRTODX_"      ),tal)
#pragma function strtol                (alias("CRE_CK_STRTOLX_32_"   ),tal)
#pragma function strtoul               (alias("CRE_CK_STRTOULX_32_"  ),tal)
#ifdef _TANDEM_SOURCE
#pragma function stcd_i                (alias("CRE_CK_STCD_LX_32_"   ),tal)
#pragma function stcd_l                (alias("CRE_CK_STCD_LX_32_"   ),tal)
#pragma function stch_i                (alias("CRE_CK_STCH_IX_32_"   ),tal)
#pragma function stci_d                (alias("CRE_CK_STCI_DX_32_"   ),tal)
#pragma function stcu_d                (alias("CRE_CK_STCU_DX_32_"   ),tal)
#endif /* _TANDEM_SOURCE */
#endif  /* __CHECK */
#endif

#endif /* __TANDEM && _TANDEM_ARCH_ == 0 */

#ifdef __TANDEM
#if _TANDEM_ARCH_ != 0 && defined(_GUARDIAN_TARGET)
#pragma function system                (alias("system_guardian"), tal)
#endif

#ifdef _TANDEM_SOURCE
#pragma function terminate_program (alias("CRE_TERMINATOR_"), extensible, tal)
#ifdef _GUARDIAN_TARGET
#pragma function get_max_assign_msg_ordinal \
                         (alias("CRE_ASSIGN_MAXORDINAL_"), extensible, tal)
#if _TANDEM_ARCH_ == 0
#pragma function get_assign_msg        (alias("C_GET_ASSIGN_MSG_"),    tal)
#pragma function get_assign_msg_by_name(alias("C_GET_ASSIGN_MSG_BY_NAME_"),tal)
#pragma function get_param_msg         (alias("C_GET_PARAM_MSG_"),     tal)
#pragma function get_param_by_name     (alias("C_GET_PARAM_BY_NAME_"), tal)
#pragma function get_startup_msg       (alias("C_GET_STARTUP_MSG_"),   tal)
#pragma function trap_overflows        (alias("RTL_ARITHMETIC_TRAPS_"),tal)
#endif /* _TANDEM_ARCH_ == 0 */
#endif /* _GUARDIAN_TARGET */

#pragma function CRE_RECEIVE_READ_       (extensible, unspecified)
#pragma function CRE_RECEIVE_WRITE_      (extensible, unspecified)
#pragma function CRE_RECEIVE_OPEN_CLOSE_ (extensible, unspecified)
#endif /* _TANDEM_SOURCE */

#endif /* __TANDEM */


#ifdef __cplusplus
   }
#endif

#endif  /* _STDLIB defined */
