#ifndef INTERMOD_COMMON
#define INTERMOD_COMMON

#ifdef __cplusplus
# define TRUE  (true)
# define FALSE (false)
#else
# define TRUE   (1)
# define FALSE  (0)
#endif

#define SIM_END     (-1)
#define SIM_RUNNING (0)

#undef GPU_DEBUG 

#ifdef DEBUG_ON
# define GPU_DEBUG(expr) expr
#else
# define GPU_DEBUG(expr) 
#endif

#define panic printf

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
  
#include "./sat-counter.h"

/* Macro to clear entire structure */
#define CLRSTRUCT(s) (memset((&s), 0, sizeof(s)))

/* Macro to return minimum of two numbers */
#define MINIMUM(a, b) ((a) < (b) ? (a) : (b))

#define MAX_SHADER_STAGE (4)
#define FETCH_STAGE      (0)
#define DECODE_STAGE     (1)
#define EXEC_STAGE       (2)
#define WB_STAGE         (3)

#define MAX_CORES        (128)

/* GPU units for which llc will see requests */
#define TU      (9)

/* Constants for access streams (streams identify GPU units and CPU 
 * independent of cache hierarchy) */
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
#define TST     (19)        /* Total streams */

#define UNKNOWN (0)
#define IC      (1)
#define CC      (2)
#define ZC      (3)
#define TC      (4)
#define NC      (5)
#define HZ      (6)
#define BT      (7)
#define DU      (8)
#define IX      (9)
#define SIC     (10)
#define SCC     (11)
#define SZC     (12)
#define STC     (13)
#define SHZ     (14)
#define SNC     (15)
#define TUC     (16)
#define TCL0    (TUC + 1) 

/* Temporary: Set type recognized by SDP (Stall Directed Policy) */
#define SDP_SAMPLED_NN      (0)
#define SDP_BS_SAMPLED_SET  (1)
#define SDP_PS_SAMPLED_SET  (2)
#define SDP_CS1_SAMPLED_SET (3)
#define SDP_CS2_SAMPLED_SET (4)
#define SDP_CS3_SAMPLED_SET (5)
#define SDP_CS4_SAMPLED_SET (6)
#define SDP_CS5_SAMPLED_SET (7)
#define SDP_CS6_SAMPLED_SET (8)
#define SDP_CS7_SAMPLED_SET (9)
#define SDP_FOLLOWER_SET    (10)

/* Constants for request types */
#define READ_REQ       (1)
#define WRITE_REQ      (2)
#define ALLOCATE_REQ   (3)
#define INVALIDATE_REQ (4)

/* Constants for cache state. These states are used to ensure that cache 
 * receives a request in correct state and to maintain proper sequence of 
 * operation. For example cache are flushed before next read can be issued. */
#define CACHE_INVALID  (0)
#define CACHE_READY    (1)
#define CACHE_FLUSHING (2)

/* Constants for access responses */

/* Constants for Fragment FIFO stamp state */
#define UNKNOWN_QUEUE                 (0)
#define EZE_POST_Z_TEST               (1)
#define EZE_Z_TEST_IN_PROGRESS        (2)
#define EZE_POST_SHADING              (3)
#define EZE_SHADING_IN_PROGRESS       (4)
#define EZE_POST_INTERPOLATION        (5)
#define EZE_INTERPOLATION_IN_PROGRESS (6)
#define EZE_RAST_START                (7)
#define EZD_POST_Z_TEST               (8)
#define EZD_Z_TEST_IN_PROGRESS        (9)
#define EZD_POST_SHADING              (10)
#define EZD_SHADING_IN_PROGRESS       (11)
#define EZD_POST_INTERPOLATION        (12)
#define EZD_INTERPOLATION_IN_PROGRESS (13)
#define EZD_RAST_START                (14)

#define JF_INVALID          (0)
#define JF_MISS_CE          (1)
#define JF_MISS_DE          (2)
#define JF_HIT              (3)
#define JF_MISS             (4)
#define JF_MISS_AND_BYPASS  (5)

#define HASHTINDXMASK (0xfffffff)  /* Includes 6 bits of block offset */
#define HTBLSIZE      (1 << 22)   /* Size of bucket list is 64 */

#define ATTILA_MASTER_KEY (0xffffffff) /* Universal secondary Key */

#define ATTILA_PORT_INVALID     (0)
#define ATTILA_PORT_AVAILABLE   (1)
#define ATTILA_PORT_UNAVAILABLE (2)

#define ATTILA_MSHR_INVALID     (0)
#define ATTILA_MSHR_AVAILABLE   (1)
#define ATTILA_MSHR_UNAVAILABLE (2)

#define ATTILA_ACCESS_INVALID   (0)
#define ATTILA_ACCESS_START     (1)
#define ATTILA_ACCESS_DONE      (2)

/* GPU unit name */
typedef enum gpu_unit_name
{
  gpu_unit_unknown,               /* Unknown */
  gpu_unit_shader,                /* Any of the shader core */
  gpu_unit_depth,                 /* Any of the depth ROP */
  gpu_unit_color                  /* Any of the color ROP */
}gpu_unit_name;

/* Following structures are used to track stall cycles in shader pipeline */

/* Shader stall states */
typedef enum shader_thread_state
{
  fetch_busy,                       /* Fetcher busy */
  fetch_empty,                      /* Fetch idle */
  decode_empty,                     /* Decoder idle */
  exec_busy,                        /* Execution unit busy */
  exec_empty,                       /* Execution idle */
  control_dependence,               /* Branch taken */
  data_dependence,                  /* Data dependence */  
  out_of_resource,                  /* Shader out of resources */
  texture_fetch,                    /* Texture fetched */
  thread_busy,                      /* All shader threads are busy */
}shader_thread_state;

/* Fragment memory access states */
typedef enum frag_memory_access_state
{
  memory_access_unknown,            /* Fragment is culled */
  memory_access_arrived,            /* Fragment is newly arrived */
  memory_access_pending,            /* Fragment has not started memory access */
  private_cache_access,             /* Fragment in private cache hierarchy */
  shared_cache_access,              /* Fragment in shared cache hierarchy */
  llc_access,                       /* Fragment accessing memory */
  memory_access_finish              /* Fragment has finished memory accesses */
}frag_memory_access_state;

/* Memory access status */
typedef enum memory_access_status
{
  memory_access_status_unknown,     /* Unknown */
  memory_access_hit,                /* Hit */
  memory_access_miss                /* Miss */
}memory_access_status;

/* Memory access info
 *
 * Memory access info is used to track state of memory requests 
 * in the memory system. There are two state bits which identify 
 * current state of the carrier object (whether access has started 
 * / finished etc.) and status of the request if it has complete 
 * memory access (hit / miss etc.).
 * */
typedef struct memory_access_info
{
  gpu_unit_name             unit;           /* Unit of access */
  frag_memory_access_state  state;          /* Access has started / done etc */
  memory_access_status      status;         /* Hit / miss in cache */
  ub1                       assigned;       /* Hit / miss in cache */
  ub4                       mapped_rop;     /* Mapping ROP for this memory request */
  ub1                       sdp_bs_sample;  /* Data returned form */
  ub1                       sdp_ps_sample;  /* Data returned form */
  ub1                       sdp_cs_sample;  /* Data returned form */
}memory_access_info;

typedef memory_access_info mai;

/* Macros to access attila_data structure */
#define ATTILA_DATA_SEQNO(data) ((data)->seqno)
#define ATTILA_DATA_PREQS(data) ((data)->preqs)
#define ATTILA_DATA_VADDR(data) ((data)->vaddr)
#define ATTILA_DATA_SIZE(data)  ((data)->size)
#define ATTILA_DATA_UNIT(data)  ((data)->unit)
#define ATTILA_DATA_KIND(data)  ((data)->access_kind)
#define ATTILA_DATA_REQS(data)  ((data)->reqs)
#define ATTILA_DATA_SCYCL(data) ((data)->scycle)
#define ATTILA_DATA_ECYCL(data) ((data)->ecycle)

/* Data kept with attila for each access */
struct attila_data
{
  ub1 seqno;      /* Sequence number of this request */
  ub8 preqs;      /* Total pending request (including subblock reqs) */
  ub8 vaddr;      /* virtual address */
  ub4 size;       /* Size of request */
  ub1 unit;       /* Requesting unit (ICZT) */
  ub1 access_kind;/* Access kind (R / W)*/
  ub8 reqs;       /* Pointer to requestQueue entry */
  ub8 scycle;     /* Request start cycle */
  ub8 ecycle;     /* Request end cycle */
};

/* Macros to access m2s_data data members */
#define M2S_DATA_SEQNO(data)    ((data)->seqno)
#define M2S_DATA_VADDR(data)    ((data)->vaddr)
#define M2S_DATA_SIZE(data)     ((data)->size)
#define M2S_DATA_A_KIND(data)   ((data)->access_kind)
#define M2S_DATA_STREAM(data)   ((data)->stream)
#define M2S_DATA_PID(data)      ((data)->pid)
#define M2S_DATA_INSTANCE(data) ((data)->instance)
#define M2S_DATA_DBUF(data)     ((data)->dbuf)
#define M2S_DATA_MASKED(data)   ((data)->masked)
#define M2S_DATA_FPREAD(data)   ((data)->fpRead)
#define M2S_DATA_RBYPASS(data)  ((data)->readBypass)
#define M2S_DATA_WBYPASS(data)  ((data)->writeBypass)
#define M2S_DATA_MID(data)      ((data)->mid)
#define M2S_DATA_MOD(data)      ((data)->mod)

/* Data passed to Multi2Sim */
struct m2s_data
{
  ub4 pid;            /* Requesting processing unit */
  ub8 seqno;          /* Sequence number */
  ub8 vaddr;          /* Virtual address to be looked at */
  ub4 size;           /* Size of access */
  ub1 access_kind;    /* Store / Load */
  ub1 stream;         /* Attila stream */
  ub8 instance;       /* For debugging to track issuing object */
  ub1 dbuf;           /* True if request is coming from default buffer */
  ub1 masked;         /* Block is partially written */
  ub1 fpRead;         /* Flag for read fast path */
  ub1 readBypass;     /* Flag for read allocate */
  ub1 writeBypass;    /* Flag for write allocate */
  sb4 mid;            /* Memory map ID */
  struct mod_t *mod;  /* Pointer to agent */
};

/* Macros to check return status */
#define ACCESS_IS_HIT(data)             ((data)->status = JF_HIT)
#define ACCESS_IS_MISS(data)            ((data)->status = JF_MISS)
#define ACCESS_IS_MISS_CE(data)         ((data)->status = JF_MISS_CE)
#define ACCESS_IS_MISS_DE(data)         ((data)->status = JF_MISS_DE)
#define ACCESS_IS_MISS_AND_BYPASS(data) ((data)->status = JF_MISS);\
                                        ((data)->bypass = TRUE)

#define IS_ACCESS_HIT(data)             ((data)->status == JF_HIT)
#define IS_ACCESS_MISS(data)            ((data)->status == JF_MISS)
#define IS_ACCESS_MISS_CE(data)         ((data)->status == JF_MISS_CE)
#define IS_ACCESS_MISS_DE(data)         ((data)->status == JF_MISS_DE)
#define IS_ACCESS_BYPASS(data)          ((data)->bypass == TRUE)

#define INTRMOD_DATA_ATTILA_DATA(data)  (&((data)->attila_data))
#define INTRMOD_DATA_M2S_DATA(data)     (&((data)->m2s_data))
#define INTRMOD_DATA_KEY(data)          ((data)->key)
#define INTRMOD_DATA_CB(data)           ((data)->cb)
#define INTRMOD_DATA_STATUS(data)       ((data)->status)
#define INTRMOD_DATA_BYPASS(data)       ((data)->bypass)
#define INTRMOD_DATA_PSTRM(data)        ((data)->pstrm)
#define INTRMOD_DATA_AINFO(data)        ((data)->ainfo)

/* Structure passed between modules */
struct intermod_data
{
  union
  {
    struct attila_data attila_data;
    struct m2s_data    m2s_data;
  };
  
  void (*cb)(struct intermod_data *);
  void *key;    /* Return data */
  ub1  status;  /* LLC_HIT / LLC_MISS */
  ub1  bypass;  /* LLC bypass */
  ub1  pstrm;   /* In case of hit old stream */
  mai *ainfo;   /* Memory access info */
};

#define M2S_CONFIG_MOD(conf)      ((conf)->mod)
#define M2S_CONFIG_MID(conf)      ((conf)->agent_mid)
#define M2S_CONFIG_BLCKSZ(conf)   ((conf)->cache_blck_sz)
#define M2S_CONFIG_INI(conf)      ((conf)->ini)
#define M2S_CONFIG_REQ(conf)      ((conf)->req)
#define M2S_CONFIG_CLK(conf)      ((conf)->clk)

/* Multi2Sim configuration */
struct m2s_config
{
  void   *mod;                           /* Agent module */
  sb4    agent_mid;                      /* Memeory map id */
  ub1    cache_blck_sz;                  /* LLC block size */
  void   (*ini)(int argc, char** argv);  /* Initilization function */
  void   (*req)(struct intermod_data *); /* Public function to sbmit req to m2s */
  char   (*clk)(long int clock);         /* Clocking function to m2s */
};

/* Macros to access attila_config members */
#define ATTILA_CONFIG_UC_RBYPASS(conf)  (((conf)->uc_read_bypass))
#define ATTILA_CONFIG_UC_WBYPASS(conf)  (((conf)->uc_write_bypass))
#define ATTILA_CONFIG_I_RBYPASS(conf)   (((conf)->uc_read_bypass)[IC - 1])
#define ATTILA_CONFIG_I_WBYPASS(conf)   (((conf)->uc_write_bypass)[IC - 1])
#define ATTILA_CONFIG_C_RBYPASS(conf)   (((conf)->uc_read_bypass)[CC - 1])
#define ATTILA_CONFIG_C_WBYPASS(conf)   (((conf)->uc_write_bypass)[CC - 1])
#define ATTILA_CONFIG_Z_RBYPASS(conf)   (((conf)->uc_read_bypass)[ZC - 1])
#define ATTILA_CONFIG_Z_WBYPASS(conf)   (((conf)->uc_write_bypass)[ZC - 1])
#define ATTILA_CONFIG_T_RBYPASS(conf)   (((conf)->uc_read_bypass)[TC - 1])
#define ATTILA_CONFIG_T_WBYPASS(conf)   (((conf)->uc_write_bypass)[TC - 1])
#define ATTILA_CONFIG_H_RBYPASS(conf)   (((conf)->uc_read_bypass)[HZ - 1])
#define ATTILA_CONFIG_H_WBYPASS(conf)   (((conf)->uc_write_bypass)[HZ - 1])
#define ATTILA_CONFIG_N_RBYPASS(conf)   (((conf)->uc_read_bypass)[NC - 1])
#define ATTILA_CONFIG_N_WBYPASS(conf)   (((conf)->uc_write_bypass)[NC - 1])

#define ATTILA_CONFIG_S_RBYPASS(conf)   (((conf)->sc_read_bypass))
#define ATTILA_CONFIG_S_WBYPASS(conf)   (((conf)->sc_write_bypass))
#define ATTILA_CONFIG_SI_RBYPASS(conf)  (((conf)->sc_read_bypass)[IC - 1])
#define ATTILA_CONFIG_SI_WBYPASS(conf)  (((conf)->sc_write_bypass)[IC - 1])
#define ATTILA_CONFIG_SC_RBYPASS(conf)  (((conf)->sc_read_bypass)[CC - 1])
#define ATTILA_CONFIG_SC_WBYPASS(conf)  (((conf)->sc_write_bypass)[CC - 1])
#define ATTILA_CONFIG_SZ_RBYPASS(conf)  (((conf)->sc_read_bypass)[ZC - 1])
#define ATTILA_CONFIG_SZ_WBYPASS(conf)  (((conf)->sc_write_bypass)[ZC - 1])
#define ATTILA_CONFIG_ST_RBYPASS(conf)  (((conf)->sc_read_bypass)[TC - 1])
#define ATTILA_CONFIG_ST_WBYPASS(conf)  (((conf)->sc_write_bypass)[TC - 1])
#define ATTILA_CONFIG_SH_RBYPASS(conf)  (((conf)->sc_read_bypass)[HZ - 1])
#define ATTILA_CONFIG_SH_WBYPASS(conf)  (((conf)->sc_write_bypass)[HZ - 1])
#define ATTILA_CONFIG_SN_RBYPASS(conf)  (((conf)->sc_read_bypass)[NC - 1])
#define ATTILA_CONFIG_SN_WBYPASS(conf)  (((conf)->sc_write_bypass)[NC - 1])

#define ATTILA_CONFIG_LC_RBYPASS(conf)          (((conf)->lc_read_bypass))
#define ATTILA_CONFIG_LC_WBYPASS(conf)          (((conf)->lc_write_bypass))
#define ATTILA_CONFIG_LC_BYPASS_ALL_BUF(conf)   (((conf)->lc_bypass_all_buf))
#define ATTILA_CONFIG_LC_FP_READ(conf)          (((conf)->lc_fp_read))

#define ATTILA_CONFIG_LCI_RBYPASS(conf)         (((conf)->lc_read_bypass)[IC - 1])
#define ATTILA_CONFIG_LCI_WBYPASS(conf)         (((conf)->lc_write_bypass)[IC - 1])
#define ATTILA_CONFIG_LCI_BYPASS_ALL_BUF(conf)  (((conf)->lc_bypass_all_buf)[IC - 1])
#define ATTILA_CONFIG_LCI_FP_READ(conf)         (((conf)->lc_fp_read)[IC - 1])

#define ATTILA_CONFIG_LCC_RBYPASS(conf)         (((conf)->lc_read_bypass)[CC - 1])
#define ATTILA_CONFIG_LCC_WBYPASS(conf)         (((conf)->lc_write_bypass)[CC - 1])
#define ATTILA_CONFIG_LCC_BYPASS_ALL_BUF(conf)  (((conf)->lc_bypass_all_buf)[CC - 1])
#define ATTILA_CONFIG_LCC_FP_READ(conf)         (((conf)->lc_fp_read)[CC - 1])

#define ATTILA_CONFIG_LCZ_RBYPASS(conf)         (((conf)->lc_read_bypass)[ZC - 1])
#define ATTILA_CONFIG_LCZ_WBYPASS(conf)         (((conf)->lc_write_bypass)[ZC - 1])
#define ATTILA_CONFIG_LCZ_BYPASS_ALL_BUF(conf)  (((conf)->lc_bypass_all_buf)[ZC - 1])
#define ATTILA_CONFIG_LCZ_FP_READ(conf)         (((conf)->lc_fp_read)[ZC - 1])

#define ATTILA_CONFIG_LCT_RBYPASS(conf)         (((conf)->lc_read_bypass)[TC - 1])
#define ATTILA_CONFIG_LCT_WBYPASS(conf)         (((conf)->lc_write_bypass)[TC - 1])
#define ATTILA_CONFIG_LCT_BYPASS_ALL_BUF(conf)  (((conf)->lc_bypass_all_buf)[TC - 1])
#define ATTILA_CONFIG_LCT_FP_READ(conf)         (((conf)->lc_fp_read)[TC - 1])

#define ATTILA_CONFIG_LCH_RBYPASS(conf)         (((conf)->lc_read_bypass)[HZ - 1])
#define ATTILA_CONFIG_LCH_WBYPASS(conf)         (((conf)->lc_write_bypass)[HZ - 1])
#define ATTILA_CONFIG_LCH_BYPASS_ALL_BUF(conf)  (((conf)->lc_bypass_all_buf)[HZ - 1])
#define ATTILA_CONFIG_LCH_FP_READ(conf)         (((conf)->lc_fp_read)[HZ - 1])

#define ATTILA_CONFIG_LCN_RBYPASS(conf)         (((conf)->lc_read_bypass)[NC - 1])
#define ATTILA_CONFIG_LCN_WBYPASS(conf)         (((conf)->lc_write_bypass)[NC - 1])
#define ATTILA_CONFIG_LCN_BYPASS_ALL_BUF(conf)  (((conf)->lc_bypass_all_buf)[NC - 1])
#define ATTILA_CONFIG_LCN_FP_READ(conf)         (((conf)->lc_fp_read)[NC - 1])

#define ATTILA_CONFIG_LCB_RBYPASS(conf)         (((conf)->lc_read_bypass)[BT - 1])
#define ATTILA_CONFIG_LCB_WBYPASS(conf)         (((conf)->lc_write_bypass)[BT - 1])
#define ATTILA_CONFIG_LCB_BYPASS_ALL_BUF(conf)  (((conf)->lc_bypass_all_buf)[BT - 1])
#define ATTILA_CONFIG_LCB_FP_READ(conf)         (((conf)->lc_fp_read)[BT - 1])

#define ATTILA_CONFIG_LCD_RBYPASS(conf)         (((conf)->lc_read_bypass)[DU - 1])
#define ATTILA_CONFIG_LCD_WBYPASS(conf)         (((conf)->lc_write_bypass)[DU - 1])
#define ATTILA_CONFIG_LCD_BYPASS_ALL_BUF(conf)  (((conf)->lc_bypass_all_buf)[DU - 1])
#define ATTILA_CONFIG_LCD_FP_READ(conf)         (((conf)->lc_fp_read)[DU - 1])

#define ATTILA_CONFIG_LCX_RBYPASS(conf)         (((conf)->lc_read_bypass)[IX - 1])
#define ATTILA_CONFIG_LCX_WBYPASS(conf)         (((conf)->lc_write_bypass)[IX - 1])
#define ATTILA_CONFIG_LCX_BYPASS_ALL_BUF(conf)  (((conf)->lc_bypass_all_buf)[IX - 1])
#define ATTILA_CONFIG_LCX_FP_READ(conf)         (((conf)->lc_fp_read)[IX - 1])

#define ATTILA_CONFIG_I_BLCKSZ(conf)  (((conf)->cache_blck_sz)[IC - 1])
#define ATTILA_CONFIG_C_BLCKSZ(conf)  (((conf)->cache_blck_sz)[CC - 1])
#define ATTILA_CONFIG_Z_BLCKSZ(conf)  (((conf)->cache_blck_sz)[ZC - 1])
#define ATTILA_CONFIG_T_BLCKSZ(conf)  (((conf)->cache_blck_sz)[TC - 1])
#define ATTILA_CONFIG_N_BLCKSZ(conf)  (((conf)->cache_blck_sz)[NC - 1])
#define ATTILA_CONFIG_H_BLCKSZ(conf)  (((conf)->cache_blck_sz)[HZ - 1])
#define ATTILA_CONFIG_SI_BLCKSZ(conf) (((conf)->cache_blck_sz)[SIC - 1])
#define ATTILA_CONFIG_SC_BLCKSZ(conf) (((conf)->cache_blck_sz)[SCC - 1])
#define ATTILA_CONFIG_SZ_BLCKSZ(conf) (((conf)->cache_blck_sz)[SZC - 1])
#define ATTILA_CONFIG_ST_BLCKSZ(conf) (((conf)->cache_blck_sz)[STC - 1])
#define ATTILA_CONFIG_SH_BLCKSZ(conf) (((conf)->cache_blck_sz)[SHZ - 1])
#define ATTILA_CONFIG_SN_BLCKSZ(conf) (((conf)->cache_blck_sz)[SNC - 1])
#define ATTILA_CONFIG_BLCKSZ(conf)    ((conf)->cache_blck_sz)
#define ATTILA_CONFIG_LLC_ENBL(conf)  ((conf)->llc_enable)
#define ATTILA_CONFIG_RALLOC(conf)    ((conf)->llcReadAlloc)
#define ATTILA_CONFIG_INI(conf)       ((conf)->ini)
#define ATTILA_CONFIG_CB(conf)        ((conf)->cb)
#define ATTILA_CONFIG_CLK(conf)       ((conf)->clk)

/* Attila configuration */
struct attila_config
{
  ub4   uc_read_bypass[TUC];           /* Read bypass flags unit cache */
  ub4   uc_write_bypass[TUC];          /* Write bypass flags for unit cache */
  ub4   sc_read_bypass[TUC];           /* Read bypass flags for shared cache */
  ub4   sc_write_bypass[TUC];          /* Write bypass flags for shared cache */
  ub4   lc_read_bypass[TUC];           /* Read bypass flags LLC */
  ub4   lc_write_bypass[TUC];          /* Write bypass bitmap for LLC*/
  ub4   lc_bypass_all_buf[TUC];        /* Buffer to bypass (All / Default) */
  ub4   lc_fp_read[TUC];               /* Fast path read enable */
  ub4   cache_blck_sz[TUC];            /* Block size for ATTILA units */
  ub1   llc_enable;                    /* Flag to enable / disable llc */
  ub1   llcReadAlloc;                  /* Allocate block in llc on read */
  void  (*ini)(int argc, char **argv); /* Initialization function */
  void  (*cb)(struct intermod_data *); /* Callback function to attila */
  char  (*clk)(long int clock);        /* Clocking function to attila */
};

/* MacSim configuration */
struct macsim_config
{
  ub4   cache_blck_sz[4];               /* Block size for Macsim caches */
  void  (*ini)(int argc, char **argv);  /* Initialization function */
  void  (*cb)(struct intermod_data *);  /* Callback function to attila */
  char  (*clk)(long int clock);         /* Clocking function to attila */
};

/* Type to identify shader instruction */
typedef enum shader_instruction_type
{
  texture_operation,                /* Sampling instruction */
  alu_operation,                    /* ALU operation */
  branch_operation,                 /* Branch operation */
}shader_instruction_type;

/* State of a shader thread */
typedef struct shader_pipeline_state
{
  shader_thread_state thread_state; /* Thread state to track stalls */
}shader_pipeline_state;

/* Defines an entry of the memory request queue. */
typedef struct fc_llc_reqs
{
  ub1  req_state;     /* Current state of the request, used in stats collection. */
  ub1  unit;          /* Requesting unit */
  ub1  strm;          /* Request stream */
  ub4  pid;           /* Requesting processing id */
  ub8  inAddress;     /* Address of the first byte of the line requested into the cache. */
  ub8  outAddress;    /* Address of the first byte of the line requested out of the cache. */
  ub8  offset;        /* Offset of the request */
  ub8  line;          /* Set where the block is allocated */
  ub8  way;           /* Way in the set */
  ub8  size;          /* Size of data */
  ub1  alloc;         /* Block is to be allocate */
  ub1  spill;         /* Block is to be written back to memory. */
  ub1  fill;          /* Block is read from memory. */
  ub1  invalidate;    /* Block is invalidated from the cache */
  ub1  masked;        /* If write is masked */
  ub1  flush;         /* If request is due to flush */
  ub1  dbuf;          /* If default render target is in use */
  ub8  info;          /* Supplementary data */
  ub1  bypass;        /* Set if fill is to be bypassed */  
  ub1  fpath;         /* Set if fill taken fast path */  
  ub1  cmiss;         /* Set if access takes cold miss */
  ub1 *reqs;          /* CacheRequest from FetchCache */
  ub8  id;            /* Index of request in requestQueue */
  ub4  reserves;      /* Reservation of the block */
  ub1  rport_avl;     /* Flag to mark port availability */
  ub1  wport_avl;     /* Flag to mark port availability */
  ub1  mshr_avl;      /* Flag to mark mshr availability */
  ub8  scycle;        /* Request start cycle */
  ub8  ecycle;        /* Request end cycle */
  ub8  cache;         /* Pointer to cache issuing the request */
  ub8  scache;        /* Pointer to shared cache issuing the request */
  ub1  status;        /* Fate of the request (Hit / Miss)*/
  sb4  miplvl0;       /* Mip level 0 */
  sb4  miplvl1;       /* Mip level 1 */
  ub8  texture_base;  /* Texture base */
  mai *ainfo;       /* Memory access info */
}fc_llc_reqs;

typedef struct pc_stall_data
{
  ub8 pc;                       /* Program counter */
  ub8 access_count;             /* Times pc was at the head */
  ub8 life_cycles;              /* Total cycles in ROB */
  ub8 stall_at_head;            /* Total stall at the ROB head */
  ub8 stall_count;              /* Total stalls */
  ub8 stall_cycles;             /* Total ROB head stall cycles */
  ub8 head_insert_cycle;        /* Cycle when uop has reached at the ROB head */
  ub8 local_stall_cycles;       /* Temporary stall cycles at ROB head */
  ub8 miss_l1;                  /* L1 miss count */
  ub8 miss_l2;                  /* L2 miss count */
  ub8 miss_llc;                 /* LLC miss count */
  ub8 access_llc;               /* LLC miss count */
  ub8 x_set_count;              /* PC in X set */
  ub8 y_set_count;              /* PC in Y set */
  ub8 p_set_count;              /* PC in P set */
  ub8 q_set_count;              /* PC in Q set */
  ub8 r_set_count;              /* PC in R set */
  ub8 rob_pos;                  /* Position of LLC missed load in ROB */
  ub8 rob_loads;                /* Total loads */
  ub1 in_r_set;                 /* TRUE if PC belong to R set */
  ub8 stat_access_count;        /* Times pc was at the head */
  ub8 stat_life_cycles;         /* Total cycles in ROB */
  ub8 stat_stall_at_head;       /* Total stall at the ROB head */
  ub8 stat_stall_count;         /* Total stalls */
  ub8 stat_stall_cycles;        /* Total ROB head stall cycles */
  ub8 stat_head_insert_cycle;   /* Cycle when uop has reached at the ROB head */
  ub8 stat_local_stall_cycles;  /* Temporary stall cycles at ROB head */
  ub8 stat_miss_l1;             /* L1 miss count */
  ub8 stat_miss_l2;             /* L2 miss count */
  ub8 stat_miss_llc;            /* LLC miss count */
  ub8 stat_access_llc;          /* LLC miss count */
  ub8 stat_x_set_count;         /* PC in X set */
  ub8 stat_y_set_count;         /* PC in Y set */
  ub8 stat_p_set_count;         /* PC in P set */
  ub8 stat_q_set_count;         /* PC in Q set */
  ub8 stat_r_set_count;         /* PC in R set */
  ub8 stat_set_recat;           /* # PC is recategorized after fill */
  ub8 stat_rob_pos;             /* Position of LLC missed load in ROB */
  ub8 stat_rob_loads;           /* Total loads */
  ub1 stat_in_r_set;            /* TRUE if PC belong to R set */
}pc_stall_data;

/* FragmentFIFO configuration statistics */
typedef struct ffifo_info
{
  ub1   fgi_enable;   /* TRUE is speedup is enabled */
  ub4   fgi_count;    /* Units with memory access */
  ub8   fgi_access;   /* Global data access count */
  ub1  *fgi_speedup;  /* Per unit speedup vector */
  sctr *fgi_utility;  /* Per unit measured utility */
  sctr *fgi_cutility; /* Per unit current utility */
}ffifo_info;

/* Shader configuration and statistics */
typedef struct shader_info
{
  ub4   sgi_count;     /* Unified shader count */
  ub8   sgi_access;    /* Global data access count */
  ub4  *sgi_rthreads;  /* Per shader ready threads */
  sctr *sgi_utility;   /* Per shader measured utility */
  sctr *sgi_cutility;  /* Per shader current utility */
}shader_info;

/* ROP configuration and statistics */
typedef struct rop_info
{
  ub4   rgi_count;    /* ROP count */
  ub8   rgi_access;   /* Global data access */
  sctr *rgi_fills;    /* ROP pixel fills */
  sctr *rgi_utility;  /* ROP measured utility */
  sctr *rgi_cutility; /* ROP current utility */
}rop_info;

typedef struct frontend_info
{
  ub4  fegi_count;      /* Front-end pipes */
  ub8  fegi_access;     /* Global data access */
  sctr fegi_occupancy;  /* Front-end occupancy */
  sctr fegi_utility;    /* Front-end measured utility */
  sctr fegi_cutility;   /* Front-end current utility */
}frontend_info;

/* Info dumped in the file */
typedef struct memory_trace_in_file
{
  ub8 address;
  ub1 stream;
  ub1 fill;
  ub1 spill;
}memory_trace_in_file;

/* Info dumped in the file */
typedef struct memory_trace
{
  ub8   address;      /* Physical address */
  ub8   vtl_addr;     /* Virtual address */
  ub8   pc;           /* PC */
  ub1   stream;       /* Accessing stream */
  ub4   core;         /* CPU / GPU core id */
  ub4   pid;          /* Processing unit id */
  ub4   mapped_rop;   /* ROP segment request belongs to */
  ub1   dbuf;         /* True if address belong to default buffer */
  ub1   fill;         /* True if block is to be read */
  ub1   spill;        /* True if block is written */
  ub1   prefetch;     /* True for pre-fetch */
  ub1   dirty;        /* True if write-back is dirty */
  ub1   sap_stream;   /* stream identified by new policy (member of enum sap_stream)*/
  void *policy_data;  /* Policy specific data */
}memory_trace;

/* Cache access return status */
#define CACHE_ACCESS_UNK  (0)
#define CACHE_ACCESS_HIT  (1)
#define CACHE_ACCESS_MISS (2)
#define CACHE_ACCESS_RPLC (3)

/* This structure is used for cache access */
typedef struct cache_access_status
{
  ub8 tag;          /* Tag */
  ub8 vtl_addr;     /* Virtual address */
  ub1 fate;         /* Hit / Miss */
  sb8 way;          /* Accessed way */
  ub1 stream;       /* Accessing stream */
  ub8 pc;           /* Last access PC */
  ub8 fillpc;       /* Block filling PC */
  ub1 dirty;        /* Set if block is dirty */
  ub1 epoch;        /* Current epoch of the block */
  ub8 access;       /* Block reuse count */
  sb8 rdis;         /* Current sequence number of the block */
  sb8 prdis;        /* Previous sequence number of the block */
  ub4 last_rrpv;    /* Last RRPV seen by the block */
  ub4 fill_rrpv;    /* RRPV the block was filled at */
  ub1 is_bt_block;  /* TRUE if block is spilled by blitter */
  ub1 is_bt_pred;   /* TRUE if block is predicted to be BT */
  ub1 is_ct_block;  /* TRUE if block is spilled by color writer */
  ub1 is_ct_pred;   /* TRUE if block is spilled by color writer */
  ub1 op;           /* Last operation on block */
}cache_access_status;

/* This structre is used to track per address stats */
typedef struct cache_access_info
{
  ub8 access_count; /* Total acceses */
  ub1 stream;       /* Spilling stream */
  ub8 first_seq;    /* Sequence number of first access */
  ub8 last_seq;     /* Sequence number of last sequence */
  ub8 list_head;    /* List head for all accesses to this accres */
  ub1 is_bt_block;  /* TRUE if block is spilled by blitter */
  ub1 btuse;        /* TRUE if BT block is sampled by texture */
  ub1 is_ct_block;  /* TRUE if block is spilled by color writer  */
  ub1 ctuse;        /* TRUE if block is sampled by texture */
  ub1 rrpv_trans;   /* TRUE if block is spilled by blitter */
}cache_access_info;

int gcd_ab(int a, int b);

#endif
