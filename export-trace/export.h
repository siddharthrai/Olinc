#ifndef INTERMOD_COMMON
#define INTERMOD_COMMON

#ifdef __cplusplus
# define TRUE  (true)
# define FALSE (false)
#else
# define TRUE   (1)
# define FALSE  (0)
#endif

typedef signed   long   sb8;
typedef unsigned long   ub8;
typedef signed   int    sb4;
typedef unsigned int    ub4;
typedef float           uf4;
typedef signed   short  sb2;
typedef unsigned short  ub2;
typedef char            sb1;
typedef unsigned char   ub1;
typedef double          uf8;

/* Constants used for access streams (streams identify GPU units and CPU independent of cache hierarchy) */

#define NN      (0)         /* Invalid stream */
#define IS      (1)         /* Input vertex stream */
#define CS      (2)         /* Color stream */
#define ZS      (3)         /* Depth stream */
#define TS      (4)         /* Texture stream */
#define NS      (5)         /* Instruction stream */
#define HS      (6)         /* HZ stream */
#define BS      (7)         /* Blitter stream */
#define DS      (8)         /* DAC stream */
#define XS      (9)         /* Index stream */
#define PS      (10)        /* CPU stream */
#define PS1     (11)        /* CPU core1 stream */
#define PS2     (12)        /* CPU core2 stream */
#define PS3     (13)        /* CPU core3 stream */
#define PS4     (14)        /* CPU core4 stream */
#define DCS     (15)        /* Dynamic color */
#define DZS     (16)        /* Dynamic depth */
#define DBS     (17)        /* Dynamic blitter  */
#define DPS     (18)        /* Dynamic processor*/
#define OS      (19)        /* Mixed stream (DS, DS, XS, NS, HS etc.) */
#define GP      (20)        /* GPGPU stream */
#define TST     (20)        /* Total streams */

sb1* stream_name[TST + 1] = {"U", "I", "C", "Z", "T", "N", "H", "B", "D", "X", "P0", "P1", "P2", "P3", "P4", "DC", "DZ", "DB", "DP", "OS", "GP"};


/* Streams specific to SAPPRIORITY. SAPPRIORITY controller remaps global stream id to SAPPRIORITY
 *  * specific ids */
typedef enum sap_stream
{
  sappriority_stream_u = 0,
  sappriority_stream_x,
  sappriority_stream_y,
  sappriority_stream_p,
  sappriority_stream_q,
  sappriority_stream_r,
  sappriority_stream_cnt
}sap_stream;

sb1* sap_stream_name[sappriority_stream_cnt] = {"U", "X", "Y", "P", "Q", "R"};

/* Memory access trace */ 
typedef struct memory_trace
{
  ub8     address;      /* Physical address */
  ub8     vtl_addr;     /* Virtual address */
  ub8     pc;           /* PC */
  ub1     stream;       /* Accessing stream, it takes any of the above mentioned values */
  ub4     core;         /* CPU / GPU core id */
  ub4     pid;          /* Processing unit id */
  ub4     mapped_rop;   /* ROP segment request belongs to. It identifies shader or ROP for the LLC request */
  ub1     dbuf;         /* True if address belong to default buffer */
  ub1     fill;         /* True if block is to be read */
  ub1     spill;        /* True if block is written */
  ub1     prefetch;     /* True for pre-fetch */
  ub1     dirty;        /* True if write-back is dirty */
  ub1     sap_stream;   /* stream identified by new policy (member of enum sap_stream)*/
  ub8     cycle;        /* Clock cycle of each request */
  void   *policy_data;  /* Policy specific data */
}memory_trace;

#endif
